// inventory.c
// Implementation notes: items[] is the catalogue parsed from the inventory
// file; inventory[] holds pointers into it for the items the player carries,
// in pickup order. Images are resolved while parsing because gfxmap keeps a
// single global index table, overwritten by the next gfxmap_load (room,
// timeline, mini-game...).
#include <citro2d.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inventory.h"
#include "game.h"
#include "lang.h"
#include "gfxmap.h"
#include "callbacks.h"
#include "str_utils.h"

#define ITEM_MAX       64

static Item items[ITEM_MAX];
static size_t item_count = 0;
static size_t selected = 0;

static Item *inventory[ITEM_MAX];
static size_t inventory_count = 0;

static C2D_SpriteSheet assets;
static InventoryMode inventory_mode = INVENTORY_NORMAL;
static size_t columns = 6;

// Looks an item up in the catalogue, whether the player holds it or not.
static Item *inventory_find(const char *id) {
    for (size_t i = 0; i < item_count; i++) {
        if (strcmp(items[i].id, id) == 0) {
            return &items[i];
        }
    }

    return NULL;
}

// Appends a catalogue entry; returns NULL when the catalogue is full.
static Item *add_item(const char *id, const char *name_id) {
    if (item_count >= ITEM_MAX) {
        return NULL;
    }
    Item *item = &items[item_count++];
    memset(item, 0, sizeof(*item));
    item->id = strdup(id);
    item->name_id = strdup(name_id);
    return item;
}

// Parses the ITEM <id> <name_id> ... END_ITEM blocks of the catalogue file.
// On error, logs file:line and returns false with an empty catalogue.
static bool load_inventory(const char *filename) {
    FILE *f = fopen(filename, "r");

    if (!f) {
        printf("Cannot open inventory: %s\n", filename);
        return false;
    }

    item_count = 0;

    char line[256];
    size_t line_number = 0;
    Item *item = NULL;

    while (fgets(line, sizeof(line), f)) {
        line_number++;
        char *p = str_trim(line);

        if (*p == '\0' || *p == '#') {
            continue;
        }

        char *command = strtok(p, " ");

        if (!command) {
            continue;
        }

        if (item) {
            if (strcmp(command, "IMAGE") == 0) {
                char *image_id = strtok(NULL, " ");
                if (!image_id) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    item_count = 0;
                    return false;
                }
                item->image = gfxmap_get_image(assets, image_id);
                if (!item->image.tex) {
                    printf("%s:%zu: unknown image: %s\n", filename, line_number, image_id);
                    fclose(f);
                    item_count = 0;
                    return false;
                }
                continue;
            }
            if (strcmp(command, "EXAMINE") == 0) {
                char *text = strtok(NULL, " ");
                if (!text) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    item_count = 0;
                    return false;
                }
                item->examine_text = strdup(text);
                continue;
            }
            if (strcmp(command, "EXAMINE_CALLBACK") == 0) {
                char *cb = strtok(NULL, " ");
                if (!cb) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    item_count = 0;
                    return false;
                }
                item->examine_callback = callbacks_inventory_find(cb);
                if (!item->examine_callback) {
                    printf("%s:%zu: unknown callback: %s\n", filename, line_number, cb);
                    fclose(f);
                    item_count = 0;
                    return false;
                }
                continue;
            }
            if (strcmp(command, "USE_CALLBACK") == 0) {
                char *cb = strtok(NULL, " ");
                if (!cb) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    item_count = 0;
                    return false;
                }
                item->use_callback = callbacks_inventory_find(cb);
                if (!item->use_callback) {
                    printf("%s:%zu: unknown callback: %s\n", filename, line_number, cb);
                    fclose(f);
                    item_count = 0;
                    return false;
                }
                continue;
            }
            // DETAIL <image> <x> <y> [FULLSCREEN]
            if (strcmp(command, "DETAIL") == 0) {
                char *image_id = strtok(NULL, " ");
                if (!image_id) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    item_count = 0;
                    return false;
                }

                char *x  = strtok(NULL, " ");
                char *y  = strtok(NULL, " ");
                char *fmt = strtok(NULL, " ");

                if ((!x) || (!y)) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    item_count = 0;
                    return false;
                }
                if ((fmt) && (strcmp(fmt, "FULLSCREEN") == 0)) {
                    item->detail_fullscreen = true;
                }
                item->detail_image = gfxmap_get_image(assets, image_id);

                if (!item->detail_image.tex) {
                    printf("%s:%zu: unknown image: %s\n", filename, line_number, image_id);
                    fclose(f);
                    item_count = 0;
                    return false;
                }
                item->detail_x = atof(x);
                item->detail_y = atof(y);
                continue;
            }
            if (strcmp(command, "END_ITEM") == 0) {
                if (!item->image.tex) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    item_count = 0;
                    return false;
                }
                item = NULL;
                continue;
            }
            printf("%s:%zu: syntax error: %s\n", filename, line_number, command);
            fclose(f);
            item_count = 0;
            return false;
        }
        if (strcmp(command, "ITEM") == 0) {
            char *item_id = strtok(NULL, " ");
            char *name_id = strtok(NULL, " ");
            if ((!item_id) || (!name_id)) {
                printf("%s:%zu: syntax error\n", filename, line_number);
                fclose(f);
                item_count = 0;
                return false;
            }
            if (inventory_find(item_id)) {
                printf("%s:%zu: already exists: %s\n", filename, line_number, item_id);
                fclose(f);
                item_count = 0;
                return false;
            }
            item = add_item(item_id, name_id);
            if (!item) {
                printf("%s:%zu: too many items\n", filename, line_number);
                fclose(f);
                item_count = 0;
                return false;
            }
            continue;
        }
        printf("%s:%zu: syntax error\n", filename, line_number);
        fclose(f);
        item_count = 0;
        return false;
    }

    if (item) {
        printf("%s:%zu: missing END_ITEM\n", filename, line_number);
        fclose(f);
        item_count = 0;
        return false;
    }

    fclose(f);
    printf("Loaded %zu inventory items\n", item_count);
    return true;
}

void inventory_set_columns(size_t value) {
    columns = value;
}

bool inventory_update(u32 keys) {
    if (inventory_mode == INVENTORY_ACTION) {
// While examining, every key is swallowed; only X or B leave the view.
        if ((keys & KEY_X) || (keys & KEY_B)) {
            inventory_mode = INVENTORY_NORMAL;
        }
        return true;
    }

    if (inventory_count == 0) {
        return false;
    }

    if (keys & KEY_DLEFT) {
        if (selected == 0) {
            selected = inventory_count - 1;
        } else {
            selected--;
        }
        return true;
    }

    if (keys & KEY_DRIGHT) {
        selected++;
        if (selected >= inventory_count) {
            selected = 0;
        }
        return true;
    }


// Up/down move by one grid row, clamped to the first/last item.
    if (keys & KEY_DDOWN) {
        selected += columns;

        if (selected >= inventory_count) {
            selected = inventory_count - 1;
        }
        return true;
    }

    if (keys & KEY_DUP) {
        if (selected < columns) {
            selected = 0;
        } else {
            selected -= columns;
        }
        return true;
    }

    Item *item = inventory[selected];
    if (keys & KEY_X) {    
        if (item->detail_image.tex || item->examine_text || item->examine_callback) {
            inventory_mode = INVENTORY_ACTION;
        }
        return true;
    }

// A uses the selected item: its use_callback if it has one, otherwise the
// USE blocks of the current target (game_use_item).
    if (keys & KEY_A) {
        if (item && item->use_callback) {
            item->use_callback();
            return true;
        }
        game_use_item(item->id);
        return true;
    }
    return false;
}

const Item *inventory_get_selected_item(void) {
    if (inventory_count == 0) {
        return NULL;
    }
    return inventory[selected];
}

const Item *inventory_get_item(size_t id) {
    if (inventory_count == 0) {
        return NULL;
    }
    if (id >= inventory_count) {
        return NULL;
    }
    return inventory[id];
}

int inventory_get_selected(void) {
    return selected;
}

int inventory_get_count(void) {
    return inventory_count;
}

bool inventory_is_active(void) {
    return inventory_mode == INVENTORY_ACTION;
}

void inventory_reset(void) {
    inventory_count = 0;
    selected = 0;
    inventory_mode = INVENTORY_NORMAL;
}

bool inventory_has(const char *id) {
    Item *item = inventory_find(id);

    if (!item) {
        return false;
    }

    for (size_t i = 0; i < inventory_count; i++) {
        if (inventory[i] == item)
            return true;
    }

    return false;
}

void inventory_add(const char *id) {
    Item *item = inventory_find(id);

    if (!item) {
        printf("Unknown inventory item: %s\n", id);
        return;
    }

    if (inventory_count >= ITEM_MAX) {
        return;
    }

    if (inventory_has(id)) {
        return;
    }
    printf("Inventory: add %s\n", id);
    inventory[inventory_count++] = item;
    selected = inventory_count - 1;
}

void inventory_remove(const char *id) {
    Item *item = inventory_find(id);

    if (!item) {
        return;
    }

    for (size_t i = 0; i < inventory_count; i++) {
        if (inventory[i] != item) {
            continue;
        }

        for (size_t j = i; j < inventory_count - 1; j++)
            inventory[j] = inventory[j + 1];

        inventory_count--;

        if (inventory_count == 0) {
            selected = 0;
        } else if (selected >= inventory_count) {
            selected = inventory_count - 1;
        }
        return;
    }
}

bool inventory_init(void) {
    if (!gfxmap_load_assets("romfs:/inventory", &assets)) {
        printf("Cannot load inventory\n");
        assets = NULL;
        return false;
    }

    if (!load_inventory("romfs:/inventory/inventory")) {
        printf("Cannot load inventory\n");
        C2D_SpriteSheetFree(assets);
        assets = NULL;
        return false;
    }

    printf("Loaded %zu items\n", item_count);
    inventory_count = 0;
    selected = 0;

    return true;
}

void inventory_close(void) {
    for (size_t i = 0; i < item_count; i++) {
        free(items[i].id);
        free(items[i].name_id);
        free(items[i].examine_text);
    }
// Resetting item_count makes a second call harmless.
    item_count = 0;
    if (assets) {
        C2D_SpriteSheetFree(assets);
        assets = NULL;
    }
    inventory_count = 0;
    selected = 0;
}
