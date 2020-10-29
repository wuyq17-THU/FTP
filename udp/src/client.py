import socket



size = 8192

try:

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    for i in range(51):
        sock.sendto(str(i).encode('utf-8'), ('127.0.0.1', 9999))
        print(sock.recv(size).decode('utf-8'))
    sock.close()
except:
    print("cannot reach the server")
