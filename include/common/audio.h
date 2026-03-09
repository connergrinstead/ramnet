#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stdlib.h>

#define SAMPLE_RATE 44100
#define CHANNELS 2
#define FRAMES_PER_BUFFER 512

// 4 seconds buffer
#define BUFFER_FRAMES (SAMPLE_RATE * 4)
#define BUFFER_SAMPLES (BUFFER_FRAMES * CHANNELS)

#define PRELOAD_FRAMES 2048
#define PRELOAD_SAMPLES (PRELOAD_FRAMES * CHANNELS)

int audio_init(void);
void audio_shutdown(void);
void handle_audio(const uint8_t *packet, size_t len);

#endif