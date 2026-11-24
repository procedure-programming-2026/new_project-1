
#ifndef SKIN_DETECTION_H
#define SKIN_DETECTION_H

#include "motion_detection.h"

typedef struct {
    unsigned char *background;
    int width;
    int height;
    int initialized;
    int motion_threshold;
    int min_pixels;
} SkinDetector;

int skin_init(SkinDetector *detector, int width, int height);
Motion skin_detect(SkinDetector *detector, const unsigned char *frame);
void skin_free(SkinDetector *detector);

#endif
