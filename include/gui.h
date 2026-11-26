#ifndef GUI_H
#define GUI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "hand_detection.h"
#include "marker_tracking.h"

typedef enum {
    MODE_AUTO = 0,
    MODE_MANUAL = 1
} RobotMode;

typedef struct {
    int cam1_connected;
    int cam2_connected;
    int esp32_connected;
    RobotMode mode;
    char last_error[128];
} AppStatus;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *cam1_texture;
    SDL_Texture *cam2_texture;

    TTF_Font *font;
    TTF_Font *font_small;

    unsigned char *mask_frame;

    int win_w;
    int win_h;
    int cam1_w;
    int cam1_h;
    int cam2_w;
    int cam2_h;

    int show_mask;
    int swap_cameras;
    RobotMode mode;
} PhotorukaGUI;

int gui_init(
    PhotorukaGUI *gui,
    int win_w,
    int win_h,
    int cam1_w,
    int cam1_h,
    int cam2_w,
    int cam2_h
);

/* Повертає 0 для продовження роботи або 1, якщо користувач закрив вікно / натиснув ESC */
int gui_update(
    PhotorukaGUI *gui,
    const unsigned char *cam1_frame,
    int has_cam1,
    const unsigned char *cam2_frame,
    int has_cam2,
    const HandDetector *detector,
    Motion hand,
    Direction direction,
    const AppStatus *status
);

void gui_free(PhotorukaGUI *gui);

#endif