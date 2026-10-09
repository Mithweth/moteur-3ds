// gamestate.h
// Named boolean flags describing the game's progress (doors opened, items
// placed...). The list of flags and their types is declared once in
// romfs:/states/game.state, loaded at startup by main(). All flags start
// false and are reset to false at the start of each new game.

#pragma once

#include <stdbool.h>
#include <stddef.h>

// How gamestate_set() changes a flag.
typedef enum {
    GAMESTATE_TOGGLE,  // every set inverts the value
    GAMESTATE_KEEP     // first set makes it true; later sets do nothing
} GameStateType;

typedef struct {
    char *name;        // owned, strdup'd
    GameStateType type;
    bool value;
} GameState;

// Load the flag declarations ("<name> TOGGLE|KEEP" per line, '#' comments)
// from `filename`. Returns false on I/O error, syntax error, unknown type,
// duplicate name or too many flags. Call once at startup.
bool gamestate_init(const char *filename);
// Free all flags. Call once at exit.
void gamestate_close(void);
// Current value of a flag. Unknown names print a warning and return false.
bool gamestate_get(const char *name);
// Change a flag according to its type: TOGGLE inverts it, KEEP sets it to
// true. Note that "set" does not mean "set to true" for TOGGLE flags.
// Unknown names print a warning and are ignored.
void gamestate_set(const char *name);
// Reset every flag to false (new game). Keeps the declarations.
void gamestate_reset(void);

size_t gamestate_get_count(void);
const GameState *gamestate_get_index(size_t index);
