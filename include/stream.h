#ifndef STREAM_H
#define STREAM_H

#include <stddef.h>

typedef struct {
    int server_fd;
    int client_fd;
} Stream;

int stream_open(Stream *stream, int port);
int stream_accept(Stream *stream);
int stream_read(Stream *stream, void *buffer, size_t size);
void stream_close(Stream *stream);

#endif