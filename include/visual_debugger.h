
#ifndef VISUAL_DEBUGGER_H
#define VISUAL_DEBUGGER_H

#include <SDL2/SDL.h>

#include "hand_detection.h"
#include "marker_tracking.h"

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;

    unsigned char *mask_frame;

    int width;
    int height;
    int show_mask;

} VisualDebugger;

int visual_init(
    VisualDebugger *debugger,
    int width,
    int height
);

int visual_update(
    VisualDebugger *debugger,
    const unsigned char *frame,
    const HandDetector *detector,
    Motion hand,
    Direction direction
);

void visual_free(VisualDebugger *debugger);

#endif
