// gfxmap.c
// Global sprite name -> spritesheet index table, filled from tex3ds headers.
// There is a single table for the whole game: each gfxmap_load() replaces
// it, so callers resolve their images immediately after loading.
#include <citro2d.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gfxmap.h"


#define GFX_MAX_IMAGES 128
static GfxImageIndex image_indexes[GFX_MAX_IMAGES];
static size_t image_index_count = 0;

// Callers pass the short PNG name; tex3ds names the macros gfx_<name>_idx
// because every header is generated from a file named gfx.t3s.
int gfxmap_get_index(const char *name) {
	char index_name[256];
	snprintf(index_name, sizeof(index_name), "gfx_%s_idx", name);
    for (size_t i = 0; i < image_index_count; i++) {
        if (strcmp(image_indexes[i].name, index_name) == 0) {
            return image_indexes[i].index;
        }
    }
    return -1;
}

C2D_Image gfxmap_get_image(C2D_SpriteSheet assets, const char *name) {
    int index = gfxmap_get_index(name);

    if (index < 0) {
    	printf("Image not found: %s\n", name);
        return (C2D_Image){0};
    }
    return C2D_SpriteSheetGetImage(assets, index);
}

bool gfxmap_load(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("Cannot open %s\n", filename);
        return false;
    }

    // Drop the previous table: indexes are only valid for this header.
    image_index_count = 0;

    char line[256];

    while (fgets(line, sizeof(line), f)) {
        char directive[32];
        char name[GFX_NAME_MAX];
        char value[32];

        if (sscanf(line, "%31s %95s %31s", directive, name, value) != 3) {
            continue;
        }

        // Keep only "#define <name>_idx <n>" lines; anything else in the
        // header is ignored.
        if (strcmp(directive, "#define") != 0) {
            continue;
        }

        size_t len = strlen(name);

        if (len < 4 || strcmp(name + len - 4, "_idx") != 0) {
            continue;
        }
        int index = atoi(value);

        if (image_index_count >= GFX_MAX_IMAGES) {
            printf("Too many images in %s\n", filename);
            fclose(f);
            return false;
        }

        GfxImageIndex *entry = &image_indexes[image_index_count++];

        strcpy(entry->name, name);
        entry->index = index;
    }
    printf("%s: Loaded %zu images\n", filename, image_index_count);
    fclose(f);
    return true;
}

bool gfxmap_load_assets(const char *path, C2D_SpriteSheet *assets) {
    char filename[256];

    snprintf(filename, sizeof(filename), "%s/gfx.t3x", path);

    *assets = C2D_SpriteSheetLoad(filename);
    if (!*assets) {
        printf("Cannot load spritesheet: %s\n", filename);
        return false;
    }

    snprintf(filename, sizeof(filename), "%s/gfx.h", path);

    if (!gfxmap_load(filename)) {
        printf("Cannot load gfx headers: %s\n", filename);
        C2D_SpriteSheetFree(*assets);
        *assets = NULL;
        return false;
    }

    return true;
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
