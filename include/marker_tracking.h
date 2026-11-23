
#ifndef MARKER_TRACKING_H
#define MARKER_TRACKING_H

#include "image_processing.h"

typedef enum {
    DIRECTION_STOP,
    DIRECTION_LEFT,
    DIRECTION_RIGHT,
    DIRECTION_UP,
    DIRECTION_DOWN
} Direction;

typedef struct {
    float smooth_x;
    float smooth_y;
    float reference_x;
    float reference_y;
    int initialized;
    int frame_count;
    int threshold;
    int interval;
    Direction direction;
} MarkerTracker;

void tracker_init(MarkerTracker *tracker, int threshold);
Direction tracker_update(MarkerTracker *tracker, Marker marker);
const char *direction_to_string(Direction direction);

#endif
