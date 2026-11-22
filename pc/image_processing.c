#include <stddef.h>
#include "image_processing.h"

Marker find_green_marker(const unsigned char *frame, int width, int height)
{
    Marker marker = {0, 0, 0, 0};

    if (frame == NULL || width <= 0 || height <= 0) {
        return marker;
    }

    long long sum_x = 0;
    long long sum_y = 0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = (y * width + x) * 3;

            unsigned char r = frame[index];
            unsigned char g = frame[index + 1];
            unsigned char b = frame[index + 2];

            if (g > 100 && g > r * 1.4 && g > b * 1.3) {
                sum_x += x;
                sum_y += y;
                marker.pixels++;
            }
        }
    }

    if (marker.pixels >= 100) {
        marker.found = 1;
        marker.x = (int)(sum_x / marker.pixels);
        marker.y = (int)(sum_y / marker.pixels);
    }

    return marker;
}
