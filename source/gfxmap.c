// gfxmap.c
// Sprite name -> spritesheet index tables, filled from tex3ds headers. Each
// GfxAssets owns the table of its own header, so loads don't interfere.
#include <citro2d.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gfxmap.h"


// Callers pass the short PNG name; tex3ds names the macros gfx_<name>_idx
// because every header is generated from a file named gfx.t3s.
static int gfxmap_get_index(const GfxAssets *assets, const char *name) {
    char index_name[256];
    snprintf(index_name, sizeof(index_name), "gfx_%s_idx", name);
    for (size_t i = 0; i < assets->count; i++) {
        if (strcmp(assets->indexes[i].name, index_name) == 0) {
            return assets->indexes[i].index;
        }
    }
    return -1;
}

C2D_Image gfxmap_get_image(const GfxAssets *assets, const char *name) {
    int index = gfxmap_get_index(assets, name);

    if (index < 0) {
        printf("Image not found: %s\n", name);
        return (C2D_Image){0};
    }
    return C2D_SpriteSheetGetImage(assets->sheet, index);
}

// Fill assets->indexes from tex3ds header `filename`. On failure the table
// is freed and left empty, never half-filled.
static bool gfxmap_load(const char *filename, GfxAssets *assets) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("Cannot open %s\n", filename);
        return false;
    }

    size_t capacity = 0;
    char line[256];

    while (fgets(line, sizeof(line), f)) {
        char directive[32];
        char name[GFX_NAME_MAX];
        char value[32];

        if (sscanf(line, "%31s %95s %31s", directive, name, value) != 3) {
            continue;
        }

        if (strcmp(directive, "#define") != 0) {
            continue;
        }

        size_t len = strlen(name);

        if (len < 4 || strcmp(name + len - 4, "_idx") != 0) {
            continue;
        }

        if (assets->count == capacity) {
            size_t new_capacity = capacity ? capacity * 2 : 32;
            GfxImageIndex *grown = realloc(assets->indexes, new_capacity * sizeof(*grown));
            if (!grown) {
                printf("Out of memory loading %s\n", filename);
                free(assets->indexes);
                assets->indexes = NULL;
                assets->count = 0;
                fclose(f);
                return false;
            }
            assets->indexes = grown;
            capacity = new_capacity;
        }

        GfxImageIndex *entry = &assets->indexes[assets->count++];

        strcpy(entry->name, name);
        entry->index = atoi(value);
    }
    printf("%s: Loaded %zu images\n", filename, assets->count);
    fclose(f);
    return true;
}

bool gfxmap_load_assets(const char *path, GfxAssets *assets) {
    char filename[256];

    *assets = (GfxAssets){0};
    snprintf(filename, sizeof(filename), "%s/gfx.t3x", path);

    assets->sheet = C2D_SpriteSheetLoad(filename);
    if (!assets->sheet) {
        printf("Cannot load spritesheet: %s\n", filename);
        return false;
    }

    snprintf(filename, sizeof(filename), "%s/gfx.h", path);

    if (!gfxmap_load(filename, assets)) {
        printf("Cannot load gfx headers: %s\n", filename);
        gfxmap_free_assets(assets);
        return false;
    }

    return true;
}

void gfxmap_free_assets(GfxAssets *assets) {
    if (assets->sheet) {
        C2D_SpriteSheetFree(assets->sheet);
    }
    free(assets->indexes);
    *assets = (GfxAssets){0};
}

u32 gfxmap_parse_color(const char *name) {
    if (strcmp(name, "RED") == 0) {
        return C2D_Color32(164, 0, 0, 255);
    }

    if (strcmp(name, "BLUE") == 0) {
        return C2D_Color32(0, 0, 164, 255);
    }

    if (strcmp(name, "GREEN") == 0) {
        return C2D_Color32(0, 164, 0, 255);
    }

    if (strcmp(name, "YELLOW") == 0) {
        return C2D_Color32(164, 164, 0, 255);
    }

    if (strcmp(name, "BLACK") == 0) {
        return C2D_Color32(0, 0, 0, 255);
    }

    if (strcmp(name, "WHITE") == 0) {
        return C2D_Color32(164, 164, 164, 255);
    }

    if (strcmp(name, "GRAY") == 0) {
        return C2D_Color32(64, 64, 64, 255);
    }

    if (strcmp(name, "LIGHTGRAY") == 0) {
        return C2D_Color32(146, 146, 146, 255);
    }

    return C2D_Color32(164, 164, 164, 255);
}
