
#include "skin_detection.h"

#include <stdlib.h>
#include <string.h>

static int is_skin(int r, int g, int b) {
    float cb = 128.0f - 0.168736f * r
                        - 0.331264f * g
                        + 0.5f * b;

    float cr = 128.0f + 0.5f * r
                        - 0.418688f * g
                        - 0.081312f * b;

    int rgb_match =
        r > 60 &&
        g > 30 &&
        b > 15 &&
        r > g &&
        r > b &&
        abs(r - g) > 10;

    int ycbcr_match =
        cb >= 77.0f &&
        cb <= 135.0f &&
        cr >= 133.0f &&
        cr <= 180.0f;

    return rgb_match && ycbcr_match;
}

int skin_init(SkinDetector *detector, int width, int height) {
    if (detector == NULL || width <= 0 || height <= 0) {
        return 1;
    }

    detector->width = width;
    detector->height = height;
    detector->initialized = 0;
    detector->motion_threshold = 35;
    detector->min_pixels = 400;

    detector->background = malloc((size_t)width * height * 3);

    return detector->background == NULL ? 1 : 0;
}

Motion skin_detect(SkinDetector *detector, const unsigned char *frame) {
    Motion result = {0, 0, 0, 0};

    if (detector == NULL || frame == NULL ||
        detector->background == NULL) {
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
            size_t index = ((size_t)y * width + x) * 3;

            int r = frame[index];
            int g = frame[index + 1];
            int b = frame[index + 2];

            int dr = abs(r - (int)detector->background[index]);
            int dg = abs(g - (int)detector->background[index + 1]);
            int db = abs(b - (int)detector->background[index + 2]);

            int difference = (dr + dg + db) / 3;

            if (difference > detector->motion_threshold &&
                is_skin(r, g, b)) {
                sum_x += x;
                sum_y += y;
                count++;
            }
        }
    }

    if (count >= detector->min_pixels) {
        result.found = 1;
        result.x = (int)(sum_x / count);
        result.y = (int)(sum_y / count);
        result.pixels = count;
    }

    return result;
}

void skin_free(SkinDetector *detector) {
    if (detector != NULL) {
        free(detector->background);
        detector->background = NULL;
        detector->initialized = 0;
    }
}
