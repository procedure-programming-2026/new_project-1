#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "camera_thread.h"
#include "hand_detection.h"
#include "marker_tracking.h"
#include "gui.h"

#define WIN_WIDTH   1280
#define WIN_HEIGHT  720

#define CAM1_PORT   5000
#define CAM1_WIDTH  640
#define CAM1_HEIGHT 480

#define CAM2_PORT   5001
#define CAM2_WIDTH  1280
#define CAM2_HEIGHT 720

static volatile sig_atomic_t g_running = 1;

static void handle_signal(int sig)
{
    (void)sig;
    g_running = 0;
}

int main(void)
{
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    CameraThread cam1;
    CameraThread cam2;

    if (camera_thread_start(&cam1, "LAPTOP_CAM", CAM1_PORT, CAM1_WIDTH, CAM1_HEIGHT) != 0) {
        fprintf(stderr, "Failed to start CAM1 thread\n");
        return 1;
    }

    if (camera_thread_start(&cam2, "ROBOT_CAM", CAM2_PORT, CAM2_WIDTH, CAM2_HEIGHT) != 0) {
        fprintf(stderr, "Failed to start CAM2 thread\n");
        camera_thread_stop(&cam1);
        return 1;
    }

    unsigned char *cam1_frame = malloc((size_t)CAM1_WIDTH * CAM1_HEIGHT * 3);
    unsigned char *cam2_frame = malloc((size_t)CAM2_WIDTH * CAM2_HEIGHT * 3);
    if (cam1_frame == NULL || cam2_frame == NULL) {
        fprintf(stderr, "Failed to allocate frame buffers\n");
        free(cam1_frame);
        free(cam2_frame);
        camera_thread_stop(&cam2);
        camera_thread_stop(&cam1);
        return 1;
    }

    HandDetector detector;
    if (hand_init(&detector, CAM1_WIDTH, CAM1_HEIGHT) != 0) {
        fprintf(stderr, "Failed to init HandDetector\n");
        free(cam1_frame);
        free(cam2_frame);
        camera_thread_stop(&cam2);
        camera_thread_stop(&cam1);
        return 1;
    }

    PhotorukaGUI gui;
    if (gui_init(&gui, WIN_WIDTH, WIN_HEIGHT,
                 CAM1_WIDTH, CAM1_HEIGHT,
                 CAM2_WIDTH, CAM2_HEIGHT) != 0) {
        fprintf(stderr, "Failed to init GUI\n");
        hand_free(&detector);
        free(cam1_frame);
        free(cam2_frame);
        camera_thread_stop(&cam2);
        camera_thread_stop(&cam1);
        return 1;
    }

    MarkerTracker tracker;
    tracker_init(&tracker, 15);

    unsigned long last_cam1_count = 0;
    Motion hand = {0};
    Direction direction = DIRECTION_STOP;

    printf("PHOTORUKA GUI started.\n");
    printf("Controls: [M] Mask | [S] Swap cameras | [R] Auto/Manual | [ESC] Exit\n");
    fflush(stdout);

    while (g_running) {
        int cam1_ok = 0, cam2_ok = 0;
        unsigned long cam1_cnt = 0, cam2_cnt = 0;
        char cam1_err[128] = {0}, cam2_err[128] = {0};

        int has_cam1 = camera_thread_get_frame(
            &cam1, cam1_frame, &cam1_ok, &cam1_cnt, cam1_err, sizeof(cam1_err)
        );
        int has_cam2 = camera_thread_get_frame(
            &cam2, cam2_frame, &cam2_ok, &cam2_cnt, cam2_err, sizeof(cam2_err)
        );

        if (has_cam1 && cam1_ok) {
            if (cam1_cnt != last_cam1_count) {
                last_cam1_count = cam1_cnt;
                hand = hand_detect(&detector, cam1_frame);
                Marker target = { hand.found, hand.x, hand.y, hand.pixels };
                direction = tracker_update(&tracker, target);
            }
        } else {
            hand = (Motion){0};
            direction = DIRECTION_STOP;
        }

        AppStatus status;
        memset(&status, 0, sizeof(status));
        status.cam1_connected = cam1_ok;
        status.cam2_connected = cam2_ok;
        status.esp32_connected = 0; /* Заглушка до інтеграції з модулем P5 */
        status.mode = gui.mode;

        if (!cam1_ok && cam1_err[0] != '\0') {
            snprintf(status.last_error, sizeof(status.last_error), "CAM1: %s", cam1_err);
        } else if (!cam2_ok && cam2_err[0] != '\0') {
            snprintf(status.last_error, sizeof(status.last_error), "CAM2: %s", cam2_err);
        }

        if (gui_update(&gui, cam1_frame, has_cam1, cam2_frame, has_cam2,
                       &detector, hand, direction, &status) != 0) {
            break;
        }

        SDL_Delay(16);
    }

    printf("Stopping PHOTORUKA...\n");
    fflush(stdout);

    gui_free(&gui);
    hand_free(&detector);
    free(cam1_frame);
    free(cam2_frame);
    camera_thread_stop(&cam2);
    camera_thread_stop(&cam1);

    return 0;
}