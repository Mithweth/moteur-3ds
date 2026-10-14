// choice.h
// Scripted CHOICE dialogues and the shared START overlay.
#pragma once

#include <3ds.h>
#include <stdbool.h>
#include <stddef.h>
#include "script.h"

#define CHOICE_MAX_OPTIONS 16
#define CHOICE_MAX_CHOICES 16
#define CHOICE_MAX_DEPTH    8

// One OPTION of a choice: name is the translation key of its button. Its
// WHEN conditions decide whether the button is shown, its actions run when
// the player picks it.
typedef struct {
	char *name;
	RoomActionBlock block;
} ChoiceOption;

// One CHOICE block of a choice file: name identifies it within the file
// (MAIN is the entry point), prompt is the translation key of the question.
// Also used by choice.c for the START menu, whose fields are static strings.
typedef struct {
	char *name;
	char *prompt;
	ChoiceOption options[CHOICE_MAX_OPTIONS];
	size_t option_count;
} Choice;

// --------- SCRIPT ACTION INTERFACE

bool choice_push(const char *name, RoomAction *actions, size_t count, size_t next);
void choice_pop(void);
bool choice_has_open(void);
// Called by room_close to release room-owned CHOICE state (not graphics).
void choice_clear(void);

// --------- OVERLAY INPUT AND RENDERING

// START takes priority in every mode; scripted CHOICE only in GAME_NORMAL.
bool choice_is_active(bool normal_mode);
void choice_start_open(void);
// Returns false when Quit was selected.
bool choice_update(u32 keys, touchPosition touch);
void choice_draw(void);

// --------- INITIALIZATION AND CLEANUP

bool choice_init(void);
void choice_close(void);
