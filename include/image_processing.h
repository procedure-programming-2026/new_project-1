
#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H

typedef struct {
    int found;
    int x;
    int y;
    int pixels;
} Marker;

Marker find_green_marker(const unsigned char *frame, int width, int height);

#endif
