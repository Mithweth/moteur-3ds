// room.h
// Room engine: loads one room at a time (images, touch hotspots and the
// eight movement paths) from romfs:/rooms/<name>/, draws it on the bottom
// screen and runs the scripted actions attached to hotspots and paths.
// Only one room is loaded at a time; game_set_room() closes the current one
// and initializes the next. The room script format is documented in
// docs/ROOMS.en.md.
// A CHOICE action opens a choice file stored next to the room script
// (docs/CHOICES.en.md); room.c keeps the stack of open choices and runs the
// option the player picks, game.c draws the choice and reads the input.
#pragma once

#include <citro2d.h>
#include <stdbool.h>
#include <stddef.h>
#include "gfxmap.h"

// Fixed capacities of the room data structures. Exceeding any of them is
// reported as a load error with the offending file and line.
#define ROOM_MAX_IMAGES          64
#define ROOM_MAX_HOTSPOTS        64
#define ROOM_MAX_CONDITIONS       8
#define ROOM_MAX_ACTION_BLOCKS    8
#define ROOM_MAX_ACTIONS         16
#define ROOM_MAX_USES             8

// Capacities of a choice file (see docs/CHOICES.en.md): options per CHOICE
// block, CHOICE blocks per file, and how many choices can be stacked by
// nested CHOICE actions.
#define CHOICE_MAX_OPTIONS       16
#define CHOICE_MAX_CHOICES       16
#define CHOICE_MAX_DEPTH          8

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

// ACTION block of a hotspot or path: its actions run when all its
// conditions match.
typedef struct {
	RoomCondition conditions[ROOM_MAX_CONDITIONS];
	size_t condition_count;
	RoomAction actions[ROOM_MAX_ACTIONS];
	size_t action_count;
} RoomActionBlock;

// USE block of a hotspot: runs when the player uses a matching item on it.
typedef struct {
	char *item;      // inventory item id, or "*" to match any item
	RoomCondition conditions[ROOM_MAX_CONDITIONS];
	size_t condition_count;
	RoomAction actions[ROOM_MAX_ACTIONS];
	size_t action_count;
} RoomUse;

// Image drawn on the bottom screen while its conditions match.
typedef struct {
	C2D_Image image;
	float x;
	float y;
	float z;         // depth passed to C2D_DrawImageAt()
	RoomCondition conditions[ROOM_MAX_CONDITIONS];
	size_t condition_count;
} RoomImage;

// Touch area of the bottom screen. A hotspot whose conditions don't match
// is ignored entirely (not touchable, not targetable).
typedef struct {
	int x;
	int y;
	int width;
	int height;
	char *id;          // also used as the lang key of the HUD target name
	char *message_id;  // optional lang key shown on the first touch, or NULL
	C2D_Image message_image;  // optional image shown on the first touch, ignored if message_id is set
	RoomCondition conditions[ROOM_MAX_CONDITIONS];
	size_t condition_count;
	RoomActionBlock action_blocks[ROOM_MAX_ACTION_BLOCKS];
	size_t action_block_count;
	RoomUse uses[ROOM_MAX_USES];
	size_t use_count;
} Hotspot;

// Movement in one direction. Available when declared (exists) and all its
// conditions match; moving runs its ACTION blocks (usually a ROOM action).
typedef struct {
	bool exists;
	RoomCondition conditions[ROOM_MAX_CONDITIONS];
	size_t condition_count;
	RoomActionBlock action_blocks[ROOM_MAX_ACTION_BLOCKS];
	size_t action_block_count;
} Path;

// A loaded room. All strings and the spritesheet are owned by the room and
// freed by room_close().
typedef struct {
	char *path;      // romfs:/rooms/<name>, base directory for SFX files
	GfxAssets assets;
	RoomImage images[ROOM_MAX_IMAGES];
	size_t image_count;
	Hotspot hotspots[ROOM_MAX_HOTSPOTS];
	size_t hotspot_count;
	Path north;
	Path northwest;
	Path south;
	Path southwest;
	Path east;
	Path northeast;
	Path west;
	Path southeast;
} Room;

// One OPTION of a choice: name is the translation key of its button. Its
// WHEN conditions decide whether the button is shown, its actions run when
// the player picks it.
typedef struct {
	char *name;
	RoomActionBlock block;
} ChoiceOption;

// One CHOICE block of a choice file: name identifies it within the file
// (MAIN is the entry point), prompt is the translation key of the question.
// Also used by game.c for the START menu, whose fields are static strings.
typedef struct {
	char *name;
	char *prompt;
	ChoiceOption options[CHOICE_MAX_OPTIONS];
	size_t option_count;
} Choice;

// Move in the given direction: runs the path's ACTION blocks if the path is
// available. Does nothing when no room is loaded.
void room_move_north(void);
void room_move_northeast(void);
void room_move_east(void);
void room_move_southeast(void);
void room_move_south(void);
void room_move_southwest(void);
void room_move_west(void);
void room_move_northwest(void);
// Use inventory item `id` on `hotspot`: runs the first USE block whose item
// matches (`id` or "*") and whose conditions match. Returns false if none did.
bool room_execute_hotspot_use(Hotspot *hotspot, const char *id);
// Run every ACTION block of `hotspot` whose conditions match, in order.
void room_execute_hotspot_action(Hotspot *hotspot);
// True if `hotspot` is non-NULL and all its own conditions match.
bool room_hotspot_is_available(Hotspot *hotspot);
// True if a room is loaded and the path in that direction is available.
// Used by the HUD to draw the direction arrows.
bool room_can_move_north(void);
bool room_can_move_northeast(void);
bool room_can_move_east(void);
bool room_can_move_southeast(void);
bool room_can_move_south(void);
bool room_can_move_southwest(void);
bool room_can_move_west(void);
bool room_can_move_northwest(void);
// Load romfs:/rooms/<name>/ (gfx.t3x, gfx.h and the `room` script).
// Returns false if a room is already loaded or on any load error; on error
// everything allocated so far is freed. Call room_close() first.
bool room_init(const char *name);
// Draw the room images whose conditions match. Call during a bottom-screen
// scene. Does nothing when no room is loaded.
void room_draw(void);
// Free the current room and drop the rest of a suspended action block.
// Invalidates every Hotspot pointer. Safe to call when no room is loaded.
void room_close(void);
// First available hotspot (in declaration order) containing the point, or
// NULL. The pointer stays valid until room_close().
Hotspot *room_find_hotspot(int x, int y);
// First available hotspot with this id, or NULL. Several hotspots may share
// an id (e.g. a closed and an open door with opposite conditions).
Hotspot *room_find_hotspot_by_id(const char *id);

// True if a choice is open and the WHEN conditions of option match, i.e.
// its button should be shown.
bool room_choice_option_is_available(const ChoiceOption *option);
// Runs the actions of option index of the innermost open choice. Does
// nothing if no choice is open, the index is out of range or the option is
// not available. The choice stays open unless the actions leave it.
void room_choice_select(size_t index);
// Closes the innermost open choice, as EXIT_CHOICE does: its parent choice
// is shown again (or the choice file is freed for the outermost one) and the
// actions that followed its CHOICE action resume. Used for the B button.
// Does nothing if no choice is open.
void room_choice_exit(void);
