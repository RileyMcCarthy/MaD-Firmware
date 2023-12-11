#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include "StaticQueue.h"

static char socket_ip[64] = "127.0.0.1";
static int socket_port = 1234;

void socketio_set_address(char* address, int port)
{
    strncpy(socket_ip, address, 64);
    socket_port = port;
}

void socketio_send_str(int32_t socket_id, char* data)
{
    // Send data to the server
    send(socket_id, data, strlen(data), 0);
}

void socketio_send(int32_t socket_id, uint8_t data)
{
    // Send data to the server
    uint8_t buffer[1];
    buffer[0] = data;
    send(socket_id, buffer, sizeof(uint8_t), 0);
}

int32_t socketio_create_socket(char* id)
{
    struct sockaddr_in server_addr;

    // Create socket
    int32_t socket_id = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_id < 0) {
        perror("Error in socket creation");
        exit(1);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(socket_port);
    // Use inet_pton to convert the IP address to binary form
    if (inet_pton(AF_INET, socket_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(1);
    }

    // Connect to the server
    if (connect(socket_id, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error in connection");
    }

    // Send the ID as the first message if provided
    if (id != NULL) {
        socketio_send_str(socket_id, id);
    }

    return socket_id;
}

void socketio_close(int32_t socket_id)
{
    // Close the socket
    close(socket_id);
}

int32_t socketio_receive(int32_t socket_id, uint8_t* data, uint16_t length)
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(socket_id, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;  // Set the timeout to 10 second
    timeout.tv_usec = 0;

    int ready = select(socket_id + 1, &readfds, NULL, NULL, &timeout);

    if (ready > 0 && FD_ISSET(socket_id, &readfds))
    {
        int16_t bytes_received = recv(socket_id, data, length, 0);
        if (bytes_received < 0)
        {
            perror("Error in receiving");
            bytes_received = 0;
        }
        return (uint16_t)bytes_received;
    }
    else if (ready == 0)
    {
        // Timeout occurred
        return 0;
    }
    else
    {
        // Handle error or connection closed
        perror("Error in select");
        return 0;
    }
}

void socketio_begin()
{
    // Nothing to do here
}



