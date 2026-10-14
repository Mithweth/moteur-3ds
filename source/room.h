// room.h
// Room DSL, hotspots, paths, sprites and movement.
// CHOICE handling lives in choice.h; common actions live in script.h.
#pragma once

#include <citro2d.h>
#include <stdbool.h>
#include <stddef.h>
#include "gfxmap.h"
#include "script.h"

#define ROOM_MAX_IMAGES        64
#define ROOM_MAX_HOTSPOTS      64
#define ROOM_MAX_ACTION_BLOCKS 8
#define ROOM_MAX_USES          8

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

// --------- ROOM DATA FOR THE SCRIPT AND CHOICE MODULES

bool room_is_loaded(void);
// Borrowed path, valid only while a room is loaded.
const char *room_get_path(void);
// Image by value: the spritesheet is still owned by room.c.
C2D_Image room_get_image(const char *name);

// --------- MOVEMENT

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

// --------- HOTSPOTS

// Use inventory item `id` on `hotspot`: runs the first USE block whose item
// matches (`id` or "*") and whose conditions match. Returns false if none did.
bool room_execute_hotspot_use(Hotspot *hotspot, const char *id);

// Run every ACTION block of `hotspot` whose conditions match, in order.
void room_execute_hotspot_action(Hotspot *hotspot);

// True if `hotspot` is non-NULL and all its own conditions match.
bool room_hotspot_is_available(Hotspot *hotspot);

// First available hotspot (in declaration order) containing the point, or
// NULL. The pointer stays valid until room_close().
Hotspot *room_find_hotspot(int x, int y);

// First available hotspot with this id, or NULL. Several hotspots may share
// an id (e.g. a closed and an open door with opposite conditions).
Hotspot *room_find_hotspot_by_id(const char *id);

// --------- GRAPHIC FUNCTIONS

// Draw the room images whose conditions match. Call during a bottom-screen
// scene. Does nothing when no room is loaded.
void room_draw(void);

// --------- INITIALIZATION AND CLEANUP

// Load romfs:/rooms/<name>/ (gfx.t3x, gfx.h and the `room` script).
// Returns false if a room is already loaded or on any load error; on error
// everything allocated so far is freed. Call room_close() first.
bool room_init(const char *name);

// Free the current room and drop the rest of a suspended action block.
// Invalidates every Hotspot pointer. Safe to call when no room is loaded.
void room_close(void);
