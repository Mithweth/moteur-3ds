// title.c
// Title screen implementation. The menu entries are translation keys looked
// up on every frame, so switching language (lang_next) takes effect at once.
// The layout (backgrounds, sounds, music, menu order and style, controls and
// credits pages) comes from the romfs:/game/title configuration file, and the
// images from the romfs:/game sprite sheet. Both are loaded by title_init and
// released by title_close (called when a game or the intro starts), which
// also stops the title music.

#include <3ds.h>
#include <citro2d.h>
#include <string.h>
#include <stdlib.h>
#include "lang.h"
#include "audio.h"
#include "game.h"
#include "gfxmap.h"
#include "str_utils.h"
#include "title.h"
#include "save.h"

// Action triggered by a menu entry.
typedef enum {
	TITLE_LANG,
	TITLE_INTRO,
	TITLE_CONTINUE,
	TITLE_GAME,
	TITLE_CONTROLS,
	TITLE_CREDITS,
	TITLE_COUNT
} TitleChoice;

// Sub-page currently shown instead of the menu.
typedef enum {
	OPTION_NONE,
	OPTION_CONTROLS,
	OPTION_CREDITS
} OptionChoice;

// Index of the highlighted entry in choices[].
static size_t selected;
static OptionChoice option = OPTION_NONE;
static C2D_TextBuf text_buf;
static GfxAssets assets;

// Capacity of the configuration arrays; extra lines are reported and ignored.
#define TITLE_MAX_IMAGES       8
#define TITLE_MAX_TEXTS        8
#define TITLE_MAX_CREDITS      11
#define TITLE_MENU_TOUCH_WIDTH 120

// Image drawn at a fixed position on the controls or credits page.
typedef struct {
	C2D_Image image;
	float x;
	float y;
} TitleImage;

// Translated text (id is a translation key) on the controls page.
typedef struct {
	char *id;
	float x;
	float y;
	float size;
	u32 flags;
} TitleText;

// Credits line: a translated role and an untranslated name.
typedef struct {
	char *role_id;
	char *name;
} Credit;

// Menu style. x is the horizontal center of the entries, y the top of the
// first one, spacing the vertical distance between two entries.
typedef struct {
	float x;
	float y;
	float spacing;
	float text_size;
	u32 color;
	u32 selected_color;
} TitleMenu;

// Everything read from the configuration file. Strings are allocated and
// freed by title_close; images point into the assets sprite sheet.
typedef struct {
	C2D_Image background_top;
	C2D_Image background_bottom;
	C2D_Image background_controls;
	C2D_Image background_credits;
	char *sfx_select;
	char *sfx_choice;
	char *music;
	TitleMenu menu;
	TitleImage controls_images[TITLE_MAX_IMAGES];
	size_t controls_image_count;
	TitleText controls_texts[TITLE_MAX_TEXTS];
	size_t controls_text_count;
	Credit credits[TITLE_MAX_CREDITS];
	size_t credit_count;
	TitleImage credits_images[TITLE_MAX_IMAGES];
	size_t credits_image_count;
} TitleConfig;

// Menu entry that can be listed in ORDER: id is the name used in the
// configuration file, label the translation key shown on screen.
typedef struct {
	const char *id;
	const char *label;
	TitleChoice choice;
} TitleMenuEntry;

static char intro_timeline[256];
static TitleConfig title_config;
static const TitleMenuEntry menu_entries[] = {
	{ "LANG",     "LANG_NAME",      TITLE_LANG },
	{ "INTRO",    "TITLE_INTRO",    TITLE_INTRO },
	{ "CONTINUE", "TITLE_CONTINUE", TITLE_CONTINUE },
	{ "GAME",     "TITLE_GAME",     TITLE_GAME },
	{ "CONTROLS", "TITLE_CONTROLS", TITLE_CONTROLS },
	{ "CREDITS",  "TITLE_CREDITS",  TITLE_CREDITS }
};

// Entries shown in the menu, in the ORDER given by the configuration file.
static const TitleMenuEntry *choices[TITLE_COUNT];
static size_t choice_count;

// Configuration block being parsed (MENU ... END_MENU, etc.).
typedef enum {
	TITLE_SECTION_NONE,
	TITLE_SECTION_MENU,
	TITLE_SECTION_CONTROLS,
	TITLE_SECTION_CREDITS
} TitleSection;

// Returns the menu entry named id in the configuration file, or NULL.
static const TitleMenuEntry *find_menu_entry(const char *id) {
	for (size_t i = 0; i < sizeof(menu_entries) / sizeof(menu_entries[0]); i++) {
		if (strcmp(id, menu_entries[i].id) == 0) {
			return &menu_entries[i];
		}
	}
	return NULL;
}

// Returns the C2D_DrawText flags for a TEXT alignment (LEFT, CENTER or
// RIGHT), C2D_WithColor alone (left-aligned) when the argument is omitted,
// or 0 for an unknown alignment, which makes load_title fail.
static u32 parse_alignment(const char *name) {
	if (!name) {
		return C2D_WithColor;
	}
	if (strcmp(name, "LEFT") == 0) {
		return C2D_WithColor | C2D_AlignLeft;
	} else if (strcmp(name, "CENTER") == 0) {
		return C2D_WithColor | C2D_AlignCenter;
	} else if (strcmp(name, "RIGHT") == 0) {
		return C2D_WithColor | C2D_AlignRight;
	}
	return 0;
}

// Parses the title configuration file into title_config and choices[].
// A line with missing arguments, an unknown command (including a top-level
// directive such as MUSIC placed inside a block) or an empty menu makes it
// fail; the caller then releases what was already allocated with title_close.
static bool load_title(const char *filename) {
	FILE *file = fopen(filename, "r");
	char line[512];
	size_t line_number = 0;
	TitleSection section = TITLE_SECTION_NONE;
	const TitleMenuEntry *default_entry = NULL;

	if (!file) {
		return false;
	}
	// Defaults for the MENU values the file does not set.
	title_config.menu.text_size = 0.65f;
	title_config.menu.selected_color = gfxmap_parse_color("LIGHTGRAY");
	title_config.menu.color = gfxmap_parse_color("GRAY");
	title_config.menu.x = 160.0f;
	title_config.menu.y = 70.0f;
	title_config.menu.spacing = 30.0f;

	while (fgets(line, sizeof(line), file)) {
		line_number++;
		char *content = str_trim(line);

		if (!*content || *content == '#') {
			continue;
		}

		char *command = strtok(content, " ");

		if (strcmp(command, "MENU") == 0) {
			section = TITLE_SECTION_MENU;
			continue;
		}

		if (strcmp(command, "CONTROLS") == 0) {
			section = TITLE_SECTION_CONTROLS;
			continue;
		}

		if (strcmp(command, "CREDITS") == 0) {
			section = TITLE_SECTION_CREDITS;
			continue;
		}

		if (strcmp(command, "END_MENU") == 0 || strcmp(command, "END_CONTROLS") == 0 || strcmp(command, "END_CREDITS") == 0) {
			section = TITLE_SECTION_NONE;
			continue;
		}

		if (section == TITLE_SECTION_NONE) {
			if (strcmp(command, "BACKGROUND_TOP") == 0) {
				char *img = strtok(NULL, " ");
				if (!img) {
					printf("%s:%zu: missing argument\n", filename, line_number);
					fclose(file);
					return false;
				}
				title_config.background_top = gfxmap_get_image(&assets, img);
				continue;
			}

			if (strcmp(command, "BACKGROUND_BOTTOM") == 0) {
				char *img = strtok(NULL, " ");
				if (!img) {
					printf("%s:%zu: missing argument\n", filename, line_number);
					fclose(file);
					return false;
				}
				title_config.background_bottom = gfxmap_get_image(&assets, img);
				continue;
			}

			if (strcmp(command, "SFX_SELECT") == 0) {
				char *snd = strtok(NULL, " ");
				if (!snd) {
					printf("%s:%zu: missing argument\n", filename, line_number);
					fclose(file);
					return false;
				}
				free(title_config.sfx_select);
				title_config.sfx_select = audio_resolve_path("romfs:/game", snd, ".raw");
				continue;
			}

			if (strcmp(command, "SFX_CHOICE") == 0) {
				char *snd = strtok(NULL, " ");
				if (!snd) {
					printf("%s:%zu: missing argument\n", filename, line_number);
					fclose(file);
					return false;
				}
				free(title_config.sfx_choice);
				title_config.sfx_choice = audio_resolve_path("romfs:/game", snd, ".raw");
				continue;
			}

			if (strcmp(command, "MUSIC") == 0) {
				char *snd = strtok(NULL, " ");
				if (!snd) {
					printf("%s:%zu: missing argument\n", filename, line_number);
					fclose(file);
					return false;
				}
				free(title_config.music);
				title_config.music = audio_resolve_path("romfs:/game", snd, ".ogg");
				continue;
			}
		}

		if (section == TITLE_SECTION_MENU) {
			if (strcmp(command, "POSITION") == 0) {
				char *x = strtok(NULL, " ");
				char *y = strtok(NULL, " ");

				if (!x || !y) {
					printf("%s:%zu: missing arguments\n", filename, line_number);
					fclose(file);
					return false;
				}
				title_config.menu.x = atof(x);
				title_config.menu.y = atof(y);
				continue;
			} else if (strcmp(command, "SPACING") == 0) {
				char *spacing = strtok(NULL, " ");
				if (!spacing) {
					printf("%s:%zu: missing argument\n", filename, line_number);
					fclose(file);
					return false;
				}
				title_config.menu.spacing = atof(spacing);
				if (title_config.menu.spacing <= 0.0f) {
					printf("%s:%zu: spacing must be positive\n", filename, line_number);
					fclose(file);
					return false;
				}
				continue;
			} else if (strcmp(command, "TEXT_SIZE") == 0) {
				char *size = strtok(NULL, " ");
				if (!size) {
					printf("%s:%zu: missing argument\n", filename, line_number);
					fclose(file);
					return false;
				}
				title_config.menu.text_size = atof(size);
				continue;
			} else if (strcmp(command, "COLOR") == 0) {
				char *color = strtok(NULL, " ");
				if (!color) {
					printf("%s:%zu: missing argument\n", filename, line_number);
					fclose(file);
					return false;
				}
				title_config.menu.color = gfxmap_parse_color(color);
				continue;
			} else if (strcmp(command, "SELECTED_COLOR") == 0) {
				char *color = strtok(NULL, " ");
				if (!color) {
					printf("%s:%zu: missing argument\n", filename, line_number);
					fclose(file);
					return false;
				}
				title_config.menu.selected_color = gfxmap_parse_color(color);
				continue;
			} else if (strcmp(command, "INTRO") == 0) {
				char *intro = strtok(NULL, " ");
				if (!intro) {
					printf("%s:%zu: missing argument\n", filename, line_number);
					fclose(file);
					return false;
				}
				strcpy(intro_timeline, intro);
				continue;
			} else if (strcmp(command, "DEFAULT") == 0) {
				char *choice = strtok(NULL, " ");
				if (!choice) {
					printf("%s:%zu: missing argument\n", filename, line_number);
					fclose(file);
					return false;
				}
				default_entry = find_menu_entry(choice);
				continue;
			} else if (strcmp(command, "ORDER") == 0) {
				char *order = strtok(NULL, " ");
				if (!order) {
					printf("%s:%zu: missing argument\n", filename, line_number);
					fclose(file);
					return false;
				}
				char *item = strtok(order, ",");
				while (item) {
					const TitleMenuEntry *entry = find_menu_entry(item);
					if (!entry) {
						printf("%s:%zu: ORDER invalid argument: %s\n", filename, line_number, item);
					} else if (choice_count < TITLE_COUNT) {
						choices[choice_count++] = entry;
					}
					item = strtok(NULL, ",");
				}
				continue;
			}
		}

		if (section == TITLE_SECTION_CONTROLS) {
			if (strcmp(command, "IMAGE") == 0) {
				if (title_config.controls_image_count >= TITLE_MAX_IMAGES) {
					printf("%s:%zu: too many images loaded: current limit is %d\n", filename, line_number, TITLE_MAX_IMAGES);
					continue;
				}
				char *name = strtok(NULL, " ");
				char *x = strtok(NULL, " ");
				char *y = strtok(NULL, " ");

				if (!name || !x || !y) {
					printf("%s:%zu: missing arguments\n", filename, line_number);
					fclose(file);
					return false;
				}
				TitleImage *image = &title_config.controls_images[title_config.controls_image_count++];
				image->image = gfxmap_get_image(&assets, name);
				image->x = atof(x);
				image->y = atof(y);
				continue;
			} else if (strcmp(command, "TEXT") == 0) {
				if (title_config.controls_text_count >= TITLE_MAX_TEXTS) {
					printf("%s:%zu: too many text blocks loaded: current limit is %d\n", filename, line_number, TITLE_MAX_TEXTS);
					continue;
				}
				char *id = strtok(NULL, " ");
				char *x = strtok(NULL, " ");
				char *y = strtok(NULL, " ");
				char *size = strtok(NULL, " ");
				char *flags = strtok(NULL, " ");

				if (!id || !x || !y || !size) {
					printf("%s:%zu: missing arguments\n", filename, line_number);
					fclose(file);
					return false;
				}
				u32 f = parse_alignment(flags);
				if (!f) {
					printf("%s:%zu: incorrect argument: %s\n", filename, line_number, flags);
					fclose(file);
					return false;
				}
				TitleText *text = &title_config.controls_texts[title_config.controls_text_count++];
				text->id = strdup(id);
				text->x = atof(x);
				text->y = atof(y);
				text->size = atof(size);
				text->flags = f;
				continue;
			} else if (strcmp(command, "BACKGROUND") == 0) {
				char *img = strtok(NULL, " ");
				if (!img) {
					printf("%s:%zu: missing argument\n", filename, line_number);
					fclose(file);
					return false;
				}
				title_config.background_controls = gfxmap_get_image(&assets, img);
				continue;
			}
		}

		if (section == TITLE_SECTION_CREDITS) {
			if (strcmp(command, "CREDIT") == 0) {
				if (title_config.credit_count >= TITLE_MAX_CREDITS) {
					printf("%s:%zu: too many credits blocks loaded: current limit is %d\n", filename, line_number, TITLE_MAX_CREDITS);
					continue;
				}
				char *role_id = strtok(NULL, " ");
				char *name = strtok(NULL, "\n");
				if (!name || !role_id) {
					printf("%s:%zu: missing arguments\n", filename, line_number);
					fclose(file);
					return false;
				}
				Credit *credit = &title_config.credits[title_config.credit_count++];
				credit->role_id = strdup(role_id);
				credit->name = strdup(str_trim(name));
				continue;
			} else if (strcmp(command, "IMAGE") == 0) {
				if (title_config.credits_image_count >= TITLE_MAX_IMAGES) {
					printf("%s:%zu: too many credits images loaded: current limit is %d\n", filename, line_number, TITLE_MAX_IMAGES);
					continue;
				}
				char *name = strtok(NULL, " ");
				char *x = strtok(NULL, " ");
				char *y = strtok(NULL, " ");

				if (!name || !x || !y) {
					printf("%s:%zu: missing arguments\n", filename, line_number);
					fclose(file);
					return false;
				}
				TitleImage *image = &title_config.credits_images[title_config.credits_image_count++];
				image->image = gfxmap_get_image(&assets, name);
				image->x = atof(x);
				image->y = atof(y);
				continue;
			} else if (strcmp(command, "BACKGROUND") == 0) {
				char *img = strtok(NULL, " ");
				if (!img) {
					printf("%s:%zu: missing argument\n", filename, line_number);
					fclose(file);
					return false;
				}
				title_config.background_credits = gfxmap_get_image(&assets, img);
				continue;
			}
		}
		printf("%s:%zu: unknown command: %s\n", filename, line_number, command);
		fclose(file);
		return false;
	}

	size_t visible_count = 0;
	for (size_t i = 0; i < choice_count; i++) {
		const TitleMenuEntry *entry = choices[i];
		if (entry->choice == TITLE_CONTINUE && !save_exists()) {
			continue;
		}
		if (entry->choice == TITLE_INTRO && !intro_timeline[0]) {
			continue;
		}
		if (entry->choice == TITLE_LANG && lang_count() < 2) {
			continue;
		}
		choices[visible_count++] = entry;
	}
	choice_count = visible_count;

	selected = 0;
	if (default_entry) {
		for (size_t i = 0; i < choice_count; i++) {
			if (choices[i] == default_entry) {
				selected = i;
				break;
			}
		}
	}

	fclose(file);
	if (choice_count == 0) {
		printf("Title menu is empty\n");
		return false;
	}
	return true;
}

bool title_init(void) {
	if (!gfxmap_load_assets("romfs:/game", &assets)) {
		printf("Cannot load title assets\n");
		return false;
	}

	intro_timeline[0] = '\0';
	if (!load_title("romfs:/game/title")) {
		printf("Cannot load title\n");
		title_close();
		return false;
	}

	if (!text_buf) {
		text_buf = C2D_TextBufNew(4096);
	}

	// Started only once the whole file is valid, so a load failure never
	// leaves the music of a title screen that is not shown.
	if (title_config.music) {
		music_play(title_config.music);
	}

	return true;
}

static size_t find_touch_selection(int x, int y) {
	int elem_x = title_config.menu.x - TITLE_MENU_TOUCH_WIDTH / 2;
	for (size_t i = 0; i < choice_count; i++) {
		int elem_y = (i * title_config.menu.spacing) + title_config.menu.y;
		if (x >= elem_x && x <= elem_x + TITLE_MENU_TOUCH_WIDTH && y >= elem_y && y < elem_y + title_config.menu.spacing) {
			return i;
		}
	}
	return choice_count;
}

void title_update(u32 keys, touchPosition touch) {
	// While the controls or credits page is open, A, B or a touch only closes it.
	if (option != OPTION_NONE) {
		if (keys & (KEY_A | KEY_B | KEY_TOUCH)) {
			if (title_config.sfx_choice) {
				sfx_play(title_config.sfx_choice);
			}
			option = OPTION_NONE;
		}
		return;
	}

	// KEY_UP / KEY_DOWN match both the D-pad and the circle pad.
	if (keys & KEY_UP) {
		if (selected == 0) {
			selected = choice_count - 1;
		} else {
			selected--;
		}
		if (title_config.sfx_select) {
			sfx_play(title_config.sfx_select);
		}
	}

	if (keys & KEY_DOWN) {
		selected++;
		if (selected >= choice_count) {
			selected = 0;
		}
		if (title_config.sfx_select) {
			sfx_play(title_config.sfx_select);
		}
	}

	if (keys & KEY_TOUCH) {
		size_t s = find_touch_selection(touch.px, touch.py);
		if (s == choice_count) {
			return;
		}
		selected = s;
	}

	if (!(keys & (KEY_A | KEY_TOUCH))) {
		return;
	}

	if (title_config.sfx_choice) {
		sfx_play(title_config.sfx_choice);
	}
	switch (choices[selected]->choice) {
		case TITLE_LANG:
			lang_next();
			break;
		case TITLE_INTRO:
			title_close();
			game_timeline_start(intro_timeline, NULL);
			break;

		case TITLE_CONTINUE:
			game_load();
			break;

		case TITLE_GAME:
			save_delete();
			game_start();
			break;

		case TITLE_CONTROLS:
			option = OPTION_CONTROLS;
			break;

		case TITLE_CREDITS:
			option = OPTION_CREDITS;
			break;

		default:
			break;
	}
}

void title_draw_top(void) {
	if (title_config.background_top.tex) {
		C2D_DrawImageAt(title_config.background_top, 0.0f, 0.0f, 0.0f, NULL, 1.0f, 1.0f);
	}
}

static void title_draw_credits(void) {
	C2D_TextBufClear(text_buf);
	C2D_Text role;
	C2D_Text person;
	if (title_config.background_credits.tex) {
		C2D_DrawImageAt(title_config.background_credits, 0.0f, 0.0f, 0.1f, NULL, 1.0f, 1.0f);
	}
	for (size_t i = 0; i < title_config.credit_count; i++) {
		float y = 20.0f + i * 20.0f;
		C2D_TextParse(&role, text_buf, lang_get(title_config.credits[i].role_id));
		C2D_TextParse(&person, text_buf, title_config.credits[i].name);
		C2D_TextOptimize(&role);
		C2D_TextOptimize(&person);
		C2D_DrawText(&role, C2D_WithColor, 20.0f, y, 0.5f, 0.4f, 0.4f, C2D_Color32(128, 128, 128, 255));
		C2D_DrawText(&person, C2D_WithColor | C2D_AlignRight, 300.0f, y, 0.5f, 0.4f, 0.4f, C2D_Color32(164, 164, 164, 255));
	}
	for (size_t i = 0; i < title_config.credits_image_count; i++) {
		TitleImage *credit = &title_config.credits_images[i];
		if (credit->image.tex) {
			C2D_DrawImageAt(credit->image, credit->x, credit->y, 0.5f, NULL, 1.0f, 1.0f);
		}
	}
}

static void title_draw_controls(void) {
	C2D_Text controls_text;
	if (title_config.background_controls.tex) {
		C2D_DrawImageAt(title_config.background_controls, 0.0f, 0.0f, 0.1f, NULL, 1.0f, 1.0f);
	}
	for (size_t i = 0; i < title_config.controls_image_count; i++) {
		TitleImage *ctrl_img = &title_config.controls_images[i];
		if (ctrl_img->image.tex) {
			C2D_DrawImageAt(ctrl_img->image, ctrl_img->x, ctrl_img->y, 0.3f, NULL, 1.0f, 1.0f);
		}
	}
	C2D_TextBufClear(text_buf);
	for (size_t i = 0; i < title_config.controls_text_count; i++) {
		TitleText *ctrl_text = &title_config.controls_texts[i];
		C2D_TextParse(&controls_text, text_buf, lang_get(ctrl_text->id));
		C2D_TextOptimize(&controls_text);
		C2D_DrawText(&controls_text, ctrl_text->flags, ctrl_text->x, ctrl_text->y, 0.5f, ctrl_text->size, ctrl_text->size, C2D_Color32(224, 224, 224, 255));
	}
}

static void title_draw_menu(void) {
	u32 color;
	C2D_Text text;
	C2D_TextBufClear(text_buf);
	if (title_config.background_bottom.tex) {
		C2D_DrawImageAt(title_config.background_bottom, 0.0f, 0.0f, 0.0f, NULL, 1.0f, 1.0f);
	}
	for (size_t i = 0; i < choice_count; i++) {
		if (selected == i) {
			color = title_config.menu.selected_color;
		} else {
			color = title_config.menu.color;
		}
		C2D_TextParse(&text, text_buf, lang_get(choices[i]->label));
		C2D_TextOptimize(&text);
		C2D_DrawText(&text, C2D_WithColor | C2D_AlignCenter, title_config.menu.x, (i * title_config.menu.spacing) + title_config.menu.y, 0.5f, title_config.menu.text_size, title_config.menu.text_size, color);
	}
	C2D_TextParse(&text, text_buf, VERSION);
	C2D_DrawText(&text, C2D_WithColor | C2D_AlignRight, 320.0f, 230.0f, 0.5f, 0.35f, 0.35f, C2D_Color32(64, 64, 64, 255));
}

void title_draw_bottom(void) {
	switch (option) {
		case OPTION_CONTROLS:
			title_draw_controls();
			break;

		case OPTION_CREDITS:
			title_draw_credits();
			break;

		default:
			title_draw_menu();
			break;
	}
}

// Releases the assets and resets the parsed configuration, so the next
// title_init (e.g. when coming back from the intro) starts from scratch
// instead of appending to the previous menu, controls and credits.
void title_close(void) {
	music_stop();
	for (size_t i = 0; i < title_config.controls_text_count; i++) {
		free(title_config.controls_texts[i].id);
	}
	for (size_t i = 0; i < title_config.credit_count; i++) {
		free(title_config.credits[i].role_id);
		free(title_config.credits[i].name);
	}
	free(title_config.sfx_select);
	free(title_config.sfx_choice);
	free(title_config.music);
	memset(&title_config, 0, sizeof(title_config));
	choice_count = 0;
	selected = 0;
	option = OPTION_NONE;
	if (text_buf) {
		C2D_TextBufDelete(text_buf);
		text_buf = NULL;
	}
	gfxmap_free_assets(&assets);
}
