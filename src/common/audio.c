#include "audio.h"
#include <portaudio.h>
#include <stdio.h>
#include <stdatomic.h>
#include <string.h>
#include <unistd.h> // usleep

#define SAMPLE_RATE 44100
#define CHANNELS 2
#define FRAMES_PER_BUFFER 512

// 4 seconds of audio
#define BUFFER_FRAMES (SAMPLE_RATE * 4)
#define BUFFER_SAMPLES (BUFFER_FRAMES * CHANNELS)

static atomic_bool stream_playing = ATOMIC_VAR_INIT(false);

static int16_t ring_buffer[BUFFER_SAMPLES];

static atomic_size_t write_index = 0;
static atomic_size_t read_index  = 0;

static PaStream *stream = NULL;

// ---- Ring buffer helpers ----
static inline size_t rb_available()
{
    size_t w = atomic_load_explicit(&write_index, memory_order_acquire);
    size_t r = atomic_load_explicit(&read_index, memory_order_acquire);
    return (w >= r) ? (w - r) : (BUFFER_SAMPLES + w - r);
}

static inline size_t rb_space()
{
    return BUFFER_SAMPLES - rb_available();
}

// ---- PortAudio callback ----
static int pa_callback(const void *input,
                       void *output,
                       unsigned long frameCount,
                       const PaStreamCallbackTimeInfo *timeInfo,
                       PaStreamCallbackFlags statusFlags,
                       void *userData)
{
    int16_t *out = (int16_t*)output;
    size_t r = atomic_load_explicit(&read_index, memory_order_relaxed);
    size_t w = atomic_load_explicit(&write_index, memory_order_acquire);

    size_t available = (w >= r) ? (w - r) : (BUFFER_SAMPLES + w - r);
    size_t samples_needed = frameCount * CHANNELS;

    // Start playback only if enough frames are preloaded
    if (!atomic_load(&stream_playing) && available >= PRELOAD_SAMPLES)
    {
        atomic_store(&stream_playing, true);
    }

    for (size_t i = 0; i < samples_needed; i++)
    {
        if (!atomic_load(&stream_playing) || i >= available)
        {
            out[i] = 0; // silence until preloaded or underrun
        }
        else
        {
            out[i] = ring_buffer[r % BUFFER_SAMPLES];
            r++;
        }
    }

    atomic_store_explicit(&read_index, r, memory_order_release);

    // Pause playback if buffer is empty after reading
    available = (w >= r) ? (w - r) : (BUFFER_SAMPLES + w - r);
    if (atomic_load(&stream_playing) && available == 0)
    {
        atomic_store(&stream_playing, false);
    }

    return paContinue;
}

// ---- Initialize audio ----
int audio_init(void)
{
    PaError err;

    err = Pa_Initialize();
    if(err != paNoError)
    {
        fprintf(stderr,"PortAudio error: %s\n",Pa_GetErrorText(err));
        return -1;
    }

    // Clear buffer and indices
    memset(ring_buffer, 0, sizeof(ring_buffer));
    atomic_store(&write_index, 0);
    atomic_store(&read_index, 0);

    err = Pa_OpenDefaultStream(
        &stream,
        0,
        CHANNELS,
        paInt16,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        pa_callback,
        NULL
    );

    if(err != paNoError)
    {
        fprintf(stderr,"Stream error: %s\n",Pa_GetErrorText(err));
        return -1;
    }

    Pa_StartStream(stream);

    return 0;
}

// ---- Shutdown audio ----
void audio_shutdown(void)
{
    if(stream)
    {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        Pa_Terminate();
        stream = NULL;
    }
}

// ---- Handle incoming audio packets ----
void handle_audio(const uint8_t *packet, size_t len)
{
    if(len <= 8) return;

    const uint8_t *pcm_bytes = packet + 8;
    size_t pcm_bytes_len = len - 8;

    if(pcm_bytes_len % 2 != 0)
        return; // must be full samples

    size_t samples = pcm_bytes_len / 2;

    size_t w = atomic_load(&write_index);
    size_t r = atomic_load(&read_index);

    size_t space = (w >= r) ? (BUFFER_SAMPLES - (w - r)) : (r - w);
    if(samples > space)
        printf("Overrun. Skipping ahead.\n");
        //samples = space;

    for(size_t i=0;i<samples;i++)
    {
        int16_t sample;
        memcpy(&sample, pcm_bytes + i*2, 2);
        ring_buffer[w % BUFFER_SAMPLES] = sample;
        w++;
    }

    atomic_store(&write_index, w);
}

// ---- Wait until buffer has enough data to avoid initial underrun ----
void audio_wait_prefill(size_t min_samples)
{
    while(rb_available() < min_samples)
        usleep(500); // 0.5ms
}
