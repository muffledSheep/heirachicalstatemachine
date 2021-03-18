#pragma once

#include <stdbool.h>

typedef enum GUILight { RED, AMBER, GREEN } GUILight;

void gui_mainloop(void);

void gui_destroy(void);

void gui_set_light(GUILight, bool);

void gui_reset_lights(void);
