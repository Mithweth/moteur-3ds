// ui.h
// Draws the common frame, room messages and examine-image overlays.
#pragma once

#include <citro2d.h>
#include <stdbool.h>

void ui_draw_button(const char *label_id, float x, float y, bool selected);
void ui_draw_frame(float x, float y, float w, float h, float z);
void ui_draw_message(void);
void ui_show_message(const char *message_id);
void ui_show_image(C2D_Image image);
void ui_message_clear_text(void);
void ui_message_clear_image(void);
void ui_begin_frame(void);
bool ui_init(void);
void ui_close(void);
