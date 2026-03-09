import socket
import struct
import sys
import time
from pydub import AudioSegment

SERVER_IP = "127.0.0.1"
SERVER_PORT = 5556

FRAMES_PER_PACKET = 512
BYTES_PER_FRAME = 4  # 16 bit channel * 2 (left & right) = stereo

# Load audio
audio = AudioSegment.from_file(sys.argv[1])
audio = audio.set_frame_rate(44100).set_channels(2).set_sample_width(2)
data = bytes(audio.raw_data)

# Create socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

seq   = 0 # Track packets
frame = 0 # Track current frame

# High-precision timing
next_frame_time = time.perf_counter()
packet_size = FRAMES_PER_PACKET * BYTES_PER_FRAME

for i in range(0, len(data), packet_size):
    chunk = data[i:i + packet_size]
    if len(chunk) < packet_size:
        break

    header = struct.pack(">HHI", seq, 0, frame) # big-endian: 2B + 2B + 4B
    sock.sendto(header + chunk, (SERVER_IP, SERVER_PORT))

    seq = (seq + 1) % 65536
    frame += FRAMES_PER_PACKET

    # Maintain accurate timing
    next_frame_time += FRAMES_PER_PACKET / 44100.0
    while time.perf_counter() < next_frame_time:
        pass  # spin-wait for high precision