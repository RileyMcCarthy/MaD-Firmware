import socket

HOST = '127.0.0.1'  # IP address where the server is running
PORT = 12346        # Port number the server is listening on

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((HOST, PORT))
server_socket.listen()

print(f"Server is listening on {HOST}:{PORT}")

while True:
    client_socket, addr = server_socket.accept()
    print(f"Accepted connection from {addr}")

    # Handle data from the client
    data = client_socket.recv(1024)
    if not data:
        break

    print(f"Received from client: {data.decode()}")

    # Simulate a response back to the client
    response = "Response from Python server\n"
    client_socket.sendall(response.encode())

    # Close the client socket
    client_socket.close()

# Close the server socket (not shown here)

client_socket.close()
server_socket.close()
