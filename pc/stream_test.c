
#include <stdio.h>
#include <stdlib.h>
#include "stream.h"
#include "image_processing.h"

#define WIDTH 640
#define HEIGHT 480
#define CHANNELS 3
#define FRAME_SIZE (WIDTH * HEIGHT * CHANNELS)

int main(void)
{
    Stream stream = {-1, -1};

    if (stream_open(&stream, 5000) != 0) {
        return 1;
    }

    if (stream_accept(&stream) != 0) {
        stream_close(&stream);
        return 1;
    }

    unsigned char *frame = malloc(FRAME_SIZE);

    if (frame == NULL) {
        perror("Memory allocation error");
        stream_close(&stream);
        return 1;
    }

    unsigned int frame_count = 0;

    while (stream_read(&stream, frame, FRAME_SIZE) == 0) {
        frame_count++;

        if (frame_count % 15 == 0) {
            Marker marker = find_green_marker(frame, WIDTH, HEIGHT);

            if (marker.found) {
                printf("GREEN MARKER: X=%d Y=%d Pixels=%d\n",
                       marker.x, marker.y, marker.pixels);
            } else {
                printf("Marker not found\n");
            }

            fflush(stdout);
        }
    }

    free(frame);
    stream_close(&stream);

    return 0;
}
