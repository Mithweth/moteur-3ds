// game.c
// Game controller: the GameMode state machine, room input (circle pad
// movement, touch on hotspots, inventory keys), the message box, and the
// glue used by room actions and extensions to change mode. See game.h for the
// public API.
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "lang.h"
#include "inventory.h"
#include "gamestate.h"
#include "room.h"
#include "hud.h"
#include "audio.h"
#include "title.h"
#include "timeline.h"
#include "callbacks.h"
#include "str_utils.h"
#include "save.h"

#define GAME_CALLBACK_MAX     8
#define GAME_CONFIG_MAX_ITEMS 16
#define START_BOX_WIDTH      260.0f
#define START_BOX_HEIGHT     110.0f
#define START_BUTTON_SPACING  30.0f
#define START_BUTTON_PADDING   5.0f

typedef struct {
	u32 shadow;
	u32 outer_border;
	u32 outer_background;
	u32 inner_border;
	u32 inner_background;
} FrameStyle;

typedef struct {
	char *room;
	char *music;
	char *items[GAME_CONFIG_MAX_ITEMS];
	char *cannot_use_message;
	size_t item_count;
	FrameStyle frame;
	u32 text_color;
	float text_size;
	u32 button_color;
} GameConfig;

static GameConfig game_config;
static GameMode game_mode = GAME_NORMAL;
// Circle pad must go back to the dead zone before the next move is accepted,
// so holding the stick only moves one room.
static bool circle_ready = true;
// Last hotspot touched; a second touch on it runs its actions (see update_touch).
static Hotspot *active_hotspot = NULL;
// Hotspot shown in the HUD and used as the target of inventory items.
// Both pointers point into the current room and are reset by game_set_room.
static Hotspot *target = NULL;
// Message shown in GAME_MESSAGE; when examine_image is set, it is shown
// instead of the text.
static const char *message_text = NULL;
static C2D_Image examine_image;
static C2D_TextBuf text_buf;
static C2D_Text text;
// Called once the GAME_BUSY sound effect has finished playing.
static void (*game_busy_callback)(void) = NULL;
// Called when a timeline ends with RETURN and the game is back in GAME_NORMAL.
static void (*game_timeline_callback)(void) = NULL;
static MiniGame *active_minigame = NULL;
static int game_busy_sfx_channel = -1;
static bool start_menu;
static size_t start_selected;
static char current_room_name[64];
static GameMode timeline_return_mode;


// Parses value as "R G B A" into color. Returns false if value is NULL or
// one of the four components is missing.
static bool parse_color(char *value, u32 *color) {
	if (!value) {
		return false;
	}
	char *r = strtok(value, " ");
	char *g = strtok(NULL, " ");
	char *b = strtok(NULL, " ");
	char *a = strtok(NULL, " ");
	if (!r || !g || !b || !a) {
		return false;
	}
	*color = C2D_Color32(atoi(r), atoi(g), atoi(b), atoi(a));
	return true;
}

static bool game_config_load(const char *filename) {
	FILE *file = fopen(filename, "r");
	char line[512];

	if (!file) {
		printf("Cannot open %s\n", filename);
		return false;
	}

	bool frame_colors = false;
	bool has_frame_colors_block = false;
	size_t line_number = 0;

	game_config.text_color = C2D_Color32(255, 255, 255, 255);
	game_config.button_color = C2D_Color32(105, 82, 40, 255);
	game_config.text_size = 0.6f;

	while (fgets(line, sizeof(line), file)) {
		line_number++;
		char *p = str_trim(line);

		if (*p == '\0' || *p == '#') {
			continue;
		}

		char *command = strtok(p, " ");

		if (!command) {
			continue;
		}
		if (frame_colors) {
			bool valid;
			if (strcmp(command, "END_FRAME_COLORS") == 0) {
				frame_colors = false;
				continue;
			} else if (strcmp(command, "SHADOW") == 0) {
				valid = parse_color(strtok(NULL, "\n"), &game_config.frame.shadow);
			} else if (strcmp(command, "OUTER_BORDER") == 0) {
				valid = parse_color(strtok(NULL, "\n"), &game_config.frame.outer_border);
			} else if (strcmp(command, "OUTER_BACKGROUND") == 0) {
				valid = parse_color(strtok(NULL, "\n"), &game_config.frame.outer_background);
			} else if (strcmp(command, "INNER_BORDER") == 0) {
				valid = parse_color(strtok(NULL, "\n"), &game_config.frame.inner_border);
			} else if (strcmp(command, "INNER_BACKGROUND") == 0) {
				valid = parse_color(strtok(NULL, "\n"), &game_config.frame.inner_background);
			} else {
				printf("%s:%zu: unknown command: %s\n", filename, line_number, command);
				fclose(file);
				return false;
			}
			if (!valid) {
				printf("%s:%zu: invalid %s\n", filename, line_number, command);
				fclose(file);
				return false;
			}
			continue;
		}

		if (strcmp(command, "ROOM") == 0) {
			char *value = strtok(NULL, " ");
			if (!value) {
				printf( "%s:%zu: invalid %s\n", filename, line_number, command);
				fclose(file);
				return false;
			}
			if (game_config.room) {
				printf("%s:%zu: duplicate %s\n", filename, line_number, command);
				fclose(file);
				return false;
			}
			game_config.room = strdup(value);
			continue;
		} else if (strcmp(command, "TEXT_COLOR") == 0) {
			if (!parse_color(strtok(NULL, "\n"), &game_config.text_color)) {
				printf("%s:%zu: invalid %s\n", filename, line_number, command);
				fclose(file);
				return false;
			}
			continue;
		} else if (strcmp(command, "TEXT_SIZE") == 0) {
			char *s = strtok(NULL, " ");
			if (!s) {
				printf( "%s:%zu: invalid %s\n", filename, line_number, command);
				fclose(file);
				return false;
			}
			game_config.text_size = atof(s);
			continue;
		} else if (strcmp(command, "BUTTON_COLOR") == 0) {
			if (!parse_color(strtok(NULL, "\n"), &game_config.button_color)) {
				printf("%s:%zu: invalid %s\n", filename, line_number, command);
				fclose(file);
				return false;
			}
			continue;
		} else if (strcmp(command, "MUSIC") == 0) {
			char *value = strtok(NULL, " ");
			if (!value) {
				printf( "%s:%zu: invalid %s\n", filename, line_number, command);
				fclose(file);
				return false;
			}
			if (game_config.music) {
				printf("%s:%zu: duplicate %s\n", filename, line_number, command);
				fclose(file);
				return false;
			}
			// Like the title screen's MUSIC: a relative name is looked up in
			// romfs:/game with .ogg appended, a romfs:/ path is kept as is.
			game_config.music = audio_resolve_path("romfs:/game", value, ".ogg");
			continue;
		} else if (strcmp(command, "SAVE") == 0) {
			char *value = strtok(NULL, " ");
			if (!value) {
				printf( "%s:%zu: invalid %s\n", filename, line_number, command);
				fclose(file);
				return false;
			}
			save_set_filename(value);
			continue;
		} else if (strcmp(command, "CANNOT_USE_MESSAGE") == 0) {
			char *value = strtok(NULL, " ");
			if (!value) {
				printf( "%s:%zu: invalid %s\n", filename, line_number, command);
				fclose(file);
				return false;
			}
			if (game_config.cannot_use_message) {
				printf("%s:%zu: duplicate CANNOT_USE_MESSAGE\n", filename, line_number);
				fclose(file);
				return false;
			}
			game_config.cannot_use_message = strdup(value);
			continue;
		} else if (strcmp(command, "ITEM") == 0) {
			char *value = strtok(NULL, " ");
			if (!value) {
				printf("%s:%zu: invalid %s\n", filename, line_number, command);
				fclose(file);
				return false;
			}
			if (game_config.item_count >= GAME_CONFIG_MAX_ITEMS) {
				printf("%s:%zu: too many ITEM (max %d)\n", filename, line_number, GAME_CONFIG_MAX_ITEMS);
				fclose(file);
				return false;
			}
			game_config.items[game_config.item_count++] = strdup(value);
			continue;
		} else if (strcmp(command, "FRAME_COLORS") == 0) {
			frame_colors = true;
			has_frame_colors_block = true;
			continue;
		}
		printf("%s:%zu: unknown command: %s\n", filename, line_number, command);
		fclose(file);
		return false;
	}
	fclose(file);
	if (frame_colors) {
		printf("%s: missing END_FRAME_COLORS\n", filename);
		return false;
	}
	if (!has_frame_colors_block) {
		printf("%s: missing FRAME_COLORS block\n", filename);
		return false;
	}
	if (!game_config.room) {
		printf("%s: Missing ROOM\n", filename);
		return false;
	}
	return true;
}

static bool can_save() {
	return game_mode == GAME_NORMAL && save_enabled();
}

void game_minigame_start(const char *name) {
	active_minigame = callbacks_minigame_find(name);
	if (!active_minigame) {
		printf("Unknown mini-game: %s\n", name);
		return;
	}
	music_stop();
	if (active_minigame->init) {
		if (!active_minigame->init()) {
			printf("Fail loading mini-game\n");
			game_minigame_stop();
			return;
		}
	}
	game_mode = GAME_MINIGAME;
}

void game_minigame_stop(void) {
	if (active_minigame && active_minigame->close) {
		active_minigame->close();
	}
	if (game_config.music) {
		music_play(game_config.music);
	}
	active_minigame = NULL;
	game_mode = GAME_NORMAL;
}

const char *game_get_room(void) {
	return current_room_name;
}

bool game_set_room(const char *name) {
	// name usually comes from a ROOM action owned by the current room, which
	// room_close is about to free: copy it first.
	char room_name[64];
	snprintf(room_name, sizeof(room_name), "%s", name);
	room_close();
	active_hotspot = NULL;
	target = NULL;
	game_mode = GAME_NORMAL;
	message_text = NULL;
	examine_image = (C2D_Image){0};
	if (!room_init(room_name)) {
		printf("Cannot enter room: %s\n", room_name);
		// Stay where the player was rather than on an empty screen. The
		// previous room loaded fine a moment ago, so this should work.
		if (current_room_name[0] && !room_init(current_room_name)) {
			printf("Cannot reload room: %s\n", current_room_name);
		}
		return false;
	}
	snprintf(current_room_name, sizeof(current_room_name), "%s", room_name);
	return true;
}

// Closes whatever the current mode may hold (mini-game, room, title screen,
// timeline) and stops the music. Shared by game_title_start, which then opens
// the title screen, and game_close at exit. Safe when nothing is open.
static void game_close_modes(void) {
	if (active_minigame && active_minigame->close) {
		active_minigame->close();
	}
	active_minigame = NULL;
	room_close();
	active_hotspot = NULL;
	target = NULL;
	message_text = NULL;
	game_busy_callback = NULL;
	game_timeline_callback = NULL;
	title_close();
	timeline_close();
	music_stop();
}

void game_close(void) {
	game_close_modes();
	if (text_buf) {
		C2D_TextBufDelete(text_buf);
		text_buf = NULL;
	}
	free(game_config.room);
	free(game_config.music);
	free(game_config.cannot_use_message);
	for (size_t i = 0; i < game_config.item_count; i++) {
		free(game_config.items[i]);
	}
	memset(&game_config, 0, sizeof(game_config));
	hud_close();
	inventory_close();
	// Last, in reverse order of game_init, which starts with callbacks_init.
	callbacks_close();
}

// An action may have hidden the target since it was selected (its WHEN
// conditions no longer match). Fall back to an available hotspot with the same
// id, e.g. an open door replacing a closed one, or to no target at all.
// Called lazily wherever target is read, so every path is covered.
static void refresh_target(void) {
	if (target && !room_hotspot_is_available(target)) {
		target = room_find_hotspot_by_id(target->id);
	}
}

const char *game_target_name(void) {
	refresh_target();
	if (!target) {
		return NULL;
	}
	return target->id;
}

bool game_title_start(void) {
	game_close_modes();
	game_mode = GAME_TITLE;
	if (!title_init()) {
		printf("Cannot initialize title screen\n");
		return false;
	}
	return true;
}

// A timeline ending with RETURN, started from the game: back to the room,
// which stayed loaded meanwhile; timeline_close stops the timeline's music, so
// the game music restarts from the beginning. The callback is cleared before
// it runs, so it may start another timeline. END, or RETURN from the title
// screen (no game to go back to): title screen, and the callback is dropped.
static void game_timeline_stop(void) {
	if (timeline_exit() != TIMELINE_EXIT_RETURN || timeline_return_mode == GAME_TITLE) {
		game_title_start();
		return;
	}
	timeline_close();
	if (game_config.music) {
		music_play(game_config.music);
	}
	game_mode = GAME_NORMAL;
	if (game_timeline_callback) {
		void (*callback)(void) = game_timeline_callback;
		game_timeline_callback = NULL;
		callback();
	}
}

bool game_timeline_start(const char *name, void (*callback)(void)) {
	char path[256];
	// The timeline takes over the screen: close the mini-game now, without
	// restarting the game music as game_minigame_stop would. A pending
	// WAIT_SFX callback is dropped too, since RETURN goes to GAME_NORMAL.
	if (active_minigame && active_minigame->close) {
		active_minigame->close();
	}
	active_minigame = NULL;
	game_busy_callback = NULL;
	timeline_return_mode = (game_mode == GAME_TITLE) ? GAME_TITLE : GAME_NORMAL;
	snprintf(path, sizeof(path), "romfs:/timelines/%s", name);
	if (!timeline_init(path)) {
		game_title_start();
		return false;
	}
	game_timeline_callback = callback;
	game_mode = GAME_TIMELINE;
	return true;
}

bool game_init(void) {
	callbacks_init();
	if (!game_config_load("romfs:/game/game")) {
		printf("Cannot load game configuration\n");
		return false;
	}
	if (!inventory_init()) {
		printf("Cannot initialize inventory\n");
		return false;
	}
	if (!hud_init()) {
		printf("Cannot initialize HUD\n");
		return false;
	}
	// Created here rather than in game_start: the quit confirmation also
	// draws its text on the title screen, before any game has started.
	text_buf = C2D_TextBufNew(4096);
	if (!text_buf) {
		printf("Cannot create game text buffer\n");
		return false;
	}
	return game_title_start();
}

static void game_reset(void) {
	title_close();
	callbacks_reset();
	inventory_reset();
	gamestate_reset();
	hud_reset();
	game_mode = GAME_NORMAL;
	message_text = NULL;
	active_hotspot = NULL;
	// No room yet in this game: game_set_room must not fall back to the room
	// of the previous game, and save_write must not save it.
	current_room_name[0] = '\0';
	if (game_config.music) {
		music_play(game_config.music);
	}
}

void game_load(void) {
	game_reset();
	if (save_read()) {
		game_show_message("SAVE_LOADING_SUCCESS");
	} else if (game_start()) {
		// When game_start fails, it is back on the title screen: a message
		// would switch to GAME_MESSAGE on top of it.
		game_show_message("SAVE_LOADING_ERROR");
	}
}

bool game_start(void) {
	game_reset();
	for (size_t i = 0; i < game_config.item_count; i++) {
		inventory_add(game_config.items[i]);
	}
	if (!game_set_room(game_config.room)) {
		// No previous room to fall back to: the game cannot start.
		game_title_start();
		return false;
	}
	return true;
}

bool game_wait_for_sfx(const char *sfx, void (*callback)(void)) {
	game_busy_sfx_channel = sfx_play(sfx);
	if (game_busy_sfx_channel < 0) {
		return false;
	}
	game_busy_callback = callback;
	game_mode = GAME_BUSY;
	return true;
}

// Turns the circle pad position into one of eight directions. A direction is
// a cardinal one when one axis is more than twice the other, otherwise a
// diagonal. Only one move is made per push of the stick.
static void update_movement(circlePosition analog) {
	const int DEADZONE = 60;

	int x = analog.dx;
	int y = analog.dy;

	if (abs(x) < DEADZONE && abs(y) < DEADZONE) {
		circle_ready = true;
		return;
	}

	if (!circle_ready) {
		return;
	}

	int ax = abs(x);
	int ay = abs(y);

	if (ay > ax * 2) {
		if (y > 0) {
			circle_ready = false;
			room_move_north();
			return;
		} else if (y < 0) {
			circle_ready = false;
			room_move_south();
			return;
		}
	} else if (ax > ay * 2) {
		if (x > 0) {
			circle_ready = false;
			room_move_east();
			return;
		} else if (x < 0) {
			circle_ready = false;
			room_move_west();
			return;
		}
	} else {
		if (x > 0 && y > 0) {
			circle_ready = false;
			room_move_northeast();
			return;
		} else if (x < 0 && y > 0) {
			circle_ready = false;
			room_move_northwest();
			return;
		} else if (x > 0 && y < 0) {
			circle_ready = false;
			room_move_southeast();
			return;
		} else if (x < 0 && y < 0) {
			circle_ready = false;
			room_move_southwest();
			return;
		}
	}
}

// Handles a tap on the room. The first tap on a hotspot selects it as target
// and, if it has a MESSAGE or a MESSAGE_IMAGE, only shows it (MESSAGE wins
// when both are set); tapping the same hotspot again runs its ACTION blocks.
// Hotspots with neither run their actions on the first tap.
static void update_touch(touchPosition touch) {
	if (active_hotspot && !room_hotspot_is_available(active_hotspot)) {
		active_hotspot = NULL;
	}

	Hotspot *hotspot = room_find_hotspot(touch.px, touch.py);
	if (!hotspot) {
		return;
	}

	if (hotspot != active_hotspot) {
		active_hotspot = hotspot;
		target = hotspot;

		if (hotspot->message_id) {
			game_show_message(hotspot->message_id);
			return;
		}

		if (hotspot->message_image.tex) {
			game_show_image(hotspot->message_image);
			return;
		}
	}

	room_execute_hotspot_action(hotspot);
}


// Draws the golden-framed dark box used by the message box and the quit
// confirmation, with a drop shadow. Uses depths z - 0.01 to z + 0.03.
static void draw_framed_box(float x, float y, float w, float h, float z) {
	C2D_DrawRectSolid(x + 3.0f, y + 3.0f, z - 0.01f, w, h, game_config.frame.shadow);
	C2D_DrawRectSolid(x, y, z, w, h, game_config.frame.outer_border);
	C2D_DrawRectSolid(x + 2.0f, y + 2.0f, z + 0.01f, w - 4.0f, h - 4.0f, game_config.frame.outer_background);
	C2D_DrawRectSolid(x + 5.0f, y + 5.0f, z + 0.02f, w - 10.0f, h - 10.0f, game_config.frame.inner_border);
	C2D_DrawRectSolid(x + 6.0f, y + 6.0f, z + 0.03f, w - 12.0f, h - 12.0f, game_config.frame.inner_background);
}

// Draws the room on the bottom screen, plus the examine image or the message
// box when in GAME_MESSAGE. The text is parsed again on every frame.
static void game_draw_room(void) {
	room_draw();
	if (game_mode == GAME_MESSAGE) {
		if (examine_image.tex) {
			float x = (320 - examine_image.subtex->width) / 2;
			float y = (240 - examine_image.subtex->height) / 2;
			C2D_DrawRectSolid(0.0f, 0.0f, 0.8f, 320, 240, C2D_Color32(0, 0, 0, 180));
			C2D_DrawImageAt(examine_image, x, y, 0.9f, NULL, 1.0f, 1.0f);
		} else {
			C2D_TextBufClear(text_buf);
			C2D_TextParse(&text, text_buf, message_text);
			C2D_TextOptimize(&text);
			float width, height;
			C2D_TextGetDimensions(&text, 0.5f, 0.5f, &width, &height);
			float box_height = height + 20.0f;
			float box_y = 240.0f - box_height - 10.0f;
			draw_framed_box(10.0f, box_y, 300.0f, box_height, 0.80f);
			C2D_DrawText(&text, C2D_WithColor, 22.0f, box_y + 10.0f, 0.9f, 0.5f, 0.5f, game_config.text_color);
		}
	}
}

// Draws a centered label at (x, y), highlighted when selected. Appends to
// text_buf without clearing it: the caller clears it first.
static void draw_button(const char *label_id, float x, float y, bool selected) {
	float width, height;
	C2D_TextParse(&text, text_buf, lang_get(label_id));
	C2D_TextOptimize(&text);
	C2D_TextGetDimensions(&text, game_config.text_size, game_config.text_size, &width, &height);
	if (selected) {
		C2D_DrawRectSolid(x - width / 2.0f - START_BUTTON_PADDING, y - height / 2.0f - START_BUTTON_PADDING, 0.95f, width + 2.0f * START_BUTTON_PADDING, height + 2.0f * START_BUTTON_PADDING, game_config.button_color);
	}
	C2D_DrawText(&text, C2D_WithColor | C2D_AlignCenter, x, y - height / 2.0f, 0.96f, game_config.text_size, game_config.text_size, game_config.text_color);
}

// Draws the quit confirmation centered on the bottom screen: a dimmed
// background, the question, and the No (left) / Yes (right) buttons.
static void start_menu_draw(void) {
	float box_x = (320.0f - START_BOX_WIDTH) / 2.0f;
	float box_y = (240.0f - START_BOX_HEIGHT) / 2.0f;
	C2D_DrawRectSolid(0.0f, 0.0f, 0.85f, 320.0f, 240.0f, C2D_Color32(0, 0, 0, 128));
	draw_framed_box(box_x, box_y, START_BOX_WIDTH, START_BOX_HEIGHT, 0.90f);
	C2D_TextBufClear(text_buf);
	float center_y = box_y + START_BOX_HEIGHT / 2.0f;

	draw_button("GAME_OPTION_BACK", 160.0f, center_y - START_BUTTON_SPACING, (start_selected == 0));
	if (can_save()) {
		draw_button("GAME_OPTION_SAVE", 160.0f, center_y, (start_selected == 1));
	}
	draw_button("GAME_OPTION_QUIT", 160.0f, center_y + START_BUTTON_SPACING, (start_selected == 2));
}


bool game_use_item(const char *id) {
	refresh_target();
	if (!target) {
		return false;
	}
	
	printf("Use item: %s\n", id);
	return room_execute_hotspot_use(target, id);
}

void game_cannot_use_item(void) {
	refresh_target();
	if (!target || !game_config.cannot_use_message) {
		return;
	}
	game_show_message(game_config.cannot_use_message);
}

void game_show_message(const char *message_id) {
	examine_image = (C2D_Image){0};
	message_text = lang_get(message_id);
	printf("Print: %s\n", message_id);
	game_mode = GAME_MESSAGE;
}

void game_show_image(C2D_Image image) {
	examine_image = image;
	game_mode = GAME_MESSAGE;
}

bool game_update(u32 keys, circlePosition analog, touchPosition touch) {
	if (start_menu) {
		bool activate = (keys & KEY_A);

		if (keys & KEY_UP) {
			if (start_selected > 0) {
				start_selected--;
			}
			if (!can_save() && start_selected == 1) {
				start_selected--;
			}
		}

		if (keys & KEY_DOWN) {
			if (start_selected < 2) {
				start_selected++;
			}
			if (!can_save() && start_selected == 1) {
				start_selected++;
			}
		}
		if (keys & KEY_TOUCH) {
			float box_x = (320.0f - START_BOX_WIDTH) / 2.0f;
			float box_y = (240.0f - START_BOX_HEIGHT) / 2.0f;
			float center_y = box_y + START_BOX_HEIGHT / 2.0f;
			float button_y[] = {center_y - START_BUTTON_SPACING, center_y, center_y + START_BUTTON_SPACING};

			if (touch.px >= box_x && touch.px <= box_x + START_BOX_WIDTH) {
				for (int i = 0; i < 3; i++) {
					if (!can_save() && i == 1) {
						continue;
					}
					if (touch.py >= button_y[i] - START_BUTTON_SPACING / 2.0f && touch.py < button_y[i] + START_BUTTON_SPACING / 2.0f) {
						start_selected = i;
						activate = true;
						break;
					}
				}
			}
		}

		if (keys & (KEY_B | KEY_START)) {
			start_menu = false;
			timer_resume();
			return true;
		}

		if (activate) {
			switch (start_selected) {
			case 0:
				start_menu = false;
				break;
			case 1:
				start_menu = false;
				if (!save_write()) {
					game_show_message("SAVE_SAVING_ERROR");
				} else {
					game_show_message("SAVE_SAVING_SUCCESS");
				}
				break;
			case 2:
				return false;
			}
			timer_resume();
		}
		return true;
	}

	if (keys & KEY_START) {
		start_menu = true;
		start_selected = 0;
		if (game_mode == GAME_MESSAGE) {
			game_mode = GAME_NORMAL;
			examine_image = (C2D_Image){0};
		}
		return true;
	}

	if (game_mode != GAME_TITLE && game_mode != GAME_TIMELINE && hud_update()) {
		return true;
	}

	switch (game_mode) {
		case GAME_TITLE:
			title_update(keys, touch);
			return true;

		case GAME_TIMELINE:
			if (!timeline_update(keys)) {
				game_timeline_stop();
			}
			return true;

		case GAME_MINIGAME:
			if (active_minigame && active_minigame->update) {
				active_minigame->update(keys, touch);
			}
			return true;

		case GAME_MESSAGE:
			if (keys & (KEY_A | KEY_B | KEY_TOUCH)) {
				game_mode = GAME_NORMAL;
				examine_image = (C2D_Image){0};
			}
			return true;

		case GAME_BUSY:
			// The callback is cleared before being called because it may
			// start a new wait (e.g. the next WAIT_SFX of a room action list).
			if (!sfx_is_playing(game_busy_sfx_channel)) {
				game_mode = GAME_NORMAL;
				if (game_busy_callback) {
					void (*callback)(void) = game_busy_callback;
					game_busy_callback = NULL;
					game_busy_sfx_channel = -1;
					callback();
				}
			}
			return true;

		default:
			break;
	}

	// GAME_NORMAL: the inventory gets the keys first (D-pad, A, X); when it
	// consumes them, no movement or touch is handled this frame.
	if ((inventory_is_active()) | (inventory_update(keys))) {
		return true;
	}

	update_movement(analog);

	if (keys & KEY_TOUCH) {
		update_touch(touch);
	}
	return true;
}

void game_draw(C3D_RenderTarget *top, C3D_RenderTarget *bottom) {
	C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
	C2D_TargetClear(bottom, C2D_Color32(0, 0, 0, 255));
	switch (game_mode) {
		case GAME_TITLE:
			C2D_SceneBegin(bottom);
			title_draw_bottom();
			C2D_SceneBegin(top);
			title_draw_top();
			break;

		case GAME_TIMELINE:
			C2D_SceneBegin(bottom);
			timeline_draw_bottom();
			C2D_SceneBegin(top);
			timeline_draw_top();
			break;

		case GAME_MINIGAME:
			// The mini-game draws over the room, with a higher depth.
			C2D_SceneBegin(bottom);
			room_draw();
			if (active_minigame && active_minigame->draw) {
				active_minigame->draw();
			}
			C2D_SceneBegin(top);
			hud_draw();
			break;

		default:
			C2D_SceneBegin(bottom);
			game_draw_room();
			C2D_SceneBegin(top);
			hud_draw();
			break;
	}
	if (start_menu) {
		C2D_SceneBegin(bottom);
		start_menu_draw();
	}
}
