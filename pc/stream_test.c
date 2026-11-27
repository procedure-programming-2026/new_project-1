
#include <stdio.h>
#include <stdlib.h>

#include "stream.h"
#include "image_processing.h"
#include "marker_tracking.h"
#include "hand_detection.h"
#include "visual_debugger.h"
#include "serial_comm.h"

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
        stream_close(&stream);
        return 1;
    }

    unsigned char *frame =
        malloc(WIDTH * HEIGHT * 3);

    if (frame == NULL) {
        stream_close(&stream);
        return 1;
    }

    HandDetector detector;

    if (hand_init(&detector, WIDTH, HEIGHT) != 0) {
        free(frame);
        stream_close(&stream);
        return 1;
    }

    VisualDebugger debugger;

    if (visual_init(&debugger, WIDTH, HEIGHT) != 0) {
        hand_free(&detector);
        free(frame);
        stream_close(&stream);
        return 1;
    }

    MarkerTracker tracker;
    SerialPort serial_port;
    tracker_init(&tracker, 15);
    serial_init(&serial_port, "/dev/ttyUSB0");
    int frame_count = 0;

    printf("Visual debugger started\n");
    printf("M - Toggle mask\n");
    printf("ESC - Exit\n");

    fflush(stdout);

    while (
        stream_read(
            &stream,
            frame,
            WIDTH * HEIGHT * 3
        ) == 0
    ) {

        Motion hand =
            hand_detect(&detector, frame);

        Marker target = {
            .found = hand.found,
            .x = hand.x,
            .y = hand.y,
            .pixels = hand.pixels,
            .grip = hand.grip
        };

        Direction direction =
            tracker_update(&tracker, target);
        static int frame_div = 0;
        if (++frame_div >= 2){
            frame_div = 0;
            serial_send_state(&serial_port, direction, &target);
        }
        frame_count++;

        if (frame_count % 15 == 0) {

            if (hand.found) {

                printf(
                    "Candidate | X=%d Y=%d Pixels=%d Direction=%s\n",
                    WIDTH - 1 - hand.x,
                    hand.y,
                    hand.pixels,
                    direction_to_string(direction)
                );

            } else {

                printf("No candidate | Direction=STOP\n");
            }

            fflush(stdout);
        }

        if (visual_update(
            &debugger,
            frame,
            &detector,
            hand,
            direction
        ) != 0) {
            break;
        }
    }

    printf("Camera stopped | Direction=STOP\n");

    visual_free(&debugger);
    hand_free(&detector);

    free(frame);

    stream_close(&stream);

    return 0;
}
