
#include <stdio.h>
#include <stdlib.h>

#include "stream.h"
#include "image_processing.h"
#include "marker_tracking.h"

#define WIDTH 640
#define HEIGHT 480

int main(void) {
    Stream stream;

    if (stream_open(&stream, 5000) != 0) {
        fprintf(stderr, "Failed to open stream\n");
        return 1;
    }

    printf("Waiting for camera connection...\n");
    fflush(stdout);

    if (stream_accept(&stream) != 0) {
        fprintf(stderr, "Failed to accept connection\n");
        stream_close(&stream);
        return 1;
    }

    unsigned char *frame = malloc(WIDTH * HEIGHT * 3);

    if (frame == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        stream_close(&stream);
        return 1;
    }

    MarkerTracker tracker;
    tracker_init(&tracker, 15);

    int frame_count = 0;

    while (stream_read(&stream, frame, WIDTH * HEIGHT * 3) == 0) {
        Marker marker = find_green_marker(frame, WIDTH, HEIGHT);
        Direction direction = tracker_update(&tracker, marker);

        frame_count++;

        if (frame_count % 15 == 0) {
            if (marker.found) {
                printf("X=%d Y=%d Pixels=%d Direction=%s\n",
                       marker.x,
                       marker.y,
                       marker.pixels,
                       direction_to_string(direction));
            } else {
                printf("Marker not found | Direction=STOP\n");
            }

            fflush(stdout);
        }
    }

    free(frame);
    stream_close(&stream);

    return 0;
}
