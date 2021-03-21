#define _DEFAULT_SOURCE

#include <SDL2/SDL.h>
#include <unistd.h>

#include "utils.h"
#include "gui.h"
#include "lights.h"

typedef struct Light Light;

struct Light {
    bool on;
    int r;
    int g;
    int b;
};

enum {
    FPS_CAP           = 30,
    LIGHTS_SIZE       = 3,
    LIGHT_HGAP        = 10,
    LIGHT_VGAP        = 20,
    LIGHT_RAD         = 100,
    LIGHT_OPACITY     = 128,
    LIGHT_OPACITY_ALT = 16,
    WIN_WIDTH         = 2 * LIGHT_RAD + 2 * LIGHT_HGAP,
    WIN_HEIGHT        = LIGHT_VGAP + LIGHTS_SIZE * (2 * LIGHT_RAD + LIGHT_VGAP),
    ON_KEY            = '1',
    OFF_KEY           = '0'
};

static void init(void);
static void redraw(void);
static void fill_circle(int, int, int);
static void draw8(int, int, int, int);
static void handle_event(SDL_Event);

static SDL_Window* win = NULL;
static SDL_Renderer* ren = NULL;
static bool running = false;
static Light lights[LIGHTS_SIZE] = {
    (Light) {.on = false, .r = 255, .g = 0,   .b = 0},
    (Light) {.on = false, .r = 255, .g = 191, .b = 0},
    (Light) {.on = false, .r = 0,   .g = 255, .b = 0}
};

void gui_mainloop(void) {
    init();

    running = true;

    while (running) {
        SDL_Event e;

        while (SDL_PollEvent(&e)) {
            handle_event(e);
        }

        redraw();

        SDL_Delay((1.0 / (float)FPS_CAP) * 1000);
    }
}

void handle_event(SDL_Event e) {
    if (e.type == SDL_QUIT) {
        running = false;
    } else if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
            case ON_KEY:  lights_turn_on(); break;
            case OFF_KEY: lights_turn_off(); break;
        }
    }
}

void gui_destroy(void) {
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
}

void gui_set_light(GUILight light, bool on) {
    lights[light].on = on;
}

void gui_reset_lights(void) {
    for (int i = 0; i < LIGHTS_SIZE; i++) {
        lights[i].on = false;
    }
}

static void redraw(void) {
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 1);
    SDL_RenderClear(ren);
    
    int x = WIN_WIDTH / 2;

    for (int i = 0, y = LIGHT_RAD + LIGHT_VGAP; i < LIGHTS_SIZE; 
         i++, y += 2 * LIGHT_RAD + LIGHT_VGAP) {
        Light l = lights[i];
        int opacity = l.on ? LIGHT_OPACITY : LIGHT_OPACITY_ALT;

        SDL_SetRenderDrawColor(ren, l.r, l.g, l.b, opacity);
        fill_circle(x, y, LIGHT_RAD);
    }
   
    SDL_RenderPresent(ren);
}

// Adapted from: www.mindcontrol.org/~hplus/graphics/RasterizeCircle.html
static void fill_circle(int cx, int cy, int r) {
    int x = 0;
    int y = r;
    int d = -r / 2;

    while (x <= y) {
        draw8(cx, cy, x, y);

        if (d <= 0) {
            x++;
            d += x;
        } else {
            y--;
            d -= y;
        }
    }
}

static void draw8(int ox, int oy, int px, int py) {
    SDL_RenderDrawLine(ren, ox - px, oy - py, ox + px, oy + py);
    SDL_RenderDrawLine(ren, ox - py, oy - px, ox + py, oy + px);
    SDL_RenderDrawLine(ren, ox + py, oy - px, ox - py, oy + px);
    SDL_RenderDrawLine(ren, ox + px, oy - py, ox - px, oy + py);
    SDL_RenderDrawLine(ren, ox - px, oy + py, ox + px, oy - py);
    SDL_RenderDrawLine(ren, ox - py, oy + px, ox + py, oy - px);
    SDL_RenderDrawLine(ren, ox + py, oy + px, ox - py, oy - px);
    SDL_RenderDrawLine(ren, ox + px, oy + py, ox - px, oy - py);
}

static void init(void) {
    win = SDL_CreateWindow("Lights", SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED, WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_SHOWN);
            
    if (win == NULL) {
        DIE("SDL Error\n");
    }

    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    if (ren == NULL) {
        DIE("SDL Error\n");
    }

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
}

