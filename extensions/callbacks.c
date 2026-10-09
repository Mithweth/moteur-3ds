// callbacks.c

#include <stdio.h>
#include <string.h>
#include "callbacks.h"
#include "game.h"

typedef struct {
    const char *name;
    MiniGame *minigame;
} MiniGameCallback;

static MiniGameCallback minigame_callbacks[] = {};

static InventoryCallback inventory_callbacks[] = {};

static const size_t minigame_callback_count = sizeof(minigame_callbacks) / sizeof(minigame_callbacks[0]);
static const size_t inventory_callback_count = sizeof(inventory_callbacks) / sizeof(inventory_callbacks[0]);

size_t callbacks_get_count(void) {
    return inventory_callback_count;
}
const InventoryCallback *callbacks_get_index(size_t index) {
    if (index >= inventory_callback_count) {
        return NULL;
    }
    return &inventory_callbacks[index];
}

void (*callbacks_inventory_find(const char *name, InventoryCallbackType type))(void) {
    for (size_t i = 0; i < inventory_callback_count; i++) {
        InventoryCallback *cb = &inventory_callbacks[i];
        if (strcmp(cb->name, name) == 0 && cb->type == type) {
            return cb->callback;
        }
    }
    printf("Callback not found: %s\n", name);
    return NULL;
}

MiniGame *callbacks_minigame_find(const char *name) {
    for (size_t i = 0; i < minigame_callback_count; i++) {
        MiniGameCallback *cb = &minigame_callbacks[i];

        if (strcmp(cb->name, name) == 0) {
            return cb->minigame;
        }
    }
    printf("Callback not found: %s\n", name);
    return NULL;
}

void callbacks_init(void) {
    for (size_t i = 0; i < inventory_callback_count; i++) {
        InventoryCallback *cb = &inventory_callbacks[i];
        if (cb->init) {
            cb->init();
        }
    }
}

void callbacks_close(void) {
    for (size_t i = 0; i < inventory_callback_count; i++) {
        InventoryCallback *cb = &inventory_callbacks[i];
        if (cb->close) {
            cb->close();
        }
    }
}

void callbacks_reset(void) {
    for (size_t i = 0; i < inventory_callback_count; i++) {
        InventoryCallback *cb = &inventory_callbacks[i];
        if (cb->reset) {
            cb->reset();
        }
    }
}
