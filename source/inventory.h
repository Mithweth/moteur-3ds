// inventory.h
// Item catalogue (loaded from romfs:/inventory/inventory) and the player's
// inventory. Drawn by the HUD (hud.c). inventory_init is called once by
// game_init, inventory_close by game_close.
#pragma once

#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>

// One catalogue entry. Strings are owned by the inventory module.
typedef struct {
    char *id;               // identifier used by room scripts (INVENTORY_ADD, USE...)
    char *name_id;          // lang key of the name shown in the HUD
    char *examine_text;     // lang key shown when examining (X), or NULL
    C2D_Image image;
    C2D_Image detail_image; // image shown when examining, or empty
    bool detail_fullscreen; // black out the whole top screen behind detail_image
    float detail_x;
    float detail_y;
    void (*examine_callback)(void); // extra drawing while examining, or NULL
    void (*use_callback)(void);     // replaces the "use on target" logic, or NULL
} Item;

typedef enum {
    INVENTORY_NORMAL,       // browsing items
    INVENTORY_ACTION        // examining the selected item (X pressed)
} InventoryMode;

// Loads the inventory spritesheet and the item catalogue. Returns false on any
// load or syntax error. Must be called once, after C2D is initialized.
bool inventory_init(void);

// Frees the catalogue and graphics. Safe to call more than once.
void inventory_close(void);

// Adds a catalogue item to the inventory and selects it. Unknown ids are
// logged and ignored; adding an item already held does nothing.
void inventory_add(const char *id);

// Returns true if the player currently holds the item.
bool inventory_has(const char *id);

// Removes the item from the inventory if held; the selection is kept in range.
void inventory_remove(const char *id);

// Handles D-pad navigation, X (examine) and A (use). Returns true when the
// keys were consumed, in which case the caller must not process them further.
bool inventory_update(u32 keys);

// Returns true while an item is being examined.
bool inventory_is_active(void);

// Empties the inventory and leaves examine mode (new game).
void inventory_reset(void);

// Returns the selected item, or NULL if the inventory is empty. The pointer
// stays owned by the inventory module.
const Item *inventory_get_selected_item(void);

// Returns the held item at position id (pickup order), or NULL if id is out
// of range.
const Item *inventory_get_item(size_t id);

// Returns the position of the selected item among the held items (0 when the
// inventory is empty).
int inventory_get_selected(void);

// Returns the number of items the player holds.
int inventory_get_count(void);

// Sets the grid width used by up/down navigation. Called by hud_init with the
// HUD's COLUMNS value.
void inventory_set_columns(size_t value);
