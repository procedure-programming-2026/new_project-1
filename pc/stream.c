#include "stream.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>

int stream_open(Stream *stream, int port)
{
    stream->server_fd = -1;
    stream->client_fd = -1;

    stream->server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (stream->server_fd < 0) {
        perror("Socket creation error");
        return 1;
    }

    int opt = 1;

    setsockopt(stream->server_fd, SOL_SOCKET, SO_REUSEADDR,
               &opt, sizeof(opt));

    struct sockaddr_in address = {0};

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    if (bind(stream->server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0) {
        perror("Socket bind error");
        stream_close(stream);
        return 1;
    }

    if (listen(stream->server_fd, 1) < 0) {
        perror("Socket listen error");
        stream_close(stream);
        return 1;
    }

    printf("Waiting for connection on port %d...\n", port);

    return 0;
}

int stream_accept(Stream *stream)
{
    stream->client_fd = accept(stream->server_fd, NULL, NULL);

    if (stream->client_fd < 0) {
        perror("Connection error");
        return 1;
    }

    printf("Client connected!\n");

    return 0;
}

int stream_read(Stream *stream, void *buffer, size_t size)
{
    size_t received = 0;

    while (received < size) {
        ssize_t result = recv(
            stream->client_fd,
            (char *)buffer + received,
            size - received,
            0
        );

        if (result == 0) {
            fprintf(stderr, "Connection closed\n");
            return 1;
        }

        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }

            perror("Receive error");
            return 1;
        }

        received += (size_t)result;
    }

    return 0;
}

void stream_close(Stream *stream)
{
    if (stream->client_fd >= 0) {
        close(stream->client_fd);
        stream->client_fd = -1;
    }

    if (stream->server_fd >= 0) {
        close(stream->server_fd);
        stream->server_fd = -1;
    }
}