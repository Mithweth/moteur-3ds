// script.h
// Shared WHEN conditions and ACTION directives used by rooms and choices.
#pragma once

#include <citro2d.h>
#include <stdbool.h>
#include <stddef.h>

#define ROOM_MAX_CONDITIONS 8
#define ROOM_MAX_ACTIONS    16

// Kind of test performed by a WHEN directive.
typedef enum {
	ROOM_CONDITION_STATE_IS,       // gamestate_get(name) == expected
	ROOM_CONDITION_INVENTORY_HAS   // inventory_has(name) == expected
} RoomConditionType;

// One WHEN directive. A list of conditions matches only if all of them do.
typedef struct {
	RoomConditionType type;
	char *name;      // gamestate name or inventory item id (owned, strdup'd)
	bool expected;   // parsed with str_to_bool(): only "true" is true
} RoomCondition;

// Action directives allowed inside ACTION and USE blocks.
// ROOM and MINIGAME change the game mode and end the action flow. MESSAGE,
// MESSAGE_IMAGE, WAIT_SFX and TIMELINE suspend it: the rest of the block
// resumes once the message is dismissed, the sound is over, or the timeline
// ends with RETURN.
typedef enum {
	ROOM_ACTION_SET,
	ROOM_ACTION_INVENTORY_ADD,
	ROOM_ACTION_INVENTORY_REMOVE,
	ROOM_ACTION_MESSAGE,
	ROOM_ACTION_MESSAGE_IMAGE,
	ROOM_ACTION_SFX,
	ROOM_ACTION_WAIT_SFX,
	ROOM_ACTION_ROOM,
	ROOM_ACTION_TIMELINE,
	ROOM_ACTION_CHOICE,
	ROOM_ACTION_EXIT_CHOICE,
	ROOM_ACTION_MINIGAME
} RoomActionType;

typedef struct {
	RoomActionType type;
	char *argument;  // single argument of the directive (owned, strdup'd)
	C2D_Image image; // MESSAGE_IMAGE only: resolved while parsing from the
	                 // room's spritesheet
} RoomAction;

// ACTION block of a hotspot, path or CHOICE option: its actions run when all its
// conditions match.
typedef struct {
	RoomCondition conditions[ROOM_MAX_CONDITIONS];
	size_t condition_count;
	RoomAction actions[ROOM_MAX_ACTIONS];
	size_t action_count;
} RoomActionBlock;

// Parsing helpers return structures by value. Free the owned strings later.
RoomCondition script_condition_add(const char *type, const char *name, const char *value);
void script_condition_remove(RoomCondition *condition);
RoomAction script_action_add(const char *command, const char *argument);
void script_action_remove(RoomAction *action);
void script_action_block_remove(RoomActionBlock *block);

bool script_match_conditions(const RoomCondition *conditions, size_t count);
// Returns true if execution was interrupted or handed over to another mode.
bool script_execute_actions(RoomAction *actions, size_t count, size_t start);
void script_execute_action_blocks(RoomActionBlock *blocks, size_t count);
// Drop references to actions owned by the room being closed.
void script_reset(void);
