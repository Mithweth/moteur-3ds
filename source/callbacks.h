// callbacks.h
// Registry of the game-specific code in extensions/: inventory callbacks
// (referenced by EXAMINE_CALLBACK / USE_CALLBACK in the inventory file) and
// mini-games (started by the MINIGAME room action). Implemented in
// extensions/callbacks.c; add new entries to its tables.
#pragma once

#include "game.h"

// Which inventory directive may reference an entry. EXAMINE is the zero value,
// so it is the default for an entry that does not set .type.
typedef enum {
	INVENTORY_CALLBACK_EXAMINE,     // EXAMINE_CALLBACK: draws every frame while examining
	INVENTORY_CALLBACK_USE          // USE_CALLBACK: logic only, called outside of any frame
} InventoryCallbackType;

typedef struct {
	const char *name;
	InventoryCallbackType type;
	void (*init)(void);     // once per session (allocations)
	void (*close)(void);    // once at exit (frees what init allocated)
	void (*reset)(void);    // at the start of every game (game state)
	void (*callback)(void);
	char* (*serialize)(void);
	void (*deserialize)(const char*);
} InventoryCallback;

// Runs the init function of every inventory callback that has one (one-time
// allocations). Called once by game_init.
void callbacks_init(void);

// Runs the close function of every inventory callback that has one, to free
// what init allocated. Called once by game_close, even if game_init failed
// or was never called: close must cope with a missing or partial init.
void callbacks_close(void);

// Runs the reset function of every inventory callback that has one (e.g.
// draws a new secret code). Called by game_start at the start of each game.
void callbacks_reset(void);

// Returns the inventory callback registered under name with the given type,
// or NULL (and logs it) if there is none. Two entries may share a name if
// their types differ.
void (*callbacks_inventory_find(const char *name, InventoryCallbackType type))(void);

// Returns the mini-game registered under name, or NULL (and logs it) if there
// is none. The returned pointer refers to a static object: do not free it.
MiniGame *callbacks_minigame_find(const char *name);

size_t callbacks_get_count(void);
const InventoryCallback *callbacks_get_index(size_t index);