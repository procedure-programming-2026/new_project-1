
#include "visual_debugger.h"

#include <stdio.h>
#include <stdlib.h>

int visual_init(
    VisualDebugger *debugger,
    int width,
    int height
) {
    *debugger = (VisualDebugger){0};

    debugger->width = width;
    debugger->height = height;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
        return 1;
    }

    debugger->window = SDL_CreateWindow(
        "PHOTORUKA - Hand Tracking",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN
    );

    if (debugger->window == NULL) {
        visual_free(debugger);
        return 1;
    }

    debugger->renderer = SDL_CreateRenderer(
        debugger->window,
        -1,
        SDL_RENDERER_SOFTWARE
    );

    if (debugger->renderer == NULL) {
        visual_free(debugger);
        return 1;
    }

    debugger->texture = SDL_CreateTexture(
        debugger->renderer,
        SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );

    if (debugger->texture == NULL) {
        visual_free(debugger);
        return 1;
    }

    debugger->mask_frame =
        malloc((size_t)width * height * 3);

    if (debugger->mask_frame == NULL) {
        visual_free(debugger);
        return 1;
    }

    return 0;
}

int visual_update(
    VisualDebugger *debugger,
    const unsigned char *frame,
    const HandDetector *detector,
    Motion hand,
    Direction direction
) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {

        if (event.type == SDL_QUIT) {
            return 1;
        }

        if (event.type == SDL_KEYDOWN) {

            if (event.key.keysym.sym == SDLK_ESCAPE) {
                return 1;
            }

            if (event.key.keysym.sym == SDLK_m &&
                !event.key.repeat) {

                debugger->show_mask =
                    !debugger->show_mask;
            }
        }
    }

    const unsigned char *display_frame = frame;

    if (debugger->show_mask) {

        int total =
            debugger->width * debugger->height;

        for (int i = 0; i < total; i++) {

            unsigned char value =
                detector->mask[i] ? 255 : 0;

            debugger->mask_frame[i * 3] = value;
            debugger->mask_frame[i * 3 + 1] = value;
            debugger->mask_frame[i * 3 + 2] = value;
        }

        display_frame = debugger->mask_frame;
    }

    if (SDL_UpdateTexture(
        debugger->texture,
        NULL,
        display_frame,
        debugger->width * 3
    ) != 0) {
        fprintf(stderr, "Texture Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetRenderDrawColor(
        debugger->renderer, 0, 0, 0, 255
    );

    SDL_RenderClear(debugger->renderer);

    SDL_RenderCopyEx(
        debugger->renderer,
        debugger->texture,
        NULL,
        NULL,
        0.0,
        NULL,
        SDL_FLIP_HORIZONTAL
    );

    char title[256];

    if (hand.found) {

        int mirrored_x =
            debugger->width - 1 - hand.x;

        SDL_Rect rectangle = {
            debugger->width - 1 - detector->max_x,
            detector->min_y,
            detector->max_x - detector->min_x + 1,
            detector->max_y - detector->min_y + 1
        };

        SDL_SetRenderDrawColor(
            debugger->renderer,
            0, 255, 100, 255
        );

        SDL_RenderDrawRect(
            debugger->renderer,
            &rectangle
        );

        SDL_RenderDrawLine(
            debugger->renderer,
            mirrored_x - 10,
            hand.y,
            mirrored_x + 10,
            hand.y
        );

        SDL_RenderDrawLine(
            debugger->renderer,
            mirrored_x,
            hand.y - 10,
            mirrored_x,
            hand.y + 10
        );

        snprintf(
            title,
            sizeof(title),
            "PHOTORUKA | %s | X:%d Y:%d | Pixels:%d | %s",
            debugger->show_mask ? "MASK" : "CAMERA",
            mirrored_x,
            hand.y,
            hand.pixels,
            direction_to_string(direction)
        );

    } else {

        snprintf(
            title,
            sizeof(title),
            "PHOTORUKA | %s | No candidate | STOP",
            debugger->show_mask ? "MASK" : "CAMERA"
        );
    }

    SDL_SetWindowTitle(debugger->window, title);

    SDL_RenderPresent(debugger->renderer);

    return 0;
}

void visual_free(VisualDebugger *debugger) {

    free(debugger->mask_frame);

    if (debugger->texture != NULL) {
        SDL_DestroyTexture(debugger->texture);
    }

    if (debugger->renderer != NULL) {
        SDL_DestroyRenderer(debugger->renderer);
    }

    if (debugger->window != NULL) {
        SDL_DestroyWindow(debugger->window);
    }

    SDL_Quit();

    *debugger = (VisualDebugger){0};
}
