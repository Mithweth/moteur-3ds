// hud.c
// Implementation notes: the layout is read from romfs:/hud/hud (format in
// docs/HUD.en.md) into hud_config. Its images come from the romfs:/hud
// spritesheet and are resolved once while parsing. All HUD text is re-parsed
// into text_buf every frame. The countdown lasts the TIMER block's
// MAX_DURATION seconds of game time, measured with osGetTime(); the timer
// functions do nothing when there is no TIMER block.
#include <3ds.h>
#include <citro2d.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "hud.h"
#include "game.h"
#include "inventory.h"
#include "lang.h"
#include "str_utils.h"
#include "gfxmap.h"
#include "room.h"

typedef enum {
    HUD_BLOCK_NONE,
    HUD_BLOCK_INVENTORY,
    HUD_BLOCK_DIRECTIONS,
    HUD_BLOCK_OBJECT,
    HUD_BLOCK_TARGET,
    HUD_BLOCK_TIMER
} HudBlock;

typedef enum {
    HUD_DIRECTION_NORTH,
    HUD_DIRECTION_NORTHEAST,
    HUD_DIRECTION_EAST,
    HUD_DIRECTION_SOUTHEAST,
    HUD_DIRECTION_SOUTH,
    HUD_DIRECTION_SOUTHWEST,
    HUD_DIRECTION_WEST,
    HUD_DIRECTION_NORTHWEST,
    HUD_DIRECTION_COUNT
} HudDirection;

typedef struct {
    C2D_Image image;
    float x;
    float y;
} HudElement;

typedef struct {
    u32 color;
    float x;
    float y;
    float size;
} HudStyle;

typedef struct {
    char *id;
    HudStyle style;
} HudText;

typedef struct {
    HudElement background;
    HudText text;
    float item_x;
    float item_y;
    float item_spacing_x;
    float item_spacing_y;
    float item_size;
    size_t columns;
    size_t rows;
    HudElement selection;
    HudStyle selected_item;
    bool selected_item_enabled;
    HudElement examine_background;
    HudStyle examine_text;
} HudInventory;

typedef struct {
    bool enabled;
    HudElement background;
    HudElement directions[HUD_DIRECTION_COUNT];
} HudDirections;

typedef struct {
    bool enabled;
    HudElement background;
    HudText text;
    HudStyle item;
} HudPanel;

typedef struct {
    bool enabled;
    HudElement background;
    HudStyle text;
    u32 max_duration;       /* seconds */
    char *timeline;
} HudTimer;

typedef struct {
    C2D_Image background;
    HudInventory inventory;
    HudDirections compass;
    HudPanel object;
    HudPanel target;
    HudTimer timer;
} HudConfig;


static GfxAssets assets;
static C2D_TextBuf text_buf;
static C2D_Text text;
// Game time already spent, in milliseconds. last_time is the osGetTime() value
// at the previous update; timer_resume resets it after a suspension so the time
// spent in the home menu or in sleep mode isn't counted.
static u64 elapsed_time;
static u64 last_time;
static bool time_up_triggered;
static HudConfig hud_config;

// Index in HudDirections.directions for a direction keyword, or -1.
static int parse_direction(const char *name) {
    if (strcmp(name, "NORTH") == 0) {
        return HUD_DIRECTION_NORTH;
    }
    if (strcmp(name, "NORTHEAST") == 0) {
        return HUD_DIRECTION_NORTHEAST;
    }
    if (strcmp(name, "EAST") == 0) {
        return HUD_DIRECTION_EAST;
    }
    if (strcmp(name, "SOUTHEAST") == 0) {
        return HUD_DIRECTION_SOUTHEAST;
    }
    if (strcmp(name, "SOUTH") == 0) {
        return HUD_DIRECTION_SOUTH;
    }
    if (strcmp(name, "SOUTHWEST") == 0) {
        return HUD_DIRECTION_SOUTHWEST;
    }
    if (strcmp(name, "WEST") == 0) {
        return HUD_DIRECTION_WEST;
    }
    if (strcmp(name, "NORTHWEST") == 0) {
        return HUD_DIRECTION_NORTHWEST;
    }

    return -1;
}

// Parses the HUD layout file into hud_config. On error, logs file:line and
// returns false; strings already allocated are freed by hud_close().
static bool load_hud(const char *filename) {
    FILE *f = fopen(filename, "r");

    if (!f) {
        printf("Cannot open HUD: %s\n", filename);
        return false;
    }

    memset(&hud_config, 0, sizeof(hud_config));

    hud_config.inventory.item_size = 32.0f;
    hud_config.inventory.item_spacing_x = 14.0f;
    hud_config.inventory.item_spacing_y = 10.0f;
    hud_config.inventory.columns = 6;
    hud_config.inventory.rows = 2;

    char line[256];
    size_t line_number = 0;
    bool inventory_found = false;
    HudBlock block = HUD_BLOCK_NONE;

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

        /*
         * Top level
         */
        if (block == HUD_BLOCK_NONE) {
            if (strcmp(command, "BACKGROUND") == 0) {
                char *image_name = strtok(NULL, " ");

                if (!image_name) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    return false;
                }

                hud_config.background = gfxmap_get_image(&assets, image_name);

                if (!hud_config.background.tex) {
                    printf("%s:%zu: unknown image: %s\n", filename, line_number, image_name);
                    fclose(f);
                    return false;
                }

                continue;
            }

            if (strcmp(command, "INVENTORY") == 0) {
                block = HUD_BLOCK_INVENTORY;
                inventory_found = true;
                continue;
            }

            if (strcmp(command, "DIRECTIONS") == 0) {
                hud_config.compass.enabled = true;
                block = HUD_BLOCK_DIRECTIONS;
                continue;
            }

            if (strcmp(command, "OBJECT") == 0) {
                hud_config.object.enabled = true;
                block = HUD_BLOCK_OBJECT;
                continue;
            }

            if (strcmp(command, "TARGET") == 0) {
                hud_config.target.enabled = true;
                block = HUD_BLOCK_TARGET;
                continue;
            }

            if (strcmp(command, "TIMER") == 0) {
                hud_config.timer.enabled = true;
                block = HUD_BLOCK_TIMER;
                continue;
            }

            printf("%s:%zu: unknown command: %s\n", filename, line_number, command);
            fclose(f);
            return false;
        }

        /*
         * INVENTORY
         */
        if (block == HUD_BLOCK_INVENTORY) {
            if (strcmp(command, "END_INVENTORY") == 0) {
                block = HUD_BLOCK_NONE;
                continue;
            }

            if (strcmp(command, "BACKGROUND") == 0) {
                char *image_name = strtok(NULL, " ");
                char *x = strtok(NULL, " ");
                char *y = strtok(NULL, " ");
                if (!image_name || !x || !y) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    return false;
                }

                hud_config.inventory.background.image = gfxmap_get_image(&assets, image_name);

                if (!hud_config.inventory.background.image.tex) {
                    printf("%s:%zu: unknown image: %s\n", filename, line_number, image_name);
                    fclose(f);
                    return false;
                }

                hud_config.inventory.background.x = atof(x);
                hud_config.inventory.background.y = atof(y);
                continue;
            }

            if (strcmp(command, "TEXT") == 0) {
                char *color = strtok(NULL, " ");
                char *text  = strtok(NULL, " ");
                char *x     = strtok(NULL, " ");
                char *y     = strtok(NULL, " ");
                char *size  = strtok(NULL, " ");

                if (!color || !text || !x || !y || !size) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    return false;
                }

                hud_config.inventory.text.id = strdup(text);
                hud_config.inventory.text.style.color =
                    gfxmap_parse_color(color);
                hud_config.inventory.text.style.x = atof(x);
                hud_config.inventory.text.style.y = atof(y);
                hud_config.inventory.text.style.size = atof(size);
                continue;
            }

            if (strcmp(command, "ITEM_POSITION") == 0) {
                char *x = strtok(NULL, " ");
                char *y = strtok(NULL, " ");

                if (!x || !y) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    return false;
                }

                hud_config.inventory.item_x = atof(x);
                hud_config.inventory.item_y = atof(y);
                continue;
            }

            if (strcmp(command, "SPACING") == 0) {
                char *x = strtok(NULL, " ");
                char *y = strtok(NULL, " ");

                if (!x || !y) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    return false;
                }

                hud_config.inventory.item_spacing_x = atof(x);
                hud_config.inventory.item_spacing_y = atof(y);
                continue;
            }

            if (strcmp(command, "ITEM_SIZE") == 0) {
                char *size = strtok(NULL, " ");

                if (!size) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    return false;
                }

                hud_config.inventory.item_size = atof(size);
                continue;
            }

            if (strcmp(command, "COLUMNS") == 0) {
                char *columns = strtok(NULL, " ");

                if (!columns) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    return false;
                }

                hud_config.inventory.columns = atoi(columns);

                if (hud_config.inventory.columns == 0) {
                    printf("%s:%zu: syntax error: COLUMNS cannot be equal to 0\n", filename, line_number);
                    fclose(f);
                    return false;
                }                
                continue;
            }

            if (strcmp(command, "ROWS") == 0) {
                char *rows = strtok(NULL, " ");

                if (!rows) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    return false;
                }

                hud_config.inventory.rows = atoi(rows);

                if (hud_config.inventory.rows == 0) {
                    printf("%s:%zu: syntax error: ROWS cannot be equal to 0\n", filename, line_number);
                    fclose(f);
                    return false;
                }  
                continue;
            }

            if (strcmp(command, "SELECTION") == 0) {
                char *image_name = strtok(NULL, " ");
                char *x = strtok(NULL, " ");
                char *y = strtok(NULL, " ");

                if (!image_name || !x || !y) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    return false;
                }

                hud_config.inventory.selection.image = gfxmap_get_image(&assets, image_name);

                if (!hud_config.inventory.selection.image.tex) {
                    printf("%s:%zu: unknown image: %s\n", filename, line_number, image_name);
                    fclose(f);
                    return false;
                }

                hud_config.inventory.selection.x = atof(x);
                hud_config.inventory.selection.y = atof(y);
                continue;
            }

            if (strcmp(command, "SELECTED_ITEM") == 0) {
                char *x = strtok(NULL, " ");
                char *y = strtok(NULL, " ");
                char *size = strtok(NULL, " ");

                if (!size || !x || !y) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    return false;
                }

                hud_config.inventory.selected_item.x = atof(x);
                hud_config.inventory.selected_item.y = atof(y);
                hud_config.inventory.selected_item.size = atof(size);
                hud_config.inventory.selected_item_enabled = true;
                continue;
            }

            if (strcmp(command, "EXAMINE_BACKGROUND") == 0) {
                char *image_name = strtok(NULL, " ");
                char *x = strtok(NULL, " ");
                char *y = strtok(NULL, " ");

                if (!image_name || !x || !y) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    return false;
                }

                hud_config.inventory.examine_background.image = gfxmap_get_image(&assets, image_name);

                if (!hud_config.inventory.examine_background.image.tex) {
                    printf("%s:%zu: unknown image: %s\n", filename, line_number, image_name);
                    fclose(f);
                    return false;
                }
                hud_config.inventory.examine_background.x = atof(x);
                hud_config.inventory.examine_background.y = atof(y);
                continue;
            }

            if (strcmp(command, "EXAMINE_TEXT") == 0) {
                char *color = strtok(NULL, " ");
                char *x     = strtok(NULL, " ");
                char *y     = strtok(NULL, " ");
                char *size  = strtok(NULL, " ");

                if (!color || !x || !y || !size) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    return false;
                }

                hud_config.inventory.examine_text.color =
                    gfxmap_parse_color(color);
                hud_config.inventory.examine_text.x = atof(x);
                hud_config.inventory.examine_text.y = atof(y);
                hud_config.inventory.examine_text.size = atof(size);
                continue;
            }

            printf("%s:%zu: unknown command: %s\n", filename, line_number, command);
            fclose(f);
            return false;
        }

        /*
         * DIRECTIONS
         */
        if (block == HUD_BLOCK_DIRECTIONS) {
            if (strcmp(command, "END_DIRECTIONS") == 0) {
                block = HUD_BLOCK_NONE;
                continue;
            }

            if (strcmp(command, "BACKGROUND") == 0) {
                char *image_name = strtok(NULL, " ");
                char *x = strtok(NULL, " ");
                char *y = strtok(NULL, " ");
                if (!image_name || !x || !y) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    return false;
                }

                hud_config.compass.background.image = gfxmap_get_image(&assets, image_name);

                if (!hud_config.compass.background.image.tex) {
                    printf("%s:%zu: unknown image: %s\n", filename, line_number, image_name);
                    fclose(f);
                    return false;
                }

                hud_config.compass.background.x = atof(x);
                hud_config.compass.background.y = atof(y);
                continue;
            }

            int direction = parse_direction(command);

            if (direction >= 0) {
                char *image_name = strtok(NULL, " ");
                char *x = strtok(NULL, " ");
                char *y = strtok(NULL, " ");

                if (!image_name || !x || !y) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    return false;
                }

                HudElement *element = &hud_config.compass.directions[direction];

                element->image = gfxmap_get_image(&assets, image_name);

                if (!element->image.tex) {
                    printf("%s:%zu: unknown image: %s\n", filename, line_number, image_name);
                    fclose(f);
                    return false;
                }

                element->x = atof(x);
                element->y = atof(y);
                continue;
            }

            printf("%s:%zu: unknown command: %s\n", filename, line_number, command);
            fclose(f);
            return false;
        }

        /*
         * OBJECT / TARGET
         */
        if (block == HUD_BLOCK_OBJECT || block == HUD_BLOCK_TARGET) {
            HudPanel *panel = (block == HUD_BLOCK_OBJECT) ? &hud_config.object : &hud_config.target;

            const char *end_command = (block == HUD_BLOCK_OBJECT) ? "END_OBJECT" : "END_TARGET";

            if (strcmp(command, end_command) == 0) {
                block = HUD_BLOCK_NONE;
                continue;
            }

            if (strcmp(command, "BACKGROUND") == 0) {
                char *image_name = strtok(NULL, " ");
                char *x = strtok(NULL, " ");
                char *y = strtok(NULL, " ");
                if (!image_name || !x || !y) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    return false;
                }

                panel->background.image = gfxmap_get_image(&assets, image_name);

                if (!panel->background.image.tex) {
                    printf("%s:%zu: unknown image: %s\n", filename, line_number, image_name);
                    fclose(f);
                    return false;
                }

                panel->background.x = atof(x);
                panel->background.y = atof(y);
                continue;
            }

            if (strcmp(command, "TEXT") == 0) {
                char *color = strtok(NULL, " ");
                char *text  = strtok(NULL, " ");
                char *x     = strtok(NULL, " ");
                char *y     = strtok(NULL, " ");
                char *size  = strtok(NULL, " ");

                if (!color || !text || !x || !y || !size) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    return false;
                }

                panel->text.id = strdup(text);
                panel->text.style.color = gfxmap_parse_color(color);
                panel->text.style.x = atof(x);
                panel->text.style.y = atof(y);
                panel->text.style.size = atof(size);
                continue;
            }

            if (strcmp(command, "ITEM") == 0) {
                char *color = strtok(NULL, " ");
                char *x     = strtok(NULL, " ");
                char *y     = strtok(NULL, " ");
                char *size  = strtok(NULL, " ");

                if (!color || !x || !y || !size) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    return false;
                }

                panel->item.color = gfxmap_parse_color(color);
                panel->item.x = atof(x);
                panel->item.y = atof(y);
                panel->item.size = atof(size);
                continue;
            }

            printf("%s:%zu: unknown command: %s\n", filename, line_number, command);
            fclose(f);
            return false;
        }

        /*
         * TIMER
         */
        if (block == HUD_BLOCK_TIMER) {
            if (strcmp(command, "END_TIMER") == 0) {
                block = HUD_BLOCK_NONE;
                continue;
            }

            if (strcmp(command, "BACKGROUND") == 0) {
                char *image_name = strtok(NULL, " ");
                char *x = strtok(NULL, " ");
                char *y = strtok(NULL, " ");
                if (!image_name || !x || !y) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    return false;
                }

                hud_config.timer.background.image = gfxmap_get_image(&assets, image_name);

                if (!hud_config.timer.background.image.tex) {
                    printf("%s:%zu: unknown image: %s\n", filename, line_number, image_name);
                    fclose(f);
                    return false;
                }

                hud_config.timer.background.x = atof(x);
                hud_config.timer.background.y = atof(y);
                continue;
            }

            if (strcmp(command, "TEXT") == 0) {
                char *color = strtok(NULL, " ");
                char *x     = strtok(NULL, " ");
                char *y     = strtok(NULL, " ");
                char *size  = strtok(NULL, " ");

                if (!color || !x || !y || !size) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    return false;
                }

                hud_config.timer.text.color = gfxmap_parse_color(color);
                hud_config.timer.text.x = atof(x);
                hud_config.timer.text.y = atof(y);
                hud_config.timer.text.size = atof(size);
                continue;
            }

            if (strcmp(command, "MAX_DURATION") == 0) {
                char *duration = strtok(NULL, " ");

                if (!duration) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    return false;
                }

                hud_config.timer.max_duration = atoi(duration);         
                continue;
            }

            if (strcmp(command, "TIMELINE") == 0) {
                char *timeline = strtok(NULL, " ");

                if (!timeline) {
                    printf("%s:%zu: syntax error\n", filename, line_number);
                    fclose(f);
                    return false;
                }

                hud_config.timer.timeline = strdup(timeline);
                continue;
            }

            printf("%s:%zu: unknown command: %s\n", filename, line_number, command);
            fclose(f);
            return false;
        }
    }

    if (block != HUD_BLOCK_NONE) {
        printf("%s:%zu: missing END_* directive\n", filename, line_number);
        fclose(f);
        return false;
    }

    fclose(f);

    if (!hud_config.background.tex) {
        printf("%s: missing BACKGROUND\n", filename);
        return false;
    }

    if (hud_config.timer.enabled && !hud_config.timer.timeline) {
        printf("%s: TIMER: TIMELINE is required\n", filename);
        return false;
    }

    if (hud_config.timer.enabled && hud_config.timer.max_duration == 0) {
        printf("%s: TIMER: MAX_DURATION is required\n", filename);
        return false;
    }

    if (!inventory_found) {
        printf("%s: missing INVENTORY block\n", filename);
        return false;
    }

    if (!hud_config.inventory.text.id) {
        printf("%s: INVENTORY: TEXT is required\n", filename);
        return false;
    }

    if (!hud_config.inventory.selection.image.tex) {
        printf("%s: INVENTORY: SELECTION is required\n", filename);
        return false;
    }

    if (hud_config.object.enabled && !hud_config.object.text.id) {
        printf("%s: OBJECT: TEXT is required\n", filename);
        return false;
    }

    if (hud_config.target.enabled && !hud_config.target.text.id) {
        printf("%s: TARGET: TEXT is required\n", filename);
        return false;
    }

    return true;
}

void timer_start(void) {
    if (hud_config.timer.enabled) {
        elapsed_time = 0;
        last_time = osGetTime();
        time_up_triggered = false;
    }
}

void timer_update(void) {
    if (hud_config.timer.enabled) {
        u64 now = osGetTime();
        elapsed_time += now - last_time;
        last_time = now;
    }
}

void timer_resume(void) {
    if (hud_config.timer.enabled) {
        last_time = osGetTime();
    }
}

u64 timer_get_elapsed_time(void) {
    return elapsed_time;
}

void timer_set_elapsed_time(u64 e) {
    elapsed_time = e;
}

// Fake bold: draws the text twice, one pixel apart.
static void draw_bold_text(C2D_Text *text, float x, float y, float z, float s, u32 color) {
    C2D_DrawText(text, C2D_WithColor, x, y, z, s, s, color);
    C2D_DrawText(text, C2D_WithColor, x + 1.0f, y, z, s, s, color);
}

static void inventory_draw(void) {
    const HudInventory *hud = &hud_config.inventory;
    if (hud->background.image.tex) {
        C2D_DrawImageAt(hud->background.image, hud->background.x, hud->background.y, 0.1f, NULL, 1.0f, 1.0f);
    }
    C2D_TextParse(&text, text_buf, lang_get(hud->text.id));
    C2D_TextOptimize(&text);
    draw_bold_text(&text, hud->text.style.x, hud->text.style.y, 0.2f, hud->text.style.size, hud->text.style.color);

    if (inventory_get_count() == 0) {
        return;
    }

    if (inventory_is_active()) {
        const Item *item = inventory_get_selected_item();
        if (hud->examine_background.image.tex) {
            C2D_DrawImageAt(hud->examine_background.image, hud->examine_background.x, hud->examine_background.y, 0.2f, NULL, 1.0f, 1.0f);
        }
        if (item->detail_image.tex) {
            if (item->detail_fullscreen) {
                C2D_DrawRectSolid(0.0f, 0.0f, 0.8f, 400.0f, 240.0f, C2D_Color32(0, 0, 0, 255));
            }
            C2D_DrawImageAt(item->detail_image, item->detail_x, item->detail_y, 0.9f, NULL, 1.0f, 1.0f);
        }
        if (item->examine_text) {
            C2D_TextParse(&text, text_buf, lang_get(item->examine_text));
            C2D_TextOptimize(&text);
            C2D_DrawText(&text, C2D_WithColor, hud->examine_text.x, hud->examine_text.y, 0.3f, hud->examine_text.size, hud->examine_text.size, hud->examine_text.color);
        }
        if (item->examine_callback) {
            item->examine_callback();
        }
        return;
    }
    // Only `rows` rows fit. Keep the selected row visible, with the row above
    // it when there is room for both, and don't leave empty rows at the bottom
    // when the inventory has enough rows to fill them.
    size_t count = inventory_get_count();
    size_t total_rows = (count + hud->columns - 1) / hud->columns;
    size_t selected_row = inventory_get_selected() / hud->columns;
    size_t first_row = (selected_row > 0 && hud->rows > 1) ? selected_row - 1 : selected_row;

    if (first_row + hud->rows > total_rows) {
        first_row = total_rows > hud->rows ? total_rows - hud->rows : 0;
    }
    size_t first = first_row * hud->columns;
    size_t last = first + hud->columns * hud->rows;

    if (last > count) {
        last = count;
    }
    for (size_t i = first; i < last; i++) {
        size_t visible = i - first;

        float x = hud->item_x + (visible % hud->columns) * (hud->item_size + hud->item_spacing_x);
        float y = hud->item_y + (visible / hud->columns) * (hud->item_size + hud->item_spacing_y);
        
        if (i == inventory_get_selected()) {
            C2D_DrawImageAt(hud->selection.image, x + hud->selection.x, y + hud->selection.y, 0.2f, NULL, 1.0f, 1.0f);
        }
        C2D_DrawImageAt(inventory_get_item(i)->image, x, y, 0.4f, NULL, hud->item_size / 48, hud->item_size / 48);
    }
}


static void selected_item_draw(void) {
    const HudPanel *panel = &hud_config.object;
    if (!panel->enabled) {
        return;
    }
    if (panel->background.image.tex) {
        C2D_DrawImageAt(panel->background.image, panel->background.x, panel->background.y, 0.1f, NULL, 1.0f, 1.0f);
    }
    C2D_TextParse(&text, text_buf, lang_get(panel->text.id));
    C2D_TextOptimize(&text);
    draw_bold_text(&text, panel->text.style.x, panel->text.style.y, 0.2f, panel->text.style.size, panel->text.style.color);
    const Item *item = inventory_get_selected_item();
    if (item) {
        if (hud_config.inventory.selected_item_enabled) {
            C2D_DrawImageAt(item->image, hud_config.inventory.selected_item.x, hud_config.inventory.selected_item.y, 0.2f, NULL, hud_config.inventory.selected_item.size, hud_config.inventory.selected_item.size);
        }
        C2D_TextParse(&text, text_buf, lang_get(item->name_id));
        C2D_TextOptimize(&text);
        C2D_DrawText(&text, C2D_WithColor, panel->item.x, panel->item.y, 0.2f, panel->item.size, panel->item.size, panel->item.color);
    }
}

static void target_draw(void) {
    const HudPanel *panel = &hud_config.target;
    if (!panel->enabled) {
        return;
    }
    const char* target_name = game_target_name();
    if (panel->background.image.tex) {
        C2D_DrawImageAt(panel->background.image, panel->background.x, panel->background.y, 0.1f, NULL, 1.0f, 1.0f);
    }
    C2D_TextParse(&text, text_buf, lang_get(panel->text.id));
    C2D_TextOptimize(&text);
    draw_bold_text(&text, panel->text.style.x, panel->text.style.y, 0.2f, panel->text.style.size, panel->text.style.color);
    if (target_name) {
        C2D_TextParse(&text, text_buf, lang_get(target_name));
        C2D_TextOptimize(&text);
        C2D_DrawText(&text, C2D_WithColor, panel->item.x, panel->item.y, 0.2f, panel->item.size, panel->item.size, panel->item.color);
    }
}


static void timer_draw() {
    if (hud_config.timer.enabled) {
        if (hud_config.timer.background.image.tex) {
            C2D_DrawImageAt(hud_config.timer.background.image, hud_config.timer.background.x, hud_config.timer.background.y, 0.1f, NULL, 1.0f, 1.0f);
        }
        int total_seconds = hud_config.timer.max_duration - elapsed_time / 1000;
        if (total_seconds < 0) {
            total_seconds = 0;
        }
        int hours   = (total_seconds / 3600) % 100;
        int minutes = (total_seconds / 60) % 60;
        int seconds = total_seconds % 60;
        char str[16];
        snprintf(str, sizeof(str), "%02d:%02d:%02d", hours, minutes, seconds);
        C2D_TextParse(&text, text_buf, str);
        C2D_TextOptimize(&text);
        draw_bold_text(&text, hud_config.timer.text.x, hud_config.timer.text.y, 0.2f, hud_config.timer.text.size, hud_config.timer.text.color);
    }
}

static void movement_draw(void) {
    if (hud_config.compass.enabled) {
        if (hud_config.compass.background.image.tex) {
            C2D_DrawImageAt(hud_config.compass.background.image, hud_config.compass.background.x, hud_config.compass.background.y, 0.1f, NULL, 1.0f, 1.0f);
        }
        if (room_can_move_north() && hud_config.compass.directions[HUD_DIRECTION_NORTH].image.tex) {
            C2D_DrawImageAt(hud_config.compass.directions[HUD_DIRECTION_NORTH].image, hud_config.compass.directions[HUD_DIRECTION_NORTH].x, hud_config.compass.directions[HUD_DIRECTION_NORTH].y, 0.2f, NULL, 1.0f, 1.0f);
        }
        if (room_can_move_northeast() && hud_config.compass.directions[HUD_DIRECTION_NORTHEAST].image.tex) {
            C2D_DrawImageAt(hud_config.compass.directions[HUD_DIRECTION_NORTHEAST].image, hud_config.compass.directions[HUD_DIRECTION_NORTHEAST].x, hud_config.compass.directions[HUD_DIRECTION_NORTHEAST].y, 0.2f, NULL, 1.0f, 1.0f);
        }

        if (room_can_move_east() && hud_config.compass.directions[HUD_DIRECTION_EAST].image.tex) {
            C2D_DrawImageAt(hud_config.compass.directions[HUD_DIRECTION_EAST].image, hud_config.compass.directions[HUD_DIRECTION_EAST].x, hud_config.compass.directions[HUD_DIRECTION_EAST].y, 0.2f, NULL, 1.0f, 1.0f);
        }

        if (room_can_move_southeast() && hud_config.compass.directions[HUD_DIRECTION_SOUTHEAST].image.tex) {
            C2D_DrawImageAt(hud_config.compass.directions[HUD_DIRECTION_SOUTHEAST].image, hud_config.compass.directions[HUD_DIRECTION_SOUTHEAST].x, hud_config.compass.directions[HUD_DIRECTION_SOUTHEAST].y, 0.2f, NULL, 1.0f, 1.0f);
        }

        if (room_can_move_south() && hud_config.compass.directions[HUD_DIRECTION_SOUTH].image.tex) {
            C2D_DrawImageAt(hud_config.compass.directions[HUD_DIRECTION_SOUTH].image, hud_config.compass.directions[HUD_DIRECTION_SOUTH].x, hud_config.compass.directions[HUD_DIRECTION_SOUTH].y, 0.2f, NULL, 1.0f, 1.0f);
        }

        if (room_can_move_southwest() && hud_config.compass.directions[HUD_DIRECTION_SOUTHWEST].image.tex) {
            C2D_DrawImageAt(hud_config.compass.directions[HUD_DIRECTION_SOUTHWEST].image, hud_config.compass.directions[HUD_DIRECTION_SOUTHWEST].x, hud_config.compass.directions[HUD_DIRECTION_SOUTHWEST].y, 0.2f, NULL, 1.0f, 1.0f);
        }

        if (room_can_move_west() && hud_config.compass.directions[HUD_DIRECTION_WEST].image.tex) {
            C2D_DrawImageAt(hud_config.compass.directions[HUD_DIRECTION_WEST].image, hud_config.compass.directions[HUD_DIRECTION_WEST].x, hud_config.compass.directions[HUD_DIRECTION_WEST].y, 0.2f, NULL, 1.0f, 1.0f);
        }
        if (room_can_move_northwest() && hud_config.compass.directions[HUD_DIRECTION_NORTHWEST].image.tex) {
            C2D_DrawImageAt(hud_config.compass.directions[HUD_DIRECTION_NORTHWEST].image, hud_config.compass.directions[HUD_DIRECTION_NORTHWEST].x, hud_config.compass.directions[HUD_DIRECTION_NORTHWEST].y, 0.2f, NULL, 1.0f, 1.0f);
        }
    }
}

bool hud_init(void) {
    if (!gfxmap_load_assets("romfs:/hud", &assets)) {
        printf("Cannot load hud assets\n");
        return false;
    }

    if (!text_buf) {
        text_buf = C2D_TextBufNew(1024);
    }

    if (!load_hud("romfs:/hud/hud")) {
        hud_close();
        return false;
    }

    inventory_set_columns(hud_config.inventory.columns);
    timer_start();
    return true;
}

void hud_reset(void) {
    timer_start();
}

bool hud_update(void) {
    if (!hud_config.timer.enabled) {
        return false;
    }
    timer_update();
    if (time_up_triggered || elapsed_time / 1000 < hud_config.timer.max_duration) {
        return false;
    }
    time_up_triggered = true;
    // Whether the timeline starts or fails (back to the title screen), the
    // mode has changed: report it either way.
    game_timeline_start(hud_config.timer.timeline);
    return true;
}

void hud_close(void) {
    free(hud_config.inventory.text.id);
    free(hud_config.object.text.id);
    free(hud_config.target.text.id);
    free(hud_config.timer.timeline);

    memset(&hud_config, 0, sizeof(hud_config));

    if (text_buf) {
        C2D_TextBufDelete(text_buf);
        text_buf = NULL;
    }

    gfxmap_free_assets(&assets);
}

void hud_draw(void) {
    // Cleared once per frame: every element below appends its text to it.
    C2D_TextBufClear(text_buf);
    C2D_DrawImageAt(hud_config.background, 0.0f, 0.0f, 0.0f, NULL, 1.0f, 1.0f);
    target_draw();
    selected_item_draw();
    inventory_draw();
    movement_draw();
    timer_draw();
}
