
#include "marker_tracking.h"

#include <math.h>

void tracker_init(MarkerTracker *tracker, int threshold) {
    tracker->smooth_x = 0.0f;
    tracker->smooth_y = 0.0f;
    tracker->reference_x = 0.0f;
    tracker->reference_y = 0.0f;
    tracker->initialized = 0;
    tracker->frame_count = 0;
    tracker->threshold = threshold;
    tracker->interval = 10;
    tracker->direction = DIRECTION_STOP;
}

Direction tracker_update(MarkerTracker *tracker, Marker marker) {
    if (!marker.found) {
        tracker->initialized = 0;
        tracker->frame_count = 0;
        tracker->direction = DIRECTION_STOP;
        return DIRECTION_STOP;
    }

    if (!tracker->initialized) {
        tracker->smooth_x = (float)marker.x;
        tracker->smooth_y = (float)marker.y;
        tracker->reference_x = tracker->smooth_x;
        tracker->reference_y = tracker->smooth_y;
        tracker->initialized = 1;
        return DIRECTION_STOP;
    }

    const float alpha = 0.3f;

    tracker->smooth_x =
        alpha * marker.x + (1.0f - alpha) * tracker->smooth_x;

    tracker->smooth_y =
        alpha * marker.y + (1.0f - alpha) * tracker->smooth_y;

    tracker->frame_count++;

    if (tracker->frame_count < tracker->interval) {
        return tracker->direction;
    }

    float dx = tracker->smooth_x - tracker->reference_x;
    float dy = tracker->smooth_y - tracker->reference_y;

    tracker->reference_x = tracker->smooth_x;
    tracker->reference_y = tracker->smooth_y;
    tracker->frame_count = 0;

    if (fabsf(dx) < tracker->threshold &&
        fabsf(dy) < tracker->threshold) {
        tracker->direction = DIRECTION_STOP;
    } else if (fabsf(dx) >= fabsf(dy)) {
        tracker->direction =
            dx > 0 ? DIRECTION_LEFT : DIRECTION_RIGHT;
    } else {
        tracker->direction =
            dy > 0 ? DIRECTION_DOWN : DIRECTION_UP;
    }

    return tracker->direction;
}

const char *direction_to_string(Direction direction) {
    switch (direction) {
        case DIRECTION_LEFT:
            return "LEFT";
        case DIRECTION_RIGHT:
            return "RIGHT";
        case DIRECTION_UP:
            return "UP";
        case DIRECTION_DOWN:
            return "DOWN";
        default:
            return "STOP";
    }
}
