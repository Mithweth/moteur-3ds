// config.h
// Read-only access to the configuration loaded from romfs:/game/game.
// Returned strings belong to config.c and remain valid until config_close().
#pragma once

#include <3ds.h>
#include <stdbool.h>
#include <stddef.h>

const char *config_room(void);
const char *config_music(void);
const char *config_cannot_use_message(void);
size_t config_item_count(void);
u32 config_frame_shadow(void);
u32 config_frame_outer_border(void);
u32 config_frame_outer_background(void);
u32 config_frame_inner_border(void);
u32 config_frame_inner_background(void);
u32 config_choice_button_color(void);
u32 config_choice_button_text_color(void);
u32 config_choice_prompt_text_color(void);
float config_choice_button_text_size(void);
float config_choice_prompt_text_size(void);
float config_choice_frame_width(void);
float config_choice_button_padding(void);
u32 config_message_text_color(void);
float config_message_text_size(void);
const char *config_item(size_t index);

bool config_load(const char *filename);
void config_close(void);
