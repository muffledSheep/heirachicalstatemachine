#pragma once

#include <stdbool.h>

typedef enum GUILight GUILight;

enum GUILight { RED, AMBER, GREEN };

void gui_mainloop(void);

void gui_destroy(void);

void gui_set_light(GUILight, bool);

void gui_reset_lights(void);
