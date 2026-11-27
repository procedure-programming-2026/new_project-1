
#ifndef MOTION_DETECTION_H
#define MOTION_DETECTION_H

typedef struct {
    int found;
    int x;
    int y;
    int pixels;
    int grip;
} Motion;

typedef struct {
    unsigned char *background;
    int width;
    int height;
    int initialized;
    int threshold;
} MotionDetector;

int motion_init(MotionDetector *detector, int width, int height, int threshold);
Motion motion_detect(MotionDetector *detector, const unsigned char *frame);
void motion_free(MotionDetector *detector);

#endif
