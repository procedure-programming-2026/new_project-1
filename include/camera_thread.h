#ifndef CAMERA_THREAD_H
#define CAMERA_THREAD_H

#include <stddef.h>
#include <pthread.h>

#include "stream.h"

typedef struct {
    const char *name;
    int port;
    int width;
    int height;
    size_t frame_size;

    Stream stream;
    unsigned char *frame_buffer;
    unsigned char *work_buffer;

    pthread_t thread;
    pthread_mutex_t mutex;

    int running;
    int connected;
    int has_frame;
    unsigned long frame_count;
    char last_error[128];
} CameraThread;

int camera_thread_start(
    CameraThread *cam,
    const char *name,
    int port,
    int width,
    int height
);

/*
 * Копіює останній отриманий кадр у dst (якщо dst != NULL).
 * Повертає 1, якщо кадр уже є, або 0, якщо кадрів ще не надходило.
 */
int camera_thread_get_frame(
    CameraThread *cam,
    unsigned char *dst,
    int *connected,
    unsigned long *frame_count,
    char *err_buf,
    size_t err_size
);

void camera_thread_stop(CameraThread *cam);

#endif