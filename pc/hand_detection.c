
#include "hand_detection.h"
#include <stdlib.h>
#include <string.h>

#define MOTION_THRESHOLD 20
#define MIN_PIXELS 2500
#define MORPH_RADIUS 2

static int is_skin(int r, int g, int b) {

    float cb = 128.0f
        - 0.168736f * r
        - 0.331264f * g
        + 0.5f * b;

    float cr = 128.0f
        + 0.5f * r
        - 0.418688f * g
        - 0.081312f * b;

    return r > 60 &&
           g > 30 &&
           b > 15 &&
           r > g &&
           r > b &&
           cb >= 77.0f &&
           cb <= 135.0f &&
           cr >= 133.0f &&
           cr <= 180.0f;
}

static void dilate(
    const unsigned char *source,
    unsigned char *destination,
    int width,
    int height
) {
    for (int y = 0; y < height; y++) {

        for (int x = 0; x < width; x++) {

            int value = 0;

            for (int dy = -MORPH_RADIUS;
                 dy <= MORPH_RADIUS && !value;
                 dy++) {

                for (int dx = -MORPH_RADIUS;
                     dx <= MORPH_RADIUS;
                     dx++) {

                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx < 0 || nx >= width ||
                        ny < 0 || ny >= height) {
                        continue;
                    }

                    if (source[ny * width + nx]) {
                        value = 1;
                        break;
                    }
                }
            }

            destination[y * width + x] = value;
        }
    }
}

static void erode(
    const unsigned char *source,
    unsigned char *destination,
    int width,
    int height
) {
    for (int y = 0; y < height; y++) {

        for (int x = 0; x < width; x++) {

            int value = 1;

            for (int dy = -MORPH_RADIUS;
                 dy <= MORPH_RADIUS && value;
                 dy++) {

                for (int dx = -MORPH_RADIUS;
                     dx <= MORPH_RADIUS;
                     dx++) {

                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx < 0 || nx >= width ||
                        ny < 0 || ny >= height ||
                        !source[ny * width + nx]) {

                        value = 0;
                        break;
                    }
                }
            }

            destination[y * width + x] = value;
        }
    }
}

static void morphological_closing(
    HandDetector *detector
) {
    dilate(
        detector->mask,
        detector->temp_mask,
        detector->width,
        detector->height
    );

    erode(
        detector->temp_mask,
        detector->mask,
        detector->width,
        detector->height
    );
}

/* Analyze finger-like segments */

static int finger_rows(
    const unsigned char *mask,
    int width,
    int min_x,
    int min_y,
    int max_x,
    int max_y
) {
    int w = max_x - min_x + 1;
    int h = max_y - min_y + 1;

    int min_run = w / 20;

    if (min_run < 3) {
        min_run = 3;
    }

    int votes = 0;

    for (int k = 1; k <= 9; k++) {

        int y = min_y + h * k / 20;

        int runs = 0;
        int previous_end = -1;
        int x = min_x;

        while (x <= max_x) {

            if (!mask[y * width + x]) {
                x++;
                continue;
            }

            int start = x;

            while (x <= max_x &&
                   mask[y * width + x]) {
                x++;
            }

            if (x - start < min_run) {
                continue;
            }

            if (previous_end < 0 ||
                start - previous_end >= min_run) {
                runs++;
            }

            previous_end = x;
        }

        if (runs >= 3) {
            votes++;
        }
    }

    return votes;
}

int hand_init(
    HandDetector *detector,
    int width,
    int height
) {
    if (detector == NULL ||
        width <= 0 || height <= 0) {
        return 1;
    }

    *detector = (HandDetector){0};

    detector->width = width;
    detector->height = height;

    size_t pixels = (size_t)width * height;

    detector->background = malloc(pixels * 3);
    detector->mask = calloc(pixels, 1);
    detector->temp_mask = calloc(pixels, 1);
    detector->queue = malloc(pixels * sizeof(int));

    if (detector->background == NULL ||
        detector->mask == NULL ||
        detector->temp_mask == NULL ||
        detector->queue == NULL) {

        hand_free(detector);
        return 1;
    }

    return 0;
}

Motion hand_detect(
    HandDetector *detector,
    const unsigned char *frame
) {
    Motion result = {0, 0, 0, 0};

    if (detector == NULL || frame == NULL) {
        return result;
    }

    int width = detector->width;
    int height = detector->height;
    int total = width * height;

    detector->min_x = 0;
    detector->min_y = 0;
    detector->max_x = 0;
    detector->max_y = 0;

    if (!detector->initialized) {

        memcpy(
            detector->background,
            frame,
            (size_t)total * 3
        );

        memset(detector->mask, 0, total);

        detector->initialized = 1;

        return result;
    }

    /* Step 1: Create skin and motion mask */

    for (int i = 0; i < total; i++) {

        int index = i * 3;

        int r = frame[index];
        int g = frame[index + 1];
        int b = frame[index + 2];

        int difference = (
            abs(r - detector->background[index]) +
            abs(g - detector->background[index + 1]) +
            abs(b - detector->background[index + 2])
        ) / 3;

        detector->mask[i] =
            difference > MOTION_THRESHOLD &&
            is_skin(r, g, b);
    }

    /* Step 2: Repair fragmented mask */

    morphological_closing(detector);

    memset(
        detector->temp_mask,
        0,
        (size_t)total
    );

    /* Step 3: Find connected components */

    float best_score = 0.0f;

    for (int i = 0; i < total; i++) {

        if (detector->mask[i] != 1) {
            continue;
        }

        int head = 0;
        int tail = 0;

        detector->queue[tail++] = i;
        detector->mask[i] = 2;

        int min_x = i % width;
        int max_x = min_x;

        int min_y = i / width;
        int max_y = min_y;

        long sum_x = 0;
        long sum_y = 0;

        while (head < tail) {

            int current = detector->queue[head++];

            int x = current % width;
            int y = current / width;

            sum_x += x;
            sum_y += y;

            if (x < min_x) min_x = x;
            if (x > max_x) max_x = x;

            if (y < min_y) min_y = y;
            if (y > max_y) max_y = y;

            int neighbors[4] = {
                x > 0 ? current - 1 : -1,
                x < width - 1 ? current + 1 : -1,
                y > 0 ? current - width : -1,
                y < height - 1 ? current + width : -1
            };

            for (int j = 0; j < 4; j++) {

                int next = neighbors[j];

                if (next >= 0 &&
                    detector->mask[next] == 1) {

                    detector->mask[next] = 2;
                    detector->queue[tail++] = next;
                }
            }
        }

        /* Step 4: Filter components */

        int count = tail;

        if (count < MIN_PIXELS ||
            count > total / 3) {
            continue;
        }

        int box_width = max_x - min_x + 1;
        int box_height = max_y - min_y + 1;

        if (box_width < 35 ||
            box_height < 35) {
            continue;
        }

        float ratio =
            (float)box_width / box_height;

        float density =
            (float)count /
            (box_width * box_height);

        if (ratio < 0.35f || ratio > 2.0f) {
            continue;
        }

        if (density < 0.25f || density > 0.85f) {
            continue;
        }

        /* Step 5: Shape analysis */

        for (int j = 0; j < count; j++) {

            detector->temp_mask[
                detector->queue[j]
            ] = 1;
        }

        int votes = finger_rows(
            detector->temp_mask,
            width,
            min_x,
            min_y,
            max_x,
            max_y
        );

        for (int j = 0; j < count; j++) {

            detector->temp_mask[
                detector->queue[j]
            ] = 0;
        }

        if (votes < 3) {
            continue;
        }

        /* Step 6: Select the best candidate */

        float score =
            count * (1.0f + votes / 9.0f);

        if (score > best_score) {

            best_score = score;

            result.found = 1;
            result.x = (int)(sum_x / count);
            result.y = (int)(sum_y / count);
            result.pixels = count;

            detector->min_x = min_x;
            detector->min_y = min_y;
            detector->max_x = max_x;
            detector->max_y = max_y;
        }
    }

    return result;
}

void hand_free(HandDetector *detector) {

    if (detector == NULL) {
        return;
    }

    free(detector->background);
    free(detector->mask);
    free(detector->temp_mask);
    free(detector->queue);

    *detector = (HandDetector){0};
}
