#include "gui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static TTF_Font *open_system_font(int pt_size)
{
    static const char *font_paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-B.ttf"
    };

    size_t count = sizeof(font_paths) / sizeof(font_paths[0]);
    for (size_t i = 0; i < count; i++) {
        TTF_Font *f = TTF_OpenFont(font_paths[i], pt_size);
        if (f != NULL) {
            return f;
        }
    }

    return NULL;
}

static void draw_text(
    SDL_Renderer *renderer,
    TTF_Font *font,
    const char *text,
    int x,
    int y,
    SDL_Color color
) {
    if (font == NULL || text == NULL || text[0] == '\0') {
        return;
    }

    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
    if (surface == NULL) {
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture != NULL) {
        SDL_Rect dst = { x, y, surface->w, surface->h };
        SDL_RenderCopy(renderer, texture, NULL, &dst);
        SDL_DestroyTexture(texture);
    }

    SDL_FreeSurface(surface);
}

static void draw_placeholder(
    SDL_Renderer *renderer,
    TTF_Font *font,
    SDL_Rect rect,
    const char *label
) {
    SDL_SetRenderDrawColor(renderer, 25, 28, 34, 255);
    SDL_RenderFillRect(renderer, &rect);

    SDL_SetRenderDrawColor(renderer, 70, 75, 85, 255);
    SDL_RenderDrawRect(renderer, &rect);

    if (font != NULL && label != NULL) {
        int tw = 0, th = 0;
        TTF_SizeUTF8(font, label, &tw, &th);
        SDL_Color gray = { 180, 185, 195, 255 };
        draw_text(
            renderer,
            font,
            label,
            rect.x + (rect.w - tw) / 2,
            rect.y + (rect.h - th) / 2,
            gray
        );
    }
}

static void draw_hand_overlay(
    SDL_Renderer *renderer,
    SDL_Rect dst_rect,
    int cam_w,
    int cam_h,
    const HandDetector *detector,
    Motion hand
) {
    if (!hand.found || detector == NULL) {
        return;
    }

    float sx = (float)dst_rect.w / (float)cam_w;
    float sy = (float)dst_rect.h / (float)cam_h;

    int mirrored_x = cam_w - 1 - hand.x;
    int box_x = cam_w - 1 - detector->max_x;
    int box_y = detector->min_y;
    int box_w = detector->max_x - detector->min_x + 1;
    int box_h = detector->max_y - detector->min_y + 1;

    SDL_Rect rect = {
        dst_rect.x + (int)(box_x * sx),
        dst_rect.y + (int)(box_y * sy),
        (int)(box_w * sx),
        (int)(box_h * sy)
    };

    int cx = dst_rect.x + (int)(mirrored_x * sx);
    int cy = dst_rect.y + (int)(hand.y * sy);
    int cross = (dst_rect.w >= 640) ? 12 : 7;

    SDL_SetRenderDrawColor(renderer, 0, 255, 100, 255);
    SDL_RenderDrawRect(renderer, &rect);
    SDL_RenderDrawLine(renderer, cx - cross, cy, cx + cross, cy);
    SDL_RenderDrawLine(renderer, cx, cy - cross, cx, cy + cross);
}

static void render_cam1(
    PhotorukaGUI *gui,
    const unsigned char *cam1_frame,
    int has_cam1,
    int cam1_connected,
    const HandDetector *detector,
    Motion hand,
    SDL_Rect dst_rect
) {
    if (!has_cam1 || !cam1_connected || cam1_frame == NULL) {
        draw_placeholder(gui->renderer, gui->font_small, dst_rect, "CAM1: NO SIGNAL");
        return;
    }

    const unsigned char *display_frame = cam1_frame;

    if (gui->show_mask && detector != NULL && detector->mask != NULL) {
        int total = gui->cam1_w * gui->cam1_h;
        for (int i = 0; i < total; i++) {
            unsigned char v = detector->mask[i] ? 255 : 0;
            gui->mask_frame[i * 3]     = v;
            gui->mask_frame[i * 3 + 1] = v;
            gui->mask_frame[i * 3 + 2] = v;
        }
        display_frame = gui->mask_frame;
    }

    SDL_UpdateTexture(gui->cam1_texture, NULL, display_frame, gui->cam1_w * 3);
    SDL_RenderCopyEx(
        gui->renderer,
        gui->cam1_texture,
        NULL,
        &dst_rect,
        0.0,
        NULL,
        SDL_FLIP_HORIZONTAL
    );

    draw_hand_overlay(gui->renderer, dst_rect, gui->cam1_w, gui->cam1_h, detector, hand);
}

static void render_cam2(
    PhotorukaGUI *gui,
    const unsigned char *cam2_frame,
    int has_cam2,
    int cam2_connected,
    SDL_Rect dst_rect
) {
    if (!has_cam2 || !cam2_connected || cam2_frame == NULL) {
        draw_placeholder(gui->renderer, gui->font, dst_rect, "CAM2 (ROBOT): NO SIGNAL");
        return;
    }

    SDL_UpdateTexture(gui->cam2_texture, NULL, cam2_frame, gui->cam2_w * 3);
    SDL_RenderCopy(gui->renderer, gui->cam2_texture, NULL, &dst_rect);
}

static void draw_hud(
    PhotorukaGUI *gui,
    Motion hand,
    Direction direction,
    const AppStatus *status
) {
    SDL_Color white  = { 240, 244, 248, 255 };
    SDL_Color green  = { 80,  235, 130, 255 };
    SDL_Color red    = { 255, 95,  95,  255 };
    SDL_Color yellow = { 255, 210, 75,  255 };
    SDL_Color cyan   = { 90,  215, 255, 255 };
    SDL_Color muted  = { 175, 185, 195, 255 };

    SDL_SetRenderDrawBlendMode(gui->renderer, SDL_BLENDMODE_BLEND);

    /* Головна панель статусів зліва зверху */
    SDL_Rect panel = { 16, 16, 360, 200 };
    SDL_SetRenderDrawColor(gui->renderer, 12, 16, 22, 185);
    SDL_RenderFillRect(gui->renderer, &panel);
    SDL_SetRenderDrawColor(gui->renderer, 80, 95, 115, 220);
    SDL_RenderDrawRect(gui->renderer, &panel);

    int x = panel.x + 14;
    int y = panel.y + 12;
    int step = 26;
    char line[160];

    /* Заголовок і режим */
    draw_text(gui->renderer, gui->font, "PHOTORUKA", x, y, cyan);
    draw_text(
        gui->renderer,
        gui->font,
        status->mode == MODE_AUTO ? "[AUTO]" : "[MANUAL]",
        x + 155,
        y,
        status->mode == MODE_AUTO ? green : yellow
    );
    y += step + 6;

    /* Статус камер */
    snprintf(line, sizeof(line), "CAM1 (Laptop): %s",
             status->cam1_connected ? "CONNECTED" : "DISCONNECTED");
    draw_text(gui->renderer, gui->font_small, line, x, y,
              status->cam1_connected ? green : red);
    y += step;

    snprintf(line, sizeof(line), "CAM2 (Robot):  %s",
             status->cam2_connected ? "CONNECTED" : "DISCONNECTED");
    draw_text(gui->renderer, gui->font_small, line, x, y,
              status->cam2_connected ? green : red);
    y += step;

    /* Статус ESP32 (заглушка до інтеграції P5) */
    snprintf(line, sizeof(line), "ESP32:         %s",
             status->esp32_connected ? "CONNECTED" : "DISCONNECTED");
    draw_text(gui->renderer, gui->font_small, line, x, y,
              status->esp32_connected ? green : yellow);
    y += step;

    /* Статус долоні */
    if (hand.found) {
        int mx = gui->cam1_w - 1 - hand.x;
        snprintf(line, sizeof(line), "HAND: DETECTED (X:%d Y:%d)", mx, hand.y);
        draw_text(gui->renderer, gui->font_small, line, x, y, green);
    } else {
        draw_text(gui->renderer, gui->font_small, "HAND: NOT FOUND", x, y, muted);
    }
    y += step;

    /* Напрямок руху */
    if (status->mode == MODE_MANUAL) {
        snprintf(line, sizeof(line), "DIR:  %s (LOCKED)", direction_to_string(direction));
        draw_text(gui->renderer, gui->font, line, x, y, yellow);
    } else {
        snprintf(line, sizeof(line), "DIR:  %s", direction_to_string(direction));
        draw_text(gui->renderer, gui->font, line, x, y, white);
    }

    /* Панель помилки (якщо є) */
    if (status->last_error[0] != '\0') {
        SDL_Rect err_box = { 16, gui->win_h - 86, 540, 32 };
        SDL_SetRenderDrawColor(gui->renderer, 140, 25, 25, 200);
        SDL_RenderFillRect(gui->renderer, &err_box);
        SDL_SetRenderDrawColor(gui->renderer, 255, 95, 95, 230);
        SDL_RenderDrawRect(gui->renderer, &err_box);

        snprintf(line, sizeof(line), "ERROR: %s", status->last_error);
        draw_text(gui->renderer, gui->font_small, line, err_box.x + 10, err_box.y + 7, white);
    }

    /* Підказка гарячих клавіш знизу зліва */
    SDL_Rect hint_box = { 16, gui->win_h - 44, 590, 28 };
    SDL_SetRenderDrawColor(gui->renderer, 12, 16, 22, 185);
    SDL_RenderFillRect(gui->renderer, &hint_box);

    snprintf(
        line,
        sizeof(line),
        "[M] Mask:%s  [S] Swap:%s  [R] Mode:%s  [ESC] Exit",
        gui->show_mask ? "ON" : "OFF",
        gui->swap_cameras ? "CAM1" : "CAM2",
        gui->mode == MODE_AUTO ? "AUTO" : "MANUAL"
    );
    draw_text(gui->renderer, gui->font_small, line, hint_box.x + 10, hint_box.y + 6, muted);
}

int gui_init(
    PhotorukaGUI *gui,
    int win_w,
    int win_h,
    int cam1_w,
    int cam1_h,
    int cam2_w,
    int cam2_h
) {
    memset(gui, 0, sizeof(*gui));

    gui->win_w = win_w;
    gui->win_h = win_h;
    gui->cam1_w = cam1_w;
    gui->cam1_h = cam1_h;
    gui->cam2_w = cam2_w;
    gui->cam2_h = cam2_h;
    gui->mode = MODE_AUTO;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init error: %s\n", TTF_GetError());
        gui_free(gui);
        return 1;
    }

    gui->window = SDL_CreateWindow(
        "PHOTORUKA",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        win_w,
        win_h,
        SDL_WINDOW_SHOWN
    );
    if (gui->window == NULL) {
        fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        gui_free(gui);
        return 1;
    }

    gui->renderer = SDL_CreateRenderer(
        gui->window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (gui->renderer == NULL) {
        gui->renderer = SDL_CreateRenderer(gui->window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (gui->renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer error: %s\n", SDL_GetError());
        gui_free(gui);
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    gui->cam1_texture = SDL_CreateTexture(
        gui->renderer,
        SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_STREAMING,
        cam1_w,
        cam1_h
    );
    gui->cam2_texture = SDL_CreateTexture(
        gui->renderer,
        SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_STREAMING,
        cam2_w,
        cam2_h
    );
    if (gui->cam1_texture == NULL || gui->cam2_texture == NULL) {
        fprintf(stderr, "SDL_CreateTexture error: %s\n", SDL_GetError());
        gui_free(gui);
        return 1;
    }

    gui->mask_frame = malloc((size_t)cam1_w * (size_t)cam1_h * 3);
    if (gui->mask_frame == NULL) {
        gui_free(gui);
        return 1;
    }

    gui->font = open_system_font(20);
    gui->font_small = open_system_font(15);
    if (gui->font == NULL || gui->font_small == NULL) {
        fprintf(stderr, "Warning: system TTF font not found. Install 'fonts-dejavu-core'.\n");
    }

    return 0;
}

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
) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return 1;
        }
        if (event.type == SDL_KEYDOWN && !event.key.repeat) {
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    return 1;
                case SDLK_m:
                    gui->show_mask = !gui->show_mask;
                    break;
                case SDLK_s:
                    gui->swap_cameras = !gui->swap_cameras;
                    break;
                case SDLK_r:
                    gui->mode = (gui->mode == MODE_AUTO) ? MODE_MANUAL : MODE_AUTO;
                    break;
                default:
                    break;
            }
        }
    }

    SDL_SetRenderDrawColor(gui->renderer, 15, 18, 22, 255);
    SDL_RenderClear(gui->renderer);

    SDL_Rect main_rect = { 0, 0, gui->win_w, gui->win_h };
    SDL_Rect pip_rect  = { gui->win_w - 320 - 16, gui->win_h - 240 - 16, 320, 240 };

    if (!gui->swap_cameras) {
        render_cam2(gui, cam2_frame, has_cam2, status->cam2_connected, main_rect);
        render_cam1(gui, cam1_frame, has_cam1, status->cam1_connected, detector, hand, pip_rect);
    } else {
        render_cam1(gui, cam1_frame, has_cam1, status->cam1_connected, detector, hand, main_rect);
        render_cam2(gui, cam2_frame, has_cam2, status->cam2_connected, pip_rect);
    }

    /* Рамка навколо маленького вікна справа знизу */
    SDL_SetRenderDrawColor(gui->renderer, 90, 215, 255, 255);
    SDL_RenderDrawRect(gui->renderer, &pip_rect);
    SDL_Rect pip_border = { pip_rect.x - 1, pip_rect.y - 1, pip_rect.w + 2, pip_rect.h + 2 };
    SDL_RenderDrawRect(gui->renderer, &pip_border);

    draw_hud(gui, hand, direction, status);

    char title[256];
    snprintf(
        title,
        sizeof(title),
        "PHOTORUKA | %s | CAM1:%s CAM2:%s | ESP32:%s | DIR:%s",
        status->mode == MODE_AUTO ? "AUTO" : "MANUAL",
        status->cam1_connected ? "OK" : "OFF",
        status->cam2_connected ? "OK" : "OFF",
        status->esp32_connected ? "OK" : "OFF",
        direction_to_string(direction)
    );
    SDL_SetWindowTitle(gui->window, title);

    SDL_RenderPresent(gui->renderer);
    return 0;
}

void gui_free(PhotorukaGUI *gui)
{
    free(gui->mask_frame);

    if (gui->font != NULL) {
        TTF_CloseFont(gui->font);
    }
    if (gui->font_small != NULL) {
        TTF_CloseFont(gui->font_small);
    }
    if (gui->cam1_texture != NULL) {
        SDL_DestroyTexture(gui->cam1_texture);
    }
    if (gui->cam2_texture != NULL) {
        SDL_DestroyTexture(gui->cam2_texture);
    }
    if (gui->renderer != NULL) {
        SDL_DestroyRenderer(gui->renderer);
    }
    if (gui->window != NULL) {
        SDL_DestroyWindow(gui->window);
    }

    TTF_Quit();
    SDL_Quit();

    memset(gui, 0, sizeof(*gui));
}