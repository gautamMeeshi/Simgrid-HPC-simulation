import socket

PORT = 8080
ADDR = "127.0.0.1"

try:
    skt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
except Exception as e:
    print(e)
    print('socket creation failed')

skt.bind((ADDR, PORT))
skt.listen(5)
print("socket is listening")


clientSocket, addr = skt.accept()
print(clientSocket.recv(1024).decode())
clientSocket.send('Thanks for reaching out'.encode())

clientSocket.close()
