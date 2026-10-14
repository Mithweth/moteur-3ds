// choice.c
// CHOICE DSL, nested choice stack, START overlay, navigation and drawing.
// Room actions are executed by script.c; the graphical frame comes from ui.c.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "choice.h"
#include "script.h"
#include "room.h"
#include "game.h"
#include "config.h"
#include "ui.h"
#include "hud.h"
#include "lang.h"
#include "save.h"
#include "callbacks.h"
#include "str_utils.h"
#include "text_render.h"

#define FONT_HEIGHT 30.0f

// --------- MODULE STATE

// Parsed CHOICE blocks and the suspended parent actions of nested choices.
typedef struct {
	Choice choices[CHOICE_MAX_CHOICES];
	size_t choice_count;
} ChoiceFile;

typedef struct {
	Choice *choice;
	RoomAction *actions;
	size_t count;
	size_t next;
} ChoiceFrame;

static ChoiceFile *choices = NULL;
static ChoiceFrame choice_stack[CHOICE_MAX_DEPTH];
static size_t choice_depth = 0;

// The START menu uses the same overlay, but its options are handled locally.
// Same order as start_choice.options.
enum {
	START_BACK,
	START_SAVE,
	START_QUIT
};

static Choice start_choice = {
	.name = "START",
	.prompt = "GAME_OPTION_TITLE",
	.option_count = 3,
	.options = {
		{ .name = "GAME_OPTION_BACK" },
		{ .name = "GAME_OPTION_SAVE" },
		{ .name = "GAME_OPTION_QUIT" }
	}
};

static Choice *overlay_choice = NULL;
static size_t overlay_choice_selected = 0;
static Choice *pending_choice = NULL;
static size_t pending_choice_selected = 0;

// Last rendered first-button center, used for hit-testing on the next frame.
static float choice_first_button_y;
static C2D_TextBuf text_buf;

// --------- CHOICE FILE PARSING

static Choice *choice_find(ChoiceFile *file, const char *name) {
	for (size_t i = 0; i < file->choice_count; i++) {
		if (strcmp(file->choices[i].name, name) == 0) {
			return &file->choices[i];
		}
	}
	return NULL;
}

// Parses a complete CHOICE file; each nested CHOICE refers to this same file.
static bool choice_load(ChoiceFile *choices, const char *filename) {
	if (!choices) {
		return false;
	}
	FILE *f = fopen(filename, "r");
	if (!f) {
		printf("Cannot open choice file: %s\n", filename);
		return false;
	}

	char line[512];
	size_t line_number = 0;
	Choice *choice = NULL;
	ChoiceOption *option = NULL;

	while (fgets(line, sizeof(line), f)) {
		line_number++;
		char *p = str_trim(line);
		if (!*p || *p == '#') {
			continue;
		}

		char *command = strtok(p, " ");
		char *argument = strtok(NULL, " ");
		if (!command) {
			continue;
		}

		// At file level, CHOICE declares a block; inside OPTION, it is an action.
		if (strcmp(command, "CHOICE") == 0 && !option) {
			if (choice || !argument || choices->choice_count >= CHOICE_MAX_CHOICES) {
				printf("%s:%zu: invalid CHOICE declaration\n", filename, line_number);
				fclose(f);
				return false;
			}
			if (choice_find(choices, argument)) {
				printf("%s:%zu: duplicate CHOICE\n", filename, line_number);
				fclose(f);
				return false;
			}
			choice = &choices->choices[choices->choice_count++];
			choice->name = strdup(argument);
			continue;
		}

		if (strcmp(command, "END_CHOICE") == 0) {
			if (!choice || option || !choice->prompt || !choice->option_count) {
				printf("%s:%zu: invalid END_CHOICE\n", filename, line_number);
				fclose(f);
				return false;
			}
			choice = NULL;
			continue;
		}

		if (strcmp(command, "PROMPT") == 0) {
			if (!choice || option || !argument || choice->prompt) {
				printf("%s:%zu: invalid PROMPT\n", filename, line_number);
				fclose(f);
				return false;
			}
			choice->prompt = strdup(argument);
			continue;
		}

		if (strcmp(command, "OPTION") == 0) {
			if (!choice || option || !argument || choice->option_count >= CHOICE_MAX_OPTIONS) {
				printf("%s:%zu: invalid OPTION\n", filename, line_number);
				fclose(f);
				return false;
			}
			for (size_t i = 0; i < choice->option_count; i++) {
				if (strcmp(choice->options[i].name, argument) == 0) {
					printf("%s:%zu: duplicate OPTION\n", filename, line_number);
					fclose(f);
					return false;
				}
			}
			option = &choice->options[choice->option_count++];
			option->name = strdup(argument);
			continue;
		}

		if (strcmp(command, "END_OPTION") == 0) {
			if (!option) {
				printf("%s:%zu: END_OPTION without OPTION\n", filename, line_number);
				fclose(f);
				return false;
			}
			option = NULL;
			continue;
		}

		if (strcmp(command, "WHEN") == 0) {
			char *name = strtok(NULL, " ");
			char *value = strtok(NULL, " ");
			if (!option || !argument || !name || !value || option->block.condition_count >= ROOM_MAX_CONDITIONS) {
				printf("%s:%zu: invalid WHEN\n", filename, line_number);
				fclose(f);
				return false;
			}
			RoomCondition condition = script_condition_add(argument, name, value);
			if (!condition.name) {
				printf("%s:%zu: unknown WHEN condition\n", filename, line_number);
				fclose(f);
				return false;
			}
			option->block.conditions[option->block.condition_count++] = condition;
			continue;
		}

		if (!option) {
			printf("%s:%zu: action outside OPTION\n", filename, line_number);
			fclose(f);
			return false;
		}

		RoomAction action = script_action_add(command, argument);
		if (!action.argument && action.type != ROOM_ACTION_EXIT_CHOICE) {
			printf("%s:%zu: invalid action\n", filename, line_number);
			fclose(f);
			return false;
		}
		if (action.type == ROOM_ACTION_MESSAGE_IMAGE) {
			action.image = room_get_image(action.argument);
			if (!action.image.tex) {
				printf("%s:%zu: unknown MESSAGE_IMAGE image: %s\n", filename, line_number, action.argument);
				script_action_remove(&action);
				fclose(f);
				return false;
			}
		}
		if (action.type == ROOM_ACTION_MINIGAME && !callbacks_minigame_find(action.argument)) {
			printf("%s:%zu: invalid MINIGAME: %s\n", filename, line_number, action.argument);
			script_action_remove(&action);
			fclose(f);
			return false;
		}
		if (option->block.action_count >= ROOM_MAX_ACTIONS) {
			printf("%s:%zu: too many ACTION\n", filename, line_number);
			script_action_remove(&action);
			fclose(f);
			return false;
		}
		option->block.actions[option->block.action_count++] = action;
	}

	if (choice || option) {
		printf("%s:%zu: unclosed CHOICE or OPTION\n", filename, line_number);
		fclose(f);
		return false;
	}
	if (!choice_find(choices, "MAIN")) {
		printf("%s:%zu: missing CHOICE MAIN\n", filename, line_number);
		fclose(f);
		return false;
	}

	// Nested CHOICE directives refer to another block in this same file.
	for (size_t i = 0; i < choices->choice_count; i++) {
		Choice *current = &choices->choices[i];
		for (size_t j = 0; j < current->option_count; j++) {
			RoomActionBlock *block = &current->options[j].block;
			for (size_t k = 0; k < block->action_count; k++) {
				RoomAction *action = &block->actions[k];
				if (action->type == ROOM_ACTION_CHOICE && !choice_find(choices, action->argument)) {
					printf("%s: invalid nested CHOICE: %s\n", filename, action->argument);
					fclose(f);
					return false;
				}
			}
		}
	}

	fclose(f);
	return true;
}

// --------- CHOICE FILE CLEANUP

static void choice_file_free(ChoiceFile *file) {
	if (!file) {
		return;
	}
	for (size_t i = 0; i < file->choice_count; i++) {
		Choice *choice = &file->choices[i];
		free(choice->name);
		free(choice->prompt);
		for (size_t j = 0; j < choice->option_count; j++) {
			free(choice->options[j].name);
			script_action_block_remove(&choice->options[j].block);
		}
	}
	free(file);
}

// --------- OPTION AVAILABILITY

static bool choice_option_available(Choice *choice, size_t index) {
	if (choice == &start_choice) {
		return index != START_SAVE || (game_get_mode() == GAME_NORMAL && !pending_choice && save_enabled());
	}
	return script_match_conditions(choice->options[index].block.conditions, choice->options[index].block.condition_count);
}

static size_t choice_first_available(Choice *choice) {
	for (size_t i = 0; i < choice->option_count; i++) {
		if (choice_option_available(choice, i)) {
			return i;
		}
	}
	return 0;
}

static size_t choice_visible_count(Choice *choice) {
	size_t visible_count = 0;
	for (size_t i = 0; i < choice->option_count; i++) {
		if (choice_option_available(choice, i)) {
			visible_count++;
		}
	}
	return visible_count;
}

// --------- OVERLAY STATE

static void choice_show(Choice *choice) {
	size_t selected = choice ? choice_first_available(choice) : 0;
	if (overlay_choice == &start_choice) {
		pending_choice = choice;
		pending_choice_selected = selected;
	} else {
		overlay_choice = choice;
		overlay_choice_selected = selected;
	}
}

bool choice_is_active(bool normal_mode) {
	return overlay_choice == &start_choice || (overlay_choice != NULL && normal_mode);
}

bool choice_has_open(void) {
	return choice_depth != 0;
}

// --------- CHOICE STACK

// Called on room close: release the scripted choice without destroying the
// START overlay or the graphics buffer.
void choice_clear(void) {
	choice_show(NULL);
	choice_depth = 0;
	choice_file_free(choices);
	choices = NULL;
}

static bool choice_file_open(const char *name) {
	if (choices) {
		return false;
	}

	choices = calloc(1, sizeof(*choices));
	if (!choices) {
		return false;
	}

	char filename[512];
	snprintf(filename, sizeof(filename), "%s/%s", room_get_path(), name);
	if (!choice_load(choices, filename)) {
		choice_clear();
		return false;
	}
	return true;
}

// Restore the parent before resuming the actions after the nested CHOICE.
void choice_pop(void) {
	if (!choice_depth) {
		return;
	}

	ChoiceFrame frame = choice_stack[--choice_depth];

	if (choice_depth) {
		choice_show(choice_stack[choice_depth - 1].choice);
	} else {
		choice_clear();
	}
	script_execute_actions(frame.actions, frame.count, frame.next);
}

bool choice_push(const char *name, RoomAction *actions, size_t count, size_t next) {
	bool root = choice_depth == 0;

	if (root) {
		if (!choice_file_open(name)) {
			return false;
		}
	}

	Choice *choice = choice_find(choices, root ? "MAIN" : name);
	if (!choice || choice_depth >= CHOICE_MAX_DEPTH) {
		if (root) {
			choice_clear();
		}
		return false;
	}

	choice_stack[choice_depth++] = (ChoiceFrame){
		.choice = choice,
		.actions = actions,
		.count = count,
		.next = next,
	};
	choice_show(choice);
	return true;
}

static void choice_select(size_t index) {
	if (!choice_depth) {
		return;
	}
	Choice *choice = choice_stack[choice_depth - 1].choice;
	if (index >= choice->option_count) {
		return;
	}
	ChoiceOption *option = &choice->options[index];
	if (!script_match_conditions(option->block.conditions, option->block.condition_count)) {
		return;
	}
	script_execute_actions(option->block.actions, option->block.action_count, 0);
}

// --------- GRAPHIC FUNCTIONS

// Prompt and visible buttons share the frame; the same layout drives touch.
void choice_draw(void) {
	size_t visible_count = choice_visible_count(overlay_choice);
	if (visible_count == 0) {
		return;
	}
	C2D_TextBufClear(text_buf);

	// Prompt (not selectable).
	C2D_Text text;
	char *wrapped = text_render_wrap(lang_get(overlay_choice->prompt), config_choice_frame_width() - 12.0f, config_choice_prompt_text_size());
	if (!wrapped) {
		wrapped = strdup(lang_get(overlay_choice->prompt));
	}
	C2D_TextParse(&text, text_buf, wrapped);
	free(wrapped);
	C2D_TextOptimize(&text);
	float width, height;
	C2D_TextGetDimensions(&text, config_choice_prompt_text_size(), config_choice_prompt_text_size(), &width, &height);

	float button_spacing = FONT_HEIGHT * config_choice_button_text_size() + 2.0f * config_choice_button_padding();
	float box_height = height + (visible_count + 1) * button_spacing + 24.0f;
	float box_x = (320.0f - config_choice_frame_width()) / 2.0f;
	float box_y = (240.0f - box_height) / 2.0f;
	float y = box_y + 12.0f + button_spacing / 2.0f;

	C2D_DrawRectSolid(0.0f, 0.0f, 0.93f, 320.0f, 240.0f, C2D_Color32(0, 0, 0, 192));
	ui_begin_frame();
	ui_draw_frame(box_x, box_y, config_choice_frame_width(), box_height, 0.95f);

	C2D_DrawText(&text, C2D_AlignCenter | C2D_WithColor, 160.0f, y - 9.0f, 0.99f, config_choice_prompt_text_size(), config_choice_prompt_text_size(), config_choice_prompt_text_color());

	// Options.
	y += height + button_spacing;
	choice_first_button_y = y;
	for (size_t i = 0; i < overlay_choice->option_count; i++) {
		if (!choice_option_available(overlay_choice, i)) {
			continue;
		}
		ChoiceOption *option = &overlay_choice->options[i];
		ui_draw_button(option->name, 160.0f, y, overlay_choice_selected == i);
		y += button_spacing;
	}
}

// --------- INPUT FUNCTIONS

static int select_option(int x, int y) {
	float box_x = (320.0f - config_choice_frame_width()) / 2.0f;
	float spacing = FONT_HEIGHT * config_choice_button_text_size() + 2.0f * config_choice_button_padding();

	if (x < box_x || x >= box_x + config_choice_frame_width()) {
		return -1;
	}

	float button_y = choice_first_button_y;

	for (size_t i = 0; i < overlay_choice->option_count; i++) {
		if (!choice_option_available(overlay_choice, i)) {
			continue;
		}

		if (y >= button_y - spacing / 2.0f && y < button_y + spacing / 2.0f) {
			return (int)i;
		}

		button_y += spacing;
	}

	return -1;
}

void choice_start_open(void) {
	pending_choice = overlay_choice;
	overlay_choice = &start_choice;
	pending_choice_selected = overlay_choice_selected;
	overlay_choice_selected = choice_first_available(overlay_choice);
}

static void choice_start_close(void) {
	overlay_choice = pending_choice;
	overlay_choice_selected = pending_choice_selected;
	pending_choice = NULL;
	timer_resume();
}

// The active overlay consumes input; scripted choices also update the HUD.
bool choice_update(u32 keys, touchPosition touch) {
	if (overlay_choice != &start_choice) {
		if (hud_update()) {
			return true;
		}
		// When all options disappear, close the choice like EXIT_CHOICE.
		if (!choice_visible_count(overlay_choice)) {
			choice_pop();
			return true;
		}
		// A WHEN condition may have hidden the selected option.
		if (!choice_option_available(overlay_choice, overlay_choice_selected)) {
			overlay_choice_selected = choice_first_available(overlay_choice);
		}
	}
	if (keys & KEY_UP) {
		for (size_t i = overlay_choice_selected; i > 0; i--) {
			if (choice_option_available(overlay_choice, i - 1)) {
				overlay_choice_selected = i - 1;
				break;
			}
		}
	}
	if (keys & KEY_DOWN) {
		for (size_t i = overlay_choice_selected + 1; i < overlay_choice->option_count; i++) {
			if (choice_option_available(overlay_choice, i)) {
				overlay_choice_selected = i;
				break;
			}
		}
	}
	if (keys & KEY_TOUCH) {
		int index = select_option(touch.px, touch.py);
		if (index < 0) {
			return true;
		}
		overlay_choice_selected = index;
		keys |= KEY_A;
	}
	if (keys & KEY_A) {
		if (overlay_choice == &start_choice) {
			size_t action = overlay_choice_selected;
			choice_start_close();
			if (action == START_QUIT) {
				return false;
			}
			if (action == START_SAVE) {
				game_show_message(save_write() ? "SAVE_SAVING_SUCCESS" : "SAVE_SAVING_ERROR", NULL);
			}
		} else {
			choice_select(overlay_choice_selected);
		}
		return true;
	}
	if (keys & KEY_B) {
		if (overlay_choice == &start_choice) {
			choice_start_close();
		} else {
			choice_pop();
		}
		return true;
	}
	if (keys & KEY_START) {
		if (overlay_choice == &start_choice) {
			choice_start_close();
		} else {
			choice_start_open();
		}
	}
	return true;
}

// --------- INITIALIZATION AND CLEANUP

bool choice_init(void) {
	text_buf = C2D_TextBufNew(4096);
	if (!text_buf) {
		printf("Cannot create game text buffer\n");
		return false;
	}
	return true;
}

void choice_close(void) {
	choice_clear();
	overlay_choice = NULL;
	pending_choice = NULL;
	if (text_buf) {
		C2D_TextBufDelete(text_buf);
		text_buf = NULL;
	}
}
