// room.c
// Room loading, drawing and action execution. The current room is a single
// heap-allocated Room owned by this module (NULL when none is loaded).
// See docs/ROOMS.en.md for the room script format and its semantics.
// Implementation notes: a CHOICE action in a room script loads the whole
// choice file (docs/CHOICES.en.md) into `choices`, which lives until the
// outermost choice is closed or the room is. Nested CHOICE actions only look
// up another block of that same file and push it on choice_stack; each frame
// remembers where the action flow resumes once its choice is left, so
// EXIT_CHOICE (or B) acts like a return. A choice suspends the action flow
// like a MESSAGE: execute_actions returns true and room_choice_select
// starts the next flow when the player picks an option.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "room.h"
#include "gamestate.h"
#include "inventory.h"
#include "gfxmap.h"
#include "game.h"
#include "audio.h"
#include "callbacks.h"
#include "str_utils.h"

// The currently loaded room, or NULL.
static Room *room = NULL;

// Remaining actions of a block suspended by MESSAGE, MESSAGE_IMAGE, WAIT_SFX
// or TIMELINE. game.c calls continue_actions() once the message is dismissed,
// the sound is over, or the timeline ends with RETURN, to resume at `next`.
// The pointer refers to arrays inside the room or the choice file, so
// room_close() clears it.
typedef struct {
	RoomAction *actions;
	size_t count;
	size_t next;
} PendingActions;

// Every CHOICE block of the open choice file; MAIN is the entry point.
typedef struct {
	Choice choices[CHOICE_MAX_CHOICES];
	size_t choice_count;
} ChoiceFile;

// One open choice. actions[next..count) is the rest of the block whose CHOICE
// action opened it (in the room or in the parent choice's option), resumed
// when this choice is left.
typedef struct {
	Choice *choice;
	RoomAction *actions;
	size_t count;
	size_t next;
} ChoiceFrame;

static PendingActions pending_actions;
// The open choice file, or NULL when no choice is open.
static ChoiceFile *choices = NULL;
// Open choices, outermost first: choice_stack[choice_depth - 1] is shown.
static ChoiceFrame choice_stack[CHOICE_MAX_DEPTH];
static size_t choice_depth = 0;

static void continue_actions(void);
static bool execute_actions(RoomAction *actions, size_t count, size_t start);

// Remembers where execute_actions resumes once game.c calls continue_actions().
static void pending_actions_set(RoomAction *actions, size_t count, size_t next) {
	pending_actions.actions = actions;
	pending_actions.count = count;
	pending_actions.next = next;
}

// Build a condition from a WHEN directive. On an unknown type, returns a
// condition with a NULL name, which the parser treats as a load error.
static RoomCondition condition_add(const char *type, const char *name, const char *value) {
	RoomCondition condition = {0};

	if (strcmp(type, "STATE_IS") == 0) {
		condition.type = ROOM_CONDITION_STATE_IS;
	} else if (strcmp(type, "INVENTORY_HAS") == 0) {
		condition.type = ROOM_CONDITION_INVENTORY_HAS;
	} else {
		printf("Unknown condition: %s\n", type);
		return condition;
	}

	condition.name = strdup(name);
	condition.expected = str_to_bool(value);
	return condition;
}

// Free the strings owned by a condition, action or action block.
static void condition_remove(RoomCondition *condition) {
	free(condition->name);
}

// Build an action from a directive and its argument. On an unknown command
// or a missing argument, returns an action with a NULL argument (load error).
static RoomAction action_add(const char *command, const char *argument) {
	RoomAction action = {0};

	if (strcmp(command, "EXIT_CHOICE") == 0) {
		action.type = ROOM_ACTION_EXIT_CHOICE;
		return action;
	}
	if (!argument) {
		printf("Missing argument for %s\n", command);
		return action;
	}

	if (strcmp(command, "SET") == 0) {
		action.type = ROOM_ACTION_SET;
	} else if (strcmp(command, "INVENTORY_ADD") == 0) {
		action.type = ROOM_ACTION_INVENTORY_ADD;
	} else if (strcmp(command, "INVENTORY_REMOVE") == 0) {
		action.type = ROOM_ACTION_INVENTORY_REMOVE;
	} else if (strcmp(command, "MESSAGE") == 0) {
		action.type = ROOM_ACTION_MESSAGE;
	} else if (strcmp(command, "MESSAGE_IMAGE") == 0) {
		action.type = ROOM_ACTION_MESSAGE_IMAGE;
	} else if (strcmp(command, "SFX") == 0) {
		action.type = ROOM_ACTION_SFX;
	} else if (strcmp(command, "WAIT_SFX") == 0) {
		action.type = ROOM_ACTION_WAIT_SFX;
	} else if (strcmp(command, "ROOM") == 0) {
		action.type = ROOM_ACTION_ROOM;
	} else if (strcmp(command, "TIMELINE") == 0) {
		action.type = ROOM_ACTION_TIMELINE;
	} else if (strcmp(command, "MINIGAME") == 0) {
		action.type = ROOM_ACTION_MINIGAME;
	} else if (strcmp(command, "CHOICE") == 0) {
		action.type = ROOM_ACTION_CHOICE;
	} else {
		printf("Unknown action: %s\n", command);
		return action;
	}
	action.argument = strdup(argument);
	return action;
}

static void action_remove(RoomAction *action) {
	free(action->argument);
}

static void action_block_remove(RoomActionBlock *block) {
	for (size_t i = 0; i < block->condition_count; i++) {
		condition_remove(&block->conditions[i]);
	}

	for (size_t i = 0; i < block->action_count; i++) {
		action_remove(&block->actions[i]);
	}
}

// Path of `room` for a PATH direction keyword, or NULL if invalid.
static Path *get_path(Room *room, const char *direction) {
	if (strcmp(direction, "NORTH") == 0) {
		return &room->north;
	}

	if (strcmp(direction, "SOUTH") == 0) {
		return &room->south;
	}

	if (strcmp(direction, "EAST") == 0) {
		return &room->east;
	}

	if (strcmp(direction, "WEST") == 0) {
		return &room->west;
	}

	if (strcmp(direction, "NORTHWEST") == 0) {
		return &room->northwest;
	}

	if (strcmp(direction, "SOUTHWEST") == 0) {
		return &room->southwest;
	}

	if (strcmp(direction, "NORTHEAST") == 0) {
		return &room->northeast;
	}

	if (strcmp(direction, "SOUTHEAST") == 0) {
		return &room->southeast;
	}

	return NULL;
}

// Parse a room script into the already allocated `room`. The parser is
// line-based: each line starts with a directive, blocks are opened by
// IMAGE/HOTSPOT/PATH/ACTION/USE and closed by the matching END_*. Any error
// stops the load and returns false; room_init() then frees the room.
static bool load_room(const char *filename) {
	if (!room) {
		return false;
	}

	FILE *f = fopen(filename, "r");

	if (!f) {
		printf("Cannot open room: %s\n", filename);
		return false;
	}

	char line[512];

	// Currently open blocks. They are tracked independently, so the parser
	// does not check proper nesting: a missing END_* leaves a block open.
	RoomImage *image = NULL;
	Hotspot *hotspot = NULL;
	Path *path = NULL;
	RoomActionBlock *action_block = NULL;
	RoomUse *use = NULL;

	size_t line_number = 0;

	while (fgets(line, sizeof(line), f)) {
		line_number++;

		char *p = str_trim(line);

		if (!*p || *p == '#') {
			continue;
		}

		char *command = strtok(p, " ");

		if (!command) {
			continue;
		}

		if (strcmp(command, "IMAGE") == 0) {
			char *image_name = strtok(NULL, " ");
			char *x = strtok(NULL, " ");
			char *y = strtok(NULL, " ");
			char *z = strtok(NULL, " ");

			if (!image_name || !x || !y || !z) {
				printf("%s:%zu: invalid IMAGE\n", filename, line_number);
				fclose(f);
				return false;
			}

			if (room->image_count >= ROOM_MAX_IMAGES) {
				printf("%s:%zu: too many images\n", filename, line_number);
				fclose(f);
				return false;
			}

			image = &room->images[room->image_count++];
			memset(image, 0, sizeof(*image));

			// An unknown image name only prints a warning and leaves an empty
			// image (tex == NULL) in the room, so check the log when adding one.
			image->image = gfxmap_get_image(&room->assets, image_name);
			image->x = atof(x);
			image->y = atof(y);
			image->z = atof(z);

			continue;
		}

		if (strcmp(command, "END_IMAGE") == 0) {
			image = NULL;
			continue;
		}


		if (strcmp(command, "HOTSPOT") == 0) {
			char *id = strtok(NULL, " ");
			char *x = strtok(NULL, " ");
			char *y = strtok(NULL, " ");
			char *width = strtok(NULL, " ");
			char *height = strtok(NULL, " ");

			if (!id || !x || !y || !width || !height) {
				printf("%s:%zu: invalid HOTSPOT\n", filename, line_number);
				fclose(f);
				return false;
			}

			if (room->hotspot_count >= ROOM_MAX_HOTSPOTS) {
				printf("%s:%zu: too many hotspots\n", filename, line_number);
				fclose(f);
				return false;
			}

			hotspot = &room->hotspots[room->hotspot_count++];
			memset(hotspot, 0, sizeof(*hotspot));

			hotspot->id = strdup(id);
			hotspot->x = atoi(x);
			hotspot->y = atoi(y);
			hotspot->width = atoi(width);
			hotspot->height = atoi(height);
			continue;
		}

		if (strcmp(command, "END_HOTSPOT") == 0) {
			hotspot = NULL;
			action_block = NULL;
			use = NULL;
			continue;
		}

		if (strcmp(command, "PATH") == 0) {
			char *direction = strtok(NULL, " ");

			if (!direction) {
				printf("%s:%zu: missing PATH direction\n", filename, line_number);
				fclose(f);
				return false;
			}
			path = get_path(room, direction);
			if (!path) {
				printf("%s:%zu: invalid PATH direction: %s\n", filename, line_number, direction);
				fclose(f);
				return false;
			}
			path->exists = true;
			continue;
		}

		if (strcmp(command, "END_PATH") == 0) {
			path = NULL;
			continue;
		}

		// ACTION belongs to the open hotspot first, otherwise to the open path.
		if (strcmp(command, "ACTION") == 0) {
			if (hotspot) {
				if (hotspot->action_block_count >= ROOM_MAX_ACTION_BLOCKS) {
					printf("%s:%zu: too many ACTION blocks\n",filename, line_number);
					fclose(f);
					return false;
				}

				action_block = &hotspot->action_blocks[hotspot->action_block_count++];
			} else if (path) {
				if (path->action_block_count >= ROOM_MAX_ACTION_BLOCKS) {
					printf("%s:%zu: too many PATH ACTION blocks\n", filename, line_number);
					fclose(f);
					return false;
				}

				action_block = &path->action_blocks[path->action_block_count++];
			} else {
				printf("%s:%zu: ACTION outside HOTSPOT/PATH\n", filename, line_number);
				fclose(f);
				return false;
			}
			continue;
		}

		if (strcmp(command, "END_ACTION") == 0) {
			action_block = NULL;
			continue;
		}

		if (strcmp(command, "USE") == 0) {
			char *item = strtok(NULL, " ");

			if (!hotspot || !item) {
				printf("%s:%zu: invalid USE\n", filename, line_number);
				fclose(f);
				return false;
			}

			if (hotspot->use_count >= ROOM_MAX_USES) {
				printf("%s:%zu: too many USE blocks\n", filename, line_number);
				fclose(f);
				return false;
			}

			use = &hotspot->uses[hotspot->use_count++];
			memset(use, 0, sizeof(*use));

			use->item = strdup(item);

			continue;
		}

		if (strcmp(command, "END_USE") == 0) {
			use = NULL;
			continue;
		}

		if (strcmp(command, "WHEN") == 0) {
			char *type = strtok(NULL, " ");
			char *name = strtok(NULL, " ");
			char *value = strtok(NULL, " ");

			if (!type || !name || !value) {
				printf("%s:%zu: invalid WHEN\n", filename, line_number);
				fclose(f);
				return false;
			}

			RoomCondition condition = condition_add(type, name, value);

			if (!condition.name) {
				fclose(f);
				return false;
			}

			// Attach the condition to the innermost open block, in this order of
			// precedence: action block > use > image > hotspot > path.
			if (action_block) {
				if (action_block->condition_count >= ROOM_MAX_CONDITIONS) {
					printf("%s:%zu: too many ACTION conditions\n", filename, line_number);
					condition_remove(&condition);
					fclose(f);
					return false;
				}

				action_block->conditions[action_block->condition_count++] = condition;

			} else if (use) {
				if (use->condition_count >= ROOM_MAX_CONDITIONS) {
					printf("%s:%zu: too many USE conditions\n", filename, line_number);
					condition_remove(&condition);
					fclose(f);
					return false;
				}

				use->conditions[use->condition_count++] = condition;

			} else if (image) {
				if (image->condition_count >= ROOM_MAX_CONDITIONS) {
					printf("%s:%zu: too many IMAGE conditions\n", filename, line_number);
					condition_remove(&condition);
					fclose(f);
					return false;
				}

				image->conditions[image->condition_count++] = condition;

			} else if (hotspot) {
				if (hotspot->condition_count >= ROOM_MAX_CONDITIONS) {
					printf("%s:%zu: too many HOTSPOT conditions\n", filename, line_number);
					condition_remove(&condition);
					fclose(f);
					return false;
				}

				hotspot->conditions[hotspot->condition_count++] = condition;

			} else if (path) {
				if (path->condition_count >= ROOM_MAX_CONDITIONS) {
					printf("%s:%zu: too many PATH conditions\n", filename, line_number);
					condition_remove(&condition);
					fclose(f);
					return false;
				}

				path->conditions[path->condition_count++] = condition;

			} else {
				printf("%s:%zu: WHEN outside condition block\n", filename, line_number);
				condition_remove(&condition);
				fclose(f);
				return false;
			}
			continue;
		}

		// MESSAGE or MESSAGE_IMAGE directly inside a HOTSPOT (not in
		// ACTION/USE) is the hotspot's first-touch message (MESSAGE wins
		// when both are set); elsewhere it is a regular action.
		if (strcmp(command, "MESSAGE") == 0 && hotspot && !action_block && !use) {
			char *message = strtok(NULL, " ");

			if (!message) {
				printf("%s:%zu: invalid MESSAGE\n", filename, line_number);
				fclose(f);
				return false;
			}
			if (hotspot->message_id) {
				printf("%s:%zu: duplicate MESSAGE in HOTSPOT %s\n", filename, line_number, hotspot->id);
				fclose(f);
				return false;
			}
			hotspot->message_id = strdup(message);
			continue;
		}
		if (strcmp(command, "MESSAGE_IMAGE") == 0 && hotspot && !action_block && !use) {
			char *image = strtok(NULL, " ");

			if (!image) {
				printf("%s:%zu: invalid MESSAGE_IMAGE\n", filename, line_number);
				fclose(f);
				return false;
			}
			if (hotspot->message_image.tex) {
				printf("%s:%zu: duplicate MESSAGE_IMAGE in HOTSPOT %s\n", filename, line_number, hotspot->id);
				fclose(f);
				return false;
			}

			hotspot->message_image = gfxmap_get_image(&room->assets, image);
			if (!hotspot->message_image.tex) {
				printf("%s:%zu: unknown image %s\n", filename, line_number, image);
				fclose(f);
				return false;
			}
			continue;
		}

		// Inside ACTION or USE, any other directive is an action.
		if (action_block || use) {
			char *argument = strtok(NULL, " ");

			RoomAction action = action_add(command, argument);

			if (!action.argument && action.type != ROOM_ACTION_EXIT_CHOICE) {
				printf("%s:%zu: invalid action\n", filename, line_number);
				fclose(f);
				return false;
			}

			if (action.type == ROOM_ACTION_MESSAGE_IMAGE) {
				action.image = gfxmap_get_image(&room->assets, action.argument);
				if (!action.image.tex) {
					printf("%s:%zu: unknown image %s\n", filename, line_number, action.argument);
					action_remove(&action);
					fclose(f);
					return false;
				}
			}

			// Checked here rather than when the action runs, so a typo fails
			// the room load instead of making the hotspot silently do nothing.
			if (action.type == ROOM_ACTION_MINIGAME && !callbacks_minigame_find(action.argument)) {
				printf("%s:%zu: unknown mini-game %s\n", filename, line_number, action.argument);
				action_remove(&action);
				fclose(f);
				return false;
			}

			if (action_block) {
				if (action_block->action_count >= ROOM_MAX_ACTIONS) {
					printf("%s:%zu: too many ACTION actions\n", filename, line_number);
					action_remove(&action);
					fclose(f);
					return false;
				}

				action_block->actions[action_block->action_count++] = action;

			} else if (use) {
				if (use->action_count >= ROOM_MAX_ACTIONS) {
					printf("%s:%zu: too many USE actions\n", filename, line_number);
					action_remove(&action);
					fclose(f);
					return false;
				}

				use->actions[use->action_count++] = action;
			}
			continue;
		}

		printf("%s:%zu: unexpected directive: %s\n", filename, line_number, command);
		fclose(f);
		return false;
	}

	printf("Loaded room: %zu images, %zu hotspots\n", room->image_count, room->hotspot_count);
	fclose(f);
	return true;
}

// The CHOICE block of file called name, or NULL.
static Choice *choice_find(ChoiceFile *file, const char *name) {
	for (size_t i = 0; i < file->choice_count; i++) {
		if (strcmp(file->choices[i].name, name) == 0) {
			return &file->choices[i];
		}
	}
	return NULL;
}

// Parse a choice file into `choices` (allocated by the caller). Same lexical
// rules as a room script. At file level, CHOICE <name> opens a block; inside
// an OPTION, CHOICE <name> is an action opening another block of this file.
// Fails on any syntax error, when MAIN is missing, or when a nested CHOICE
// names a block that does not exist. On failure, the caller frees `choices`.
static bool load_choice(const char *filename) {
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
			RoomCondition condition = condition_add(argument, name, value);
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

		RoomAction action = action_add(command, argument);
		if (!action.argument && action.type != ROOM_ACTION_EXIT_CHOICE) {
			printf("%s:%zu: invalid action\n", filename, line_number);
			fclose(f);
			return false;
		}
		if (action.type == ROOM_ACTION_MESSAGE_IMAGE) {
			action.image = gfxmap_get_image(&room->assets, action.argument);
			if (!action.image.tex) {
				printf("%s:%zu: unknown MESSAGE_IMAGE image: %s\n", filename, line_number, action.argument);
				action_remove(&action);
				fclose(f);
				return false;
			}
		}
		if (action.type == ROOM_ACTION_MINIGAME && !callbacks_minigame_find(action.argument)) {
			printf("%s:%zu: invalid MINIGAME: %s\n", filename, line_number, action.argument);
			action_remove(&action);
			fclose(f);
			return false;
		}
		if (option->block.action_count >= ROOM_MAX_ACTIONS) {
			printf("%s:%zu: too many ACTION\n", filename, line_number);
			action_remove(&action);
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

// Free a choice file and everything its blocks own. Accepts NULL and a
// partially loaded file.
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
			action_block_remove(&choice->options[j].block);
		}
	}
	free(file);
}

// Hide the choice, forget every open level and free the choice file. Safe
// when no choice is open.
static void choice_close(void) {
	game_show_choice(NULL);
	choice_depth = 0;
	choice_file_free(choices);
	choices = NULL;
}

// Load romfs:/rooms/<room>/<name> as the open choice file. Returns false if
// a file is already open or it cannot be loaded.
static bool choice_init(const char *name) {
	if (choices) {
		return false;
	}

	choices = calloc(1, sizeof(*choices));
	if (!choices) {
		return false;
	}

	char filename[512];
	snprintf(filename, sizeof(filename), "%s/%s", room->path, name);
	if (!load_choice(filename)) {
		choice_close();
		return false;
	}
	return true;
}

// Leave the innermost choice: show its parent again (or close the file when
// it was the outermost one), then resume the actions that followed the
// CHOICE action that opened it.
static void choice_pop(void) {
	if (!choice_depth) {
		return;
	}

	ChoiceFrame frame = choice_stack[--choice_depth];

	if (choice_depth) {
		game_show_choice(choice_stack[choice_depth - 1].choice);
	} else {
		choice_close();
	}
	execute_actions(frame.actions, frame.count, frame.next);
}

void room_choice_exit(void) {
	choice_pop();
}

// Open a choice and show it; actions[next..count) resumes when it is left.
// From a room (no choice open), name is a choice file and its MAIN block is
// shown; from an option, name is another block of the open file. Returns
// false, so that the caller goes on with the next action, when the file
// cannot be loaded or the stack is full.
static bool choice_push(const char *name, RoomAction *actions, size_t count, size_t next) {
	bool root = choice_depth == 0;

	if (root) {
		if (!choice_init(name)) {
			return false;
		}
	}

	Choice *choice = choice_find(choices, root ? "MAIN" : name);
	if (!choice || choice_depth >= CHOICE_MAX_DEPTH) {
		if (root) {
			choice_close();
		}
		return false;
	}

	choice_stack[choice_depth++] = (ChoiceFrame){
		.choice = choice,
		.actions = actions,
		.count = count,
		.next = next,
	};
	game_show_choice(choice);
	return true;
}

// Run actions[start..count). Returns true when the action flow ended early:
// a mode-changing action (ROOM, MINIGAME), a MESSAGE, MESSAGE_IMAGE,
// WAIT_SFX or TIMELINE that will resume later through continue_actions(),
// or a CHOICE / EXIT_CHOICE that hands the flow over to the choice stack.
// Callers must then stop running further blocks, since the game mode (and
// possibly the room) has changed.
static bool execute_actions(RoomAction *actions, size_t count, size_t start) {
	char *path;
	for (size_t c = start; c < count; c++) {
		RoomAction *action = &actions[c];
		switch (action->type) {
		case ROOM_ACTION_SET:
			gamestate_set(action->argument);
			break;
		case ROOM_ACTION_INVENTORY_ADD:
			inventory_add(action->argument);
			break;
		case ROOM_ACTION_INVENTORY_REMOVE:
			inventory_remove(action->argument);
			break;
		case ROOM_ACTION_MESSAGE:
			pending_actions_set(actions, count, c + 1);
			game_show_message(action->argument, continue_actions);
			return true;
		case ROOM_ACTION_MESSAGE_IMAGE:
			pending_actions_set(actions, count, c + 1);
			game_show_image(action->image, continue_actions);
			return true;
		case ROOM_ACTION_SFX:
			path = audio_resolve_path(room->path, action->argument, ".raw");
			if (path) {
				sfx_play(path);
				free(path);
			}
			break;
		case ROOM_ACTION_WAIT_SFX:
			// sfx_play() loads the whole sample, so the path can be freed as
			// soon as game_wait_for_sfx() returns.
			path = audio_resolve_path(room->path, action->argument, ".raw");
			pending_actions_set(actions, count, c + 1);
			// Save where to resume before handing control to game.c; if the
			// sound can't be played, forget it and keep going immediately.
			if (path && game_wait_for_sfx(path, continue_actions)) {
				free(path);
				return true;
			}
			free(path);
			pending_actions_set(NULL, 0, 0);
			break;
		case ROOM_ACTION_ROOM:
			// Frees the current room: `actions` must not be touched after this.
			game_set_room(action->argument);
			return true;
		case ROOM_ACTION_TIMELINE:
			pending_actions_set(actions, count, c + 1);
			// On failure, game_timeline_start goes back to the title screen,
			// whose room_close clears pending_actions: return either way.
			game_timeline_start(action->argument, continue_actions);
			return true;
		case ROOM_ACTION_MINIGAME:
			game_minigame_start(action->argument);
			return true;
		case ROOM_ACTION_CHOICE:
			// If the choice cannot be opened (logged), go on with the next action.
			if (choice_push(action->argument, actions, count, c + 1)) {
				return true;
			}
			break;
		// With no choice open (e.g. in a room script), EXIT_CHOICE does nothing.
		case ROOM_ACTION_EXIT_CHOICE:
			if (choice_depth) {
				choice_pop();
				return true;
			}
			break;
		}
	}
	return false;
}

// True if every condition matches (an empty list always matches).
static bool match_conditions(const RoomCondition *conditions, size_t count) {
	for (size_t c = 0; c < count; c++) {
		const RoomCondition *condition = &conditions[c];
		bool value;
		if (condition->type == ROOM_CONDITION_STATE_IS) {
			value = gamestate_get(condition->name);
		} else {
			value = inventory_has(condition->name);
		}

		if (value != condition->expected) {
			return false;
		}
	}
	return true;
}

// ACTION blocks are sequential, not alternatives: every block whose
// conditions match runs, in declaration order, and conditions are evaluated
// when each block is reached, so an earlier block can enable or disable a
// later one. Stops early if a block ends the action flow.
static void execute_action_blocks(RoomActionBlock *action_blocks, size_t count) {
	for (size_t i = 0; i < count; i++) {
		RoomActionBlock *action_block = &action_blocks[i];

		if (!match_conditions(action_block->conditions, action_block->condition_count)) {
			continue;
		}

		if (execute_actions(action_block->actions, action_block->action_count, 0)) {
			break;
		}
	}
}

// Callback passed to game_show_message(), game_show_image(),
// game_wait_for_sfx() and game_timeline_start(): resumes the suspended block. Only the rest of that block runs; following ACTION
// blocks are not resumed.
static void continue_actions(void) {
	RoomAction *actions = pending_actions.actions;
	size_t count = pending_actions.count;
	size_t next = pending_actions.next;
	pending_actions_set(NULL, 0, 0);
	if (room && actions) {
		execute_actions(actions, count, next);
	}
}

bool room_choice_option_is_available(const ChoiceOption *option) {
	if (!choice_depth) {
		return false;
	}
	return match_conditions(option->block.conditions, option->block.condition_count);
}

void room_choice_select(size_t index) {
	if (!choice_depth) {
		return;
	}
	Choice *choice = choice_stack[choice_depth - 1].choice;
	if (index >= choice->option_count) {
		return;
	}
	ChoiceOption *option = &choice->options[index];
	if (!room_choice_option_is_available(option)) {
		return;
	}
	execute_actions(option->block.actions, option->block.action_count, 0);
}

// A path is usable if it was declared in the script and its conditions match.
static bool path_is_available(Path *path) {
	if (!path->exists) {
		return false;
	}
	return match_conditions(path->conditions, path->condition_count);
}

static void path_execute(Path *path) {
	if (path_is_available(path)) {
		execute_action_blocks(path->action_blocks,path->action_block_count);
	}
}

void room_move_north(void) {
	if (!room) {
		return;
	}
	path_execute(&room->north);
}

void room_move_northeast(void) {
	if (!room) {
		return;
	}
	path_execute(&room->northeast);
}

void room_move_east(void) {
	if (!room) {
		return;
	}
	path_execute(&room->east);
}

void room_move_southeast(void) {
	if (!room) {
		return;
	}
	path_execute(&room->southeast);
}

void room_move_south(void) {
	if (!room) {
		return;
	}
	path_execute(&room->south);
}

void room_move_southwest(void) {
	if (!room) {
		return;
	}
	path_execute(&room->southwest);
}

void room_move_west(void) {
	if (!room) {
		return;
	}
	path_execute(&room->west);
}

void room_move_northwest(void) {
	if (!room) {
		return;
	}
	path_execute(&room->northwest);
}

bool room_execute_hotspot_use(Hotspot *hotspot, const char *id) {
	for (size_t i = 0; i < hotspot->use_count; i++) {
		RoomUse *use = &hotspot->uses[i];
		if (strcmp(use->item, "*") != 0 && strcmp(use->item, id) != 0) {
			continue;
		}
		if (!match_conditions(use->conditions, use->condition_count)) {
			continue;
		}
		// USE blocks are alternatives: only the first match runs.
		execute_actions(use->actions, use->action_count, 0);
		return true;
	}

	return false;
}

void room_execute_hotspot_action(Hotspot *hotspot) {
	execute_action_blocks(hotspot->action_blocks,hotspot->action_block_count);
}

bool room_hotspot_is_available(Hotspot *hotspot) {
	if (!hotspot) {
		return false;
	}
	return match_conditions(hotspot->conditions, hotspot->condition_count);
}

Hotspot *room_find_hotspot(int x, int y) {
	if (!room) {
		return NULL;
	}
	for (size_t i = 0; i < room->hotspot_count; i++) {
		Hotspot *hotspot = &room->hotspots[i];

		if (!room_hotspot_is_available(hotspot)) {
			continue;
		}

		if (x >= hotspot->x && x < hotspot->x + hotspot->width && y >= hotspot->y && y < hotspot->y + hotspot->height) {
			return hotspot;
		}
	}
	return NULL;
}

Hotspot *room_find_hotspot_by_id(const char *id) {
	if (!room) {
		return NULL;
	}
	for (size_t i = 0; i < room->hotspot_count; i++) {
		Hotspot *hotspot = &room->hotspots[i];

		if (strcmp(hotspot->id, id) == 0 && room_hotspot_is_available(hotspot)) {
			return hotspot;
		}
	}

	return NULL;
}

bool room_can_move_north(void) {
	return room && path_is_available(&room->north);
}

bool room_can_move_south(void) {
	return room && path_is_available(&room->south);
}

bool room_can_move_east(void) {
	return room && path_is_available(&room->east);
}

bool room_can_move_west(void) {
	return room && path_is_available(&room->west);
}

bool room_can_move_northwest(void) {
	return room && path_is_available(&room->northwest);
}

bool room_can_move_southwest(void) {
	return room && path_is_available(&room->southwest);
}

bool room_can_move_northeast(void) {
	return room && path_is_available(&room->northeast);
}

bool room_can_move_southeast(void) {
	return room && path_is_available(&room->southeast);
}

bool room_init(const char *name) {
	char path[256];
	if (room) {
		return false;
	}

	room = calloc(1, sizeof(Room));
	if (!room) {
		return false;
	}

	snprintf(path, sizeof(path), "romfs:/rooms/%s", name);
	room->path = strdup(path);
	if (!gfxmap_load_assets(path, &room->assets)) {
		printf("Cannot load room assets\n");
		room_close();
		return false;
	}
	snprintf(path, sizeof(path), "romfs:/rooms/%s/room", name);
	if (!load_room(path)) {
		printf("Cannot load rooms: %s\n", room->path);
		room_close();
		return false;
	}
	printf("entering Room: %s\n", name);
	return true;
}

void room_draw(void) {
	if (!room) {
		return;
	}
	for (size_t i = 0; i < room->image_count; i++) {
		RoomImage *image = &room->images[i];
		if (match_conditions(image->conditions, image->condition_count)) {
			C2D_DrawImageAt(image->image, image->x, image->y, image->z, NULL, 1.0f, 1.0f);
		}
	}
}

void room_close(void) {
	if (!room) {
		return;
	}

	// A suspended block points into this room's memory or its choice file;
	// an open choice is dropped with its file.
	pending_actions_set(NULL, 0, 0);
	choice_close();
	for (size_t i = 0; i < room->image_count; i++) {
		RoomImage *image = &room->images[i];
		for (size_t j = 0; j < image->condition_count; j++) {
			condition_remove(&image->conditions[j]);
		}
	}

	for (size_t i = 0; i < room->hotspot_count; i++) {
		Hotspot *hotspot = &room->hotspots[i];

		free(hotspot->id);
		free(hotspot->message_id);

		for (size_t j = 0; j < hotspot->condition_count; j++) {
			condition_remove(&hotspot->conditions[j]);
		}

		for (size_t j = 0; j < hotspot->action_block_count; j++) {
			action_block_remove(&hotspot->action_blocks[j]);
		}

		for (size_t j = 0; j < hotspot->use_count; j++) {
			RoomUse *use = &hotspot->uses[j];
			free(use->item);
			for (size_t k = 0; k < use->condition_count; k++) {
				condition_remove(&use->conditions[k]);
			}
			for (size_t k = 0; k < use->action_count; k++) {
				action_remove(&use->actions[k]);
			}
		}
	}

	Path *paths[] = {
		&room->north,
		&room->south,
		&room->east,
		&room->west,
		&room->northeast,
		&room->southeast,
		&room->northwest,
		&room->southwest,
	};

	for (size_t i = 0; i < 8; i++) {
		Path *path = paths[i];

		for (size_t j = 0; j < path->condition_count; j++) {
			condition_remove(&path->conditions[j]);
		}

		for (size_t j = 0; j < path->action_block_count; j++) {
			action_block_remove(&path->action_blocks[j]);
		}
	}
	free(room->path);
	gfxmap_free_assets(&room->assets);
	free(room);
	room = NULL;
}
