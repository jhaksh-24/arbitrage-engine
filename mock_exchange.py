import socket
import struct
import time

MCAST_GRP = '239.255.0.1'
MCAST_PORT = 12345

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

# Python's 'struct' module lets us pack binary data.
# '=' means "no padding/alignment" (just like #pragma pack(1))
# 'c' = char (1 byte)
# 'H' = unsigned short (2 bytes)
# 'Q' = unsigned long long (8 bytes)
# 'B' = unsigned char (1 byte)
# 'q' = signed long long (8 bytes)
# Total: 1+2+8+8+1+8+8 = 36 bytes
fmt = '=cHQQBqq'

msg_type = b'A'
length = struct.calcsize(fmt)
timestamp = int(time.time() * 1e9) # Current time in nanoseconds
order_id = 1001
side = 0 # 0 for BUY, 1 for SELL
price = 5000000  # Our engine uses int64_t for prices (50,000.00)
qty = 100000000  # Our engine uses int64_t for qty (1.0)

# Pack it into raw bytes!
packet = struct.pack(fmt, msg_type, length, timestamp, order_id, side, price, qty)

print(f"Sending AddOrder packet ({length} bytes) to {MCAST_GRP}:{MCAST_PORT}...")
sock.sendto(packet, (MCAST_GRP, MCAST_PORT))
