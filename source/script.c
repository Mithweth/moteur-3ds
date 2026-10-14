// script.c
// Shared DSL conditions, action execution, and suspended action callbacks.
// A room owns the actions; a choice owns its own option actions. script.c
// borrows those arrays only until the current room is closed.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "script.h"
#include "room.h"
#include "choice.h"
#include "game.h"
#include "gamestate.h"
#include "inventory.h"
#include "audio.h"
#include "str_utils.h"

// --------- SUSPENDED ACTION STATE

typedef struct {
	RoomAction *actions;
	size_t count;
	size_t next;
} PendingActions;

static PendingActions pending_actions;

// --------- PARSING AND CLEANUP HELPERS

RoomCondition script_condition_add(const char *type, const char *name, const char *value) {
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

void script_condition_remove(RoomCondition *condition) {
	free(condition->name);
}

RoomAction script_action_add(const char *command, const char *argument) {
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

void script_action_remove(RoomAction *action) {
	free(action->argument);
}

void script_action_block_remove(RoomActionBlock *block) {
	for (size_t i = 0; i < block->condition_count; i++) {
		script_condition_remove(&block->conditions[i]);
	}

	for (size_t i = 0; i < block->action_count; i++) {
		script_action_remove(&block->actions[i]);
	}
}

// --------- CONDITION EVALUATION

bool script_match_conditions(const RoomCondition *conditions, size_t count) {
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

// --------- ACTION RESUMPTION

static void script_pending_set(RoomAction *actions, size_t count, size_t next) {
	pending_actions.actions = actions;
	pending_actions.count = count;
	pending_actions.next = next;
}

// Resume the remaining actions after MESSAGE, WAIT_SFX or TIMELINE RETURN.
static void script_continue_actions(void) {
	RoomAction *actions = pending_actions.actions;
	size_t count = pending_actions.count;
	size_t next = pending_actions.next;
	script_pending_set(NULL, 0, 0);
	if (room_is_loaded() && actions) {
		script_execute_actions(actions, count, next);
	}
}

// --------- ACTION EXECUTION

// Return true when an action changes modes or suspends execution. The caller
// must not touch the old action array after ROOM frees its owner.
bool script_execute_actions(RoomAction *actions, size_t count, size_t start) {
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
			script_pending_set(actions, count, c + 1);
			game_show_message(action->argument, script_continue_actions);
			return true;
		case ROOM_ACTION_MESSAGE_IMAGE:
			script_pending_set(actions, count, c + 1);
			game_show_image(action->image, script_continue_actions);
			return true;
		case ROOM_ACTION_SFX:
			path = audio_resolve_path(room_get_path(), action->argument, ".raw");
			if (path) {
				sfx_play(path);
				free(path);
			}
			break;
		case ROOM_ACTION_WAIT_SFX:
			// sfx_play() loads the whole sample, so the path can be freed as
			// soon as game_wait_for_sfx() returns.
			path = audio_resolve_path(room_get_path(), action->argument, ".raw");
			script_pending_set(actions, count, c + 1);
			// Save where to resume before handing control to game.c; if the
			// sound can't be played, forget it and keep going immediately.
			if (path && game_wait_for_sfx(path, script_continue_actions)) {
				free(path);
				return true;
			}
			free(path);
			script_pending_set(NULL, 0, 0);
			break;
		case ROOM_ACTION_ROOM:
			// Frees the current room: `actions` must not be touched after this.
			game_set_room(action->argument);
			return true;
		case ROOM_ACTION_TIMELINE:
			script_pending_set(actions, count, c + 1);
			// On failure, game_timeline_start goes back to the title screen,
			// whose room_close clears pending_actions: return either way.
			game_timeline_start(action->argument, script_continue_actions);
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
			if (choice_has_open()) {
				choice_pop();
				return true;
			}
			break;
		}
	}
	return false;
}

void script_execute_action_blocks(RoomActionBlock *action_blocks, size_t count) {
	for (size_t i = 0; i < count; i++) {
		RoomActionBlock *action_block = &action_blocks[i];

		if (!script_match_conditions(action_block->conditions, action_block->condition_count)) {
			continue;
		}

		if (script_execute_actions(action_block->actions, action_block->action_count, 0)) {
			break;
		}
	}
}

// --------- INITIALIZATION AND CLEANUP

void script_reset(void) {
	script_pending_set(NULL, 0, 0);
}
