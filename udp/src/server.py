import socket

size = 8192

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('127.0.0.1', 9999))
count = 0

try:
    while True:
        data, address = sock.recvfrom(size)
        count += 1
        response = str(count) + ' ' + data.decode('UTF-8')
        sock.sendto(response.encode('utf-8'), address)
finally:
    sock.close()
