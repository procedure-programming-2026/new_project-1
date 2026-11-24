
#include "motion_detection.h"

#include <stdlib.h>
#include <string.h>

int motion_init(MotionDetector *detector, int width, int height, int threshold) {
    if (detector == NULL || width <= 0 || height <= 0 || threshold < 0) {
        return 1;
    }

    detector->width = width;
    detector->height = height;
    detector->threshold = threshold;
    detector->initialized = 0;
    detector->background = malloc((size_t)width * height * 3);

    return detector->background == NULL ? 1 : 0;
}

Motion motion_detect(MotionDetector *detector, const unsigned char *frame) {
    Motion result = {0, 0, 0, 0};

    if (detector == NULL || frame == NULL || detector->background == NULL) {
        return result;
    }

    int width = detector->width;
    int height = detector->height;
    size_t size = (size_t)width * height * 3;

    if (!detector->initialized) {
        memcpy(detector->background, frame, size);
        detector->initialized = 1;
        return result;
    }

    long sum_x = 0;
    long sum_y = 0;
    int count = 0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = (y * width + x) * 3;

            int r = abs((int)frame[index] - detector->background[index]);
            int g = abs((int)frame[index + 1] - detector->background[index + 1]);
            int b = abs((int)frame[index + 2] - detector->background[index + 2]);

            int difference = (r + g + b) / 3;

            if (difference > detector->threshold) {
                sum_x += x;
                sum_y += y;
                count++;
            }
        }
    }

    if (count >= 500) {
        result.found = 1;
        result.x = (int)(sum_x / count);
        result.y = (int)(sum_y / count);
        result.pixels = count;
    }

    return result;
}

void motion_free(MotionDetector *detector) {
    if (detector != NULL) {
        free(detector->background);
        detector->background = NULL;
        detector->initialized = 0;
    }
}
