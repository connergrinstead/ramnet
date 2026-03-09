import socket
import struct
import sys
import time
from pydub import AudioSegment

SERVER_IP    = "127.0.0.1"
COMMAND_PORT = 5555
AUDIO_PORT   = 5556

SAMPLE_RATE       = 44100   # 44.1kHz = 44100 frames/second
FRAMES_PER_PACKET = 512     # 1 frame = 2 samples
BYTES_PER_FRAME   = 4       # 16 bit channel * 2 (left & right) = stereo


def stream_audio(audio_sock: socket, timeout: int, audio_data: bytes, entry_frame: int):
    audio_data = audio_data[entry_frame * BYTES_PER_FRAME:]
    frame      = entry_frame
    seq        = 0 # Track packets (this resets after every 'play')

    # High-precision timing
    next_frame_time = time.perf_counter()
    packet_size = FRAMES_PER_PACKET * BYTES_PER_FRAME

    for i in range(0, (BYTES_PER_FRAME * SAMPLE_RATE * timeout), packet_size):
        chunk = audio_data[i:i + packet_size]
        if len(chunk) < packet_size:
            break

        header = struct.pack(">HHI", seq, 0, frame) # big-endian: 2B + 2B + 4B
        audio_sock.sendto(header + chunk, (SERVER_IP, AUDIO_PORT))

        seq = (seq + 1) % 65536
        frame += FRAMES_PER_PACKET

        # Maintain accurate timing
        next_frame_time += FRAMES_PER_PACKET / float(SAMPLE_RATE)
        while time.perf_counter() < next_frame_time:
            pass  # spin-wait for high precision

    return frame


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: send_audio.py <audio_file>")
        sys.exit(1)

    # Load audio
    audio = AudioSegment.from_file(sys.argv[1])
    audio = audio.set_frame_rate(SAMPLE_RATE).set_channels(2).set_sample_width(2)
    data = bytes(audio.raw_data)
    current_frame = 0

    # Create sockets
    command_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    audio_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    command_sock.connect((SERVER_IP, COMMAND_PORT))
    command_sock.sendall(b"AUTH")

    response = command_sock.recv(8)
    if b"AUTH_OK" in response:
        print("Authenticated.\nStreaming audio.")
        # Preload min 4 packets on server -> 2048 frames = 4096 samples = 8192 bytes = 46 ms preload
        current_frame = stream_audio(audio_sock, 5, data, current_frame) # Stream audio for 5 seconds and update position

    else:
        print("Failed to authenticate. Exiting.")

    audio_sock.close()
    command_sock.close()