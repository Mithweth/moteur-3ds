// ui.c
// Shared frame graphics and message / examine-image presentation.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui.h"
#include "config.h"
#include "lang.h"
#include "text_render.h"

// --------- MODULE STATE

static char *message_text = NULL;
static C2D_Image examine_image;
static C2D_TextBuf text_buf;
static C2D_Text text;

// --------- GRAPHIC FUNCTIONS

void ui_begin_frame(void) {
    C2D_TextBufClear(text_buf);
}

void ui_draw_button(const char *label_id, float x, float y, bool selected) {
	float width, height;
	C2D_TextParse(&text, text_buf, lang_get(label_id));
	C2D_TextOptimize(&text);
	C2D_TextGetDimensions(&text, config_choice_button_text_size(), config_choice_button_text_size(), &width, &height);
	if (selected) {
		C2D_DrawRectSolid(x - width / 2.0f - config_choice_button_padding(), y - height / 2.0f - config_choice_button_padding(), 0.98f, width + 2.0f * config_choice_button_padding(), height + 2.0f * config_choice_button_padding(), config_choice_button_color());
	}
	C2D_DrawText(&text, C2D_WithColor | C2D_AlignCenter, x, y - height / 2.0f, 0.99f, config_choice_button_text_size(), config_choice_button_text_size(), config_choice_button_text_color());
}

void ui_draw_frame(float x, float y, float w, float h, float z) {
	C2D_DrawRectSolid(x + 3.0f, y + 3.0f, z - 0.01f, w, h, config_frame_shadow());
	C2D_DrawRectSolid(x, y, z, w, h, config_frame_outer_border());
	C2D_DrawRectSolid(x + 2.0f, y + 2.0f, z + 0.01f, w - 4.0f, h - 4.0f, config_frame_outer_background());
	C2D_DrawRectSolid(x + 5.0f, y + 5.0f, z + 0.02f, w - 10.0f, h - 10.0f, config_frame_inner_border());
	C2D_DrawRectSolid(x + 6.0f, y + 6.0f, z + 0.03f, w - 12.0f, h - 12.0f, config_frame_inner_background());
}

// The message may be either wrapped text or an examine image.
void ui_draw_message(void) {
	if (examine_image.tex) {
		float x = (320 - examine_image.subtex->width) / 2;
		float y = (240 - examine_image.subtex->height) / 2;
		C2D_DrawRectSolid(0.0f, 0.0f, 0.8f, 320, 240, C2D_Color32(0, 0, 0, 180));
		C2D_DrawImageAt(examine_image, x, y, 0.9f, NULL, 1.0f, 1.0f);
	} else {
		C2D_TextBufClear(text_buf);
		C2D_TextParse(&text, text_buf, message_text);
		C2D_TextOptimize(&text);
		float width, height;
		C2D_TextGetDimensions(&text, config_message_text_size(), config_message_text_size(), &width, &height);
		float box_height = height + 20.0f;
		float box_y = 240.0f - box_height - 10.0f;
		ui_draw_frame(10.0f, box_y, 300.0f, box_height, 0.80f);
		C2D_DrawText(&text, C2D_WithColor, 22.0f, box_y + 10.0f, 0.9f, config_message_text_size(), config_message_text_size(), config_message_text_color());
	}
}

// --------- MESSAGE STATE

void ui_message_clear_text(void) {
	free(message_text);
	message_text = NULL;
}

void ui_message_clear_image(void) {
	examine_image = (C2D_Image){0};
}

void ui_show_message(const char *message_id) {
	ui_message_clear_image();
	ui_message_clear_text();
	message_text = text_render_wrap(lang_get(message_id), 276.0f, config_message_text_size());
	if (!message_text) {
		message_text = strdup(lang_get(message_id));
	}
}

void ui_show_image(C2D_Image image) {
	examine_image = image;
}

// --------- INITIALIZATION AND CLEANUP

bool ui_init(void) {
	text_buf = C2D_TextBufNew(4096);
	if (!text_buf) {
		printf("Cannot create game text buffer\n");
		return false;
	}
	return true;
}

void ui_close(void) {
	ui_message_clear_text();
	if (text_buf) {
		C2D_TextBufDelete(text_buf);
		text_buf = NULL;
	}
}
