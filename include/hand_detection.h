
#ifndef HAND_DETECTION_H
#define HAND_DETECTION_H

#include "motion_detection.h"

typedef struct {
    int width;
    int height;
    int initialized;

    unsigned char *background;
    unsigned char *mask;
    unsigned char *temp_mask;

    int *queue;

    int min_x;
    int min_y;
    int max_x;
    int max_y;

} HandDetector;

int hand_init(
    HandDetector *detector,
    int width,
    int height
);

Motion hand_detect(
    HandDetector *detector,
    const unsigned char *frame
);

void hand_free(HandDetector *detector);

#endif
