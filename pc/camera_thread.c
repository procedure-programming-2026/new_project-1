#include "camera_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>

static void set_error(CameraThread *cam, const char *msg)
{
    pthread_mutex_lock(&cam->mutex);
    snprintf(cam->last_error, sizeof(cam->last_error), "%s", msg);
    pthread_mutex_unlock(&cam->mutex);
}

static int is_running(CameraThread *cam)
{
    int value;
    pthread_mutex_lock(&cam->mutex);
    value = cam->running;
    pthread_mutex_unlock(&cam->mutex);
    return value;
}

static void *camera_worker(void *arg)
{
    CameraThread *cam = (CameraThread *)arg;

    while (is_running(cam)) {
        struct pollfd pfd;
        pfd.fd = cam->stream.server_fd;
        pfd.events = POLLIN;
        pfd.revents = 0;

        /* Чекаємо підключення з таймаутом 200 мс, щоб можна було коректно зупинити потік */
        int ready = poll(&pfd, 1, 200);
        if (ready < 0) {
            if (!is_running(cam)) {
                break;
            }
            set_error(cam, "Poll error on server socket");
            usleep(100000);
            continue;
        }

        if (ready == 0) {
            continue;
        }

        if (!is_running(cam)) {
            break;
        }

        int client_fd = accept(cam->stream.server_fd, NULL, NULL);
        if (client_fd < 0) {
            if (!is_running(cam)) {
                break;
            }
            set_error(cam, "Accept failed");
            usleep(100000);
            continue;
        }

        pthread_mutex_lock(&cam->mutex);
        cam->stream.client_fd = client_fd;
        cam->connected = 1;
        cam->last_error[0] = '\0';
        pthread_mutex_unlock(&cam->mutex);

        printf("[%s] Connected on port %d (%dx%d)\n",
               cam->name, cam->port, cam->width, cam->height);
        fflush(stdout);

        while (is_running(cam)) {
            if (stream_read(&cam->stream, cam->work_buffer, cam->frame_size) != 0) {
                break;
            }

            pthread_mutex_lock(&cam->mutex);
            memcpy(cam->frame_buffer, cam->work_buffer, cam->frame_size);
            cam->has_frame = 1;
            cam->frame_count++;
            pthread_mutex_unlock(&cam->mutex);
        }

        pthread_mutex_lock(&cam->mutex);
        if (cam->stream.client_fd >= 0) {
            close(cam->stream.client_fd);
            cam->stream.client_fd = -1;
        }
        cam->connected = 0;
        if (cam->running) {
            snprintf(cam->last_error, sizeof(cam->last_error),
                     "Disconnected (waiting reconnect)");
        }
        pthread_mutex_unlock(&cam->mutex);

        if (is_running(cam)) {
            printf("[%s] Disconnected, waiting for reconnection...\n", cam->name);
            fflush(stdout);
        }
    }

    return NULL;
}

int camera_thread_start(
    CameraThread *cam,
    const char *name,
    int port,
    int width,
    int height
) {
    memset(cam, 0, sizeof(*cam));

    cam->name = name;
    cam->port = port;
    cam->width = width;
    cam->height = height;
    cam->frame_size = (size_t)width * (size_t)height * 3;
    cam->stream.server_fd = -1;
    cam->stream.client_fd = -1;

    snprintf(cam->last_error, sizeof(cam->last_error),
             "Waiting for connection on port %d", port);

    cam->frame_buffer = calloc(1, cam->frame_size);
    cam->work_buffer = calloc(1, cam->frame_size);
    if (cam->frame_buffer == NULL || cam->work_buffer == NULL) {
        fprintf(stderr, "[%s] Out of memory\n", name);
        free(cam->frame_buffer);
        free(cam->work_buffer);
        return 1;
    }

    if (stream_open(&cam->stream, port) != 0) {
        fprintf(stderr, "[%s] Failed to open TCP port %d\n", name, port);
        free(cam->frame_buffer);
        free(cam->work_buffer);
        return 1;
    }

    if (pthread_mutex_init(&cam->mutex, NULL) != 0) {
        stream_close(&cam->stream);
        free(cam->frame_buffer);
        free(cam->work_buffer);
        return 1;
    }

    cam->running = 1;

    if (pthread_create(&cam->thread, NULL, camera_worker, cam) != 0) {
        cam->running = 0;
        pthread_mutex_destroy(&cam->mutex);
        stream_close(&cam->stream);
        free(cam->frame_buffer);
        free(cam->work_buffer);
        return 1;
    }

    return 0;
}

int camera_thread_get_frame(
    CameraThread *cam,
    unsigned char *dst,
    int *connected,
    unsigned long *frame_count,
    char *err_buf,
    size_t err_size
) {
    int has_frame;

    pthread_mutex_lock(&cam->mutex);

    has_frame = cam->has_frame;
    if (has_frame && dst != NULL) {
        memcpy(dst, cam->frame_buffer, cam->frame_size);
    }
    if (connected != NULL) {
        *connected = cam->connected;
    }
    if (frame_count != NULL) {
        *frame_count = cam->frame_count;
    }
    if (err_buf != NULL && err_size > 0) {
        snprintf(err_buf, err_size, "%s", cam->last_error);
    }

    pthread_mutex_unlock(&cam->mutex);

    return has_frame;
}

void camera_thread_stop(CameraThread *cam)
{
    pthread_mutex_lock(&cam->mutex);
    cam->running = 0;
    if (cam->stream.client_fd >= 0) {
        shutdown(cam->stream.client_fd, SHUT_RDWR);
    }
    if (cam->stream.server_fd >= 0) {
        shutdown(cam->stream.server_fd, SHUT_RDWR);
    }
    pthread_mutex_unlock(&cam->mutex);

    pthread_join(cam->thread, NULL);

    stream_close(&cam->stream);
    pthread_mutex_destroy(&cam->mutex);

    free(cam->frame_buffer);
    free(cam->work_buffer);
    cam->frame_buffer = NULL;
    cam->work_buffer = NULL;
}