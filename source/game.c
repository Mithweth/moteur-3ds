// game.c
// Game controller: the GameMode state machine, room input (circle pad
// movement, touch on hotspots, inventory keys), the message box, and the
// glue used by room actions and extensions to change mode. See game.h for the
// public API.
// Implementation notes: the START menu and the choices opened by CHOICE
// actions share one overlay, drawn on the bottom screen over the current mode
// and fed by the same code (start_choice is a static Choice whose options are
// handled here instead of by room.c). Only one overlay is shown at a time:
// opening the START menu puts the current choice and its selection aside in
// pending_choice, closing it brings them back. The START menu works in every
// mode and pauses it (hud_update is skipped, then timer_resume); a room choice
// only takes the input in GAME_NORMAL and lets the countdown run.
#include <3ds.h>
#include <math.h>
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
#include "text_render.h"
#include "save.h"

#define GAME_CALLBACK_MAX     16
#define GAME_CONFIG_MAX_ITEMS 16
// Height of a line of text at scale 1.0, used to space the choice buttons.
#define FONT_HEIGHT           30.0f

typedef struct {
	u32 shadow;
	u32 outer_border;
	u32 outer_background;
	u32 inner_border;
	u32 inner_background;
} FrameStyle;

// CHOICE block of the game file: look of the choice box and START menu.
typedef struct {
	u32 button_color;
	u32 button_text_color;
	u32 prompt_text_color;
	float button_text_size;
	float prompt_text_size;
	float frame_width;
	float button_padding;
} ChoiceStyle;

// MESSAGE block of the game file: text of the message box.
typedef struct {
	u32 text_color;
	float text_size;
} MessageStyle;

typedef struct {
	char *room;
	char *music;
	char *items[GAME_CONFIG_MAX_ITEMS];
	char *cannot_use_message;
	size_t item_count;
	FrameStyle frame;
	ChoiceStyle choice;
	MessageStyle message;
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
// Message shown in GAME_MESSAGE, owned by game.c: wrapped to the message box
// width by game_show_message (raw copy if wrapping fails), freed when replaced
// or reset. When examine_image is set, it is shown instead of the text.
static char *message_text = NULL;
static C2D_Image examine_image;
static C2D_TextBuf text_buf;
static C2D_Text text;
// Called by game_resume when GAME_WAIT_SFX, GAME_MESSAGE or GAME_TIMELINE
// (ended with RETURN) gives way to GAME_NORMAL. Entering any of these modes
// overwrites it, so a mode replaced before its end drops its callback.
static void (*game_resume_callback)(void) = NULL;
static MiniGame *active_minigame = NULL;
static int wait_sfx_channel = -1;
static char current_room_name[64];
static GameMode timeline_return_mode;
// Center of the first button of the overlay, computed when it is drawn and
// used to hit-test touches on the next frame.
static float choice_first_button_y;

// The START menu, shown through the same overlay as the room choices. Its
// option indexes are used directly by game_update and choice_option_available
// (0 back, 1 save, 2 quit).
static Choice start_choice = {
	.name = "START",
	.prompt = "GAME_OPTION_TITLE",
	.option_count = 3,
	.options = {
		{ .name = "GAME_OPTION_BACK" },
		{ .name = "GAME_OPTION_SAVE" },
		{ .name = "GAME_OPTION_QUIT" }
	}
};

// Choice shown on the bottom screen (NULL when none) and its selected
// option index.
static Choice *overlay_choice = NULL;
static size_t overlay_choice_selected = 0;
// Room choice put aside while the START menu is open, restored when it closes.
static Choice *pending_choice = NULL;
static size_t pending_choice_selected = 0;


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
	bool choice_block = false;
	bool message_block = false;
	bool has_frame_colors_block = false;
	size_t line_number = 0;

	game_config.choice.prompt_text_color = C2D_Color32(255, 255, 255, 255);
	game_config.message.text_color = C2D_Color32(255, 255, 255, 255);
	game_config.choice.button_color = C2D_Color32(105, 82, 40, 255);
	game_config.choice.button_text_color = C2D_Color32(255, 255, 255, 255);
	game_config.message.text_size = 0.6f;
	game_config.choice.prompt_text_size = 0.6f;
	game_config.choice.button_text_size = 0.6f;
	game_config.choice.frame_width = 276.0f;
	game_config.choice.button_padding = 5.0f;

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
		if (choice_block) {
			bool valid;
			if (strcmp(command, "END_CHOICE") == 0) {
				choice_block = false;
				continue;
			} else if (strcmp(command, "BUTTON_TEXT_COLOR") == 0) {
				valid = parse_color(strtok(NULL, "\n"), &game_config.choice.button_text_color);
			} else if (strcmp(command, "BUTTON_COLOR") == 0) {
				valid = parse_color(strtok(NULL, "\n"), &game_config.choice.button_color);
			} else if (strcmp(command, "PROMPT_TEXT_COLOR") == 0) {
				valid = parse_color(strtok(NULL, "\n"), &game_config.choice.prompt_text_color);
			} else if (strcmp(command, "BUTTON_TEXT_SIZE") == 0) {
				char *s = strtok(NULL, " ");
				valid = (s != NULL);
				if (valid) {
					game_config.choice.button_text_size = atof(s);
				}
			} else if (strcmp(command, "PROMPT_TEXT_SIZE") == 0) {
				char *s = strtok(NULL, " ");
				valid = (s != NULL);
				if (valid) {
					game_config.choice.prompt_text_size = atof(s);
				}
			} else if (strcmp(command, "FRAME_WIDTH") == 0) {
				char *s = strtok(NULL, " ");
				valid = (s != NULL);
				if (valid) {
					game_config.choice.frame_width = atof(s);
				}
			} else if (strcmp(command, "BUTTON_PADDING") == 0) {
				char *s = strtok(NULL, " ");
				valid = (s != NULL);
				if (valid) {
					game_config.choice.button_padding = atof(s);
				}
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
		if (message_block) {
			bool valid;
			if (strcmp(command, "END_MESSAGE") == 0) {
				message_block = false;
				continue;
			} else if (strcmp(command, "TEXT_COLOR") == 0) {
				valid = parse_color(strtok(NULL, "\n"), &game_config.message.text_color);
			} else if (strcmp(command, "TEXT_SIZE") == 0) {
				char *s = strtok(NULL, " ");
				valid = (s != NULL);
				if (valid) {
					game_config.message.text_size = atof(s);
				}
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
		} else if (strcmp(command, "CHOICE") == 0) {
			choice_block = true;
			continue;
		} else if (strcmp(command, "MESSAGE") == 0) {
			message_block = true;
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
	if (message_block) {
		printf("%s: missing END_MESSAGE\n", filename);
		return false;
	}
	if (choice_block) {
		printf("%s: missing END_CHOICE\n", filename);
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

// Saving is offered only while the player explores a room: not during a
// message, a sound, a mini-game or a timeline, nor while a choice is open,
// since the save would not record it.
static bool can_save(void) {
	return game_mode == GAME_NORMAL && !pending_choice && save_enabled();
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
	free(message_text);
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
	free(message_text);
	message_text = NULL;
	game_resume_callback = NULL;
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
	free(message_text);
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

// Back to GAME_NORMAL, then runs game_resume_callback. It is cleared before
// it runs, since it may enter another waiting mode (e.g. the next action of a
// room block).
static void game_resume(void) {
	void (*callback)(void) = game_resume_callback;
	game_resume_callback = NULL;
	game_mode = GAME_NORMAL;
	if (callback) {
		callback();
	}
}

// A timeline ending with RETURN, started from the game: back to the room,
// which stayed loaded meanwhile; timeline_close stops the timeline's music, so
// the game music restarts from the beginning, then game_resume runs the
// callback. END, or RETURN from the title screen (no game to go back to):
// title screen, and the callback is dropped.
static void game_timeline_stop(void) {
	if (timeline_exit() != TIMELINE_EXIT_RETURN || timeline_return_mode == GAME_TITLE) {
		game_title_start();
		return;
	}
	timeline_close();
	if (game_config.music) {
		music_play(game_config.music);
	}
	game_resume();
}

bool game_timeline_start(const char *name, void (*callback)(void)) {
	char path[256];
	// The timeline takes over the screen: close the mini-game now, without
	// restarting the game music as game_minigame_stop would. The callback of
	// a message or WAIT_SFX replaced by the timeline is dropped when
	// game_resume_callback is overwritten below.
	if (active_minigame && active_minigame->close) {
		active_minigame->close();
	}
	active_minigame = NULL;
	timeline_return_mode = (game_mode == GAME_TITLE) ? GAME_TITLE : GAME_NORMAL;
	snprintf(path, sizeof(path), "romfs:/timelines/%s", name);
	if (!timeline_init(path)) {
		game_title_start();
		return false;
	}
	game_resume_callback = callback;
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
	free(message_text);
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
		game_show_message("SAVE_LOADING_SUCCESS", NULL);
	} else if (game_start()) {
		// When game_start fails, it is back on the title screen: a message
		// would switch to GAME_MESSAGE on top of it.
		game_show_message("SAVE_LOADING_ERROR", NULL);
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
	wait_sfx_channel = sfx_play(sfx);
	if (wait_sfx_channel < 0) {
		return false;
	}
	game_resume_callback = callback;
	game_mode = GAME_WAIT_SFX;
	return true;
}

// Turns the circle pad position into one of eight directions, each covering
// a 45° sector. A move fires once the stick goes past TRIGGER, and the next
// one waits until the stick has come back under REARM, so that one push
// makes exactly one move.
static void update_movement(circlePosition analog) {
	const int TRIGGER = 100;
	const int REARM = 40;

	int x = analog.dx;
	int y = analog.dy;
	int r2 = x * x + y * y;

	if (r2 < REARM * REARM) {
		circle_ready = true;
		return;
	}
	if (!circle_ready || r2 < TRIGGER * TRIGGER) {
		return;
	}

	// Angle in degrees, 0 = east, counter-clockwise, folded into 0..360
	float angle = atan2f(y, x) * 180.0f / (float)M_PI;
	if (angle < 0) {
		angle += 360.0f;
	}

	circle_ready = false;

	if (angle < 22.5f || angle >= 337.5f) {
		room_move_east();
	} else if (angle < 67.5f) {
		room_move_northeast();
	} else if (angle < 112.5f) {
		room_move_north();
	} else if (angle < 157.5f) {
		room_move_northwest();
	} else if (angle < 202.5f) {
		room_move_west();
	} else if (angle < 247.5f) {
		room_move_southwest();
	} else if (angle < 292.5f) {
		room_move_south();
	} else {
		room_move_southeast();
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
			game_show_message(hotspot->message_id, NULL);
			return;
		}

		if (hotspot->message_image.tex) {
			game_show_image(hotspot->message_image, NULL);
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
			C2D_TextGetDimensions(&text, game_config.message.text_size, game_config.message.text_size, &width, &height);
			float box_height = height + 20.0f;
			float box_y = 240.0f - box_height - 10.0f;
			draw_framed_box(10.0f, box_y, 300.0f, box_height, 0.80f);
			C2D_DrawText(&text, C2D_WithColor, 22.0f, box_y + 10.0f, 0.9f, game_config.message.text_size, game_config.message.text_size, game_config.message.text_color);
		}
	}
}

// Draws a centered label at (x, y), highlighted when selected. Appends to
// text_buf without clearing it: the caller clears it first.
static void draw_button(const char *label_id, float x, float y, bool selected) {
	float width, height;
	C2D_TextParse(&text, text_buf, lang_get(label_id));
	C2D_TextOptimize(&text);
	C2D_TextGetDimensions(&text, game_config.choice.button_text_size, game_config.choice.button_text_size, &width, &height);
	if (selected) {
		C2D_DrawRectSolid(x - width / 2.0f - game_config.choice.button_padding, y - height / 2.0f - game_config.choice.button_padding, 0.98f, width + 2.0f * game_config.choice.button_padding, height + 2.0f * game_config.choice.button_padding, game_config.choice.button_color);
	}
	C2D_DrawText(&text, C2D_WithColor | C2D_AlignCenter, x, y - height / 2.0f, 0.99f, game_config.choice.button_text_size, game_config.choice.button_text_size, game_config.choice.button_text_color);
}

// True if option index of choice gets a button: the WHEN conditions of a room
// choice, or "Save" only when saving is possible for the START menu.
static bool choice_option_available(Choice *choice, size_t index) {
	if (choice == &start_choice) {
		return index != 1 || can_save();
	}
	return room_choice_option_is_available(&choice->options[index]);
}

// Index of the first available option, or 0 when there is none (game_update
// then closes the choice).
static size_t choice_first_available(Choice *choice) {
	for (size_t i = 0; i < choice->option_count; i++) {
		if (choice_option_available(choice, i)) {
			return i;
		}
	}
	return 0;
}

// Number of available options, i.e. of buttons drawn.
static size_t choice_visible_count(Choice *choice) {
	size_t visible_count = 0;
	for (size_t i = 0; i < choice->option_count; i++) {
		if (choice_option_available(choice, i)) {
			visible_count++;
		}
	}
	return visible_count;
}

// Draws overlay_choice centered on the bottom screen: a dimmed background,
// the framed box, the wrapped prompt, then one button per available option.
// Draws nothing when no option is available (game_update closes the choice).
static void game_draw_choice(void) {
	size_t visible_count = choice_visible_count(overlay_choice);
	if (visible_count == 0) {
		return;
	}
	C2D_TextBufClear(text_buf);

	// Prompt (not selectable).
	C2D_Text text;
	char *wrapped = text_render_wrap(lang_get(overlay_choice->prompt), game_config.choice.frame_width - 12.0f, game_config.choice.prompt_text_size);
	if (!wrapped) {
		wrapped = strdup(lang_get(overlay_choice->prompt));
	}
	C2D_TextParse(&text, text_buf, wrapped);
	free(wrapped);
	C2D_TextOptimize(&text);
	float width, height;
	C2D_TextGetDimensions(&text, game_config.choice.prompt_text_size, game_config.choice.prompt_text_size, &width, &height);

	float button_spacing = FONT_HEIGHT * game_config.choice.button_text_size + 2.0f * game_config.choice.button_padding;
	float box_height = height + (visible_count + 1) * button_spacing + 24.0f;
	float box_x = (320.0f - game_config.choice.frame_width) / 2.0f;
	float box_y = (240.0f - box_height) / 2.0f;
	float y = box_y + 12.0f + button_spacing / 2.0f;

	C2D_DrawRectSolid(0.0f, 0.0f, 0.93f, 320.0f, 240.0f, C2D_Color32(0, 0, 0, 192));
	draw_framed_box(box_x, box_y, game_config.choice.frame_width, box_height, 0.95f);

	C2D_DrawText(&text, C2D_AlignCenter | C2D_WithColor, 160.0f, y - 9.0f, 0.99f, game_config.choice.prompt_text_size, game_config.choice.prompt_text_size, game_config.choice.prompt_text_color);

	// Options.
	y += height + button_spacing;
	choice_first_button_y = y;
	for (size_t i = 0; i < overlay_choice->option_count; i++) {
		if (!choice_option_available(overlay_choice, i)) {
			continue;
		}
		ChoiceOption *option = &overlay_choice->options[i];
		draw_button(option->name, 160.0f, y, overlay_choice_selected == i);
		y += button_spacing;
	}
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
	game_show_message(game_config.cannot_use_message, NULL);
}

void game_show_message(const char *message_id, void (*callback)(void)) {
	examine_image = (C2D_Image){0};
	free(message_text);
	message_text = text_render_wrap(lang_get(message_id), 276.0f, game_config.message.text_size);
	if (!message_text) {
		message_text = strdup(lang_get(message_id));
	}
	printf("Print: %s\n", message_id);
	game_mode = GAME_MESSAGE;
	game_resume_callback = callback;
}

void game_show_choice(Choice *choice) {
	size_t selected = choice ? choice_first_available(choice) : 0;
	if (overlay_choice == &start_choice) {
		pending_choice = choice;
		pending_choice_selected = selected;
	} else {
		overlay_choice = choice;
		overlay_choice_selected = selected;
	}
}

void game_show_image(C2D_Image image, void (*callback)(void)) {
	examine_image = image;
	game_mode = GAME_MESSAGE;
	game_resume_callback = callback;
}

// Index of the available option whose button contains (x, y), or -1. Uses
// the layout of the last frame drawn (choice_first_button_y).
static int select_option(int x, int y) {
	float box_x = (320.0f - game_config.choice.frame_width) / 2.0f;
	float spacing = FONT_HEIGHT * game_config.choice.button_text_size + 2.0f * game_config.choice.button_padding;

	if (x < box_x || x >= box_x + game_config.choice.frame_width) {
		return -1;
	}

	float button_y = choice_first_button_y;

	for (size_t i = 0; i < overlay_choice->option_count; i++) {
		if (!choice_option_available(overlay_choice, i)) {
			continue;
		}

		if (y >= button_y - spacing / 2.0f && y < button_y + spacing / 2.0f) {
			return (int)i;
		}

		button_y += spacing;
	}

	return -1;
}

bool game_update(u32 keys, circlePosition analog, touchPosition touch) {
	// An open overlay takes every key: nothing reaches the room, the inventory
	// or the current mode, except START which opens the START menu over a
	// room choice. A room choice keeps the countdown running.
	if (overlay_choice == &start_choice || (overlay_choice && game_mode == GAME_NORMAL)) {
		if (overlay_choice != &start_choice) {
			if (hud_update()) {
				return true;
			}
			// The state may have changed since the choice opened: once no
			// option is left, it closes as if EXIT_CHOICE had run.
			if (!choice_visible_count(overlay_choice)) {
				room_choice_exit();
				return true;
			}
			// An option, mini-game or timeline may have hidden the selected
			// option: move to the first one still available.
			if (!choice_option_available(overlay_choice, overlay_choice_selected)) {
				overlay_choice_selected = choice_first_available(overlay_choice);
			}
		}
		if (keys & KEY_UP) {
			for (size_t i = overlay_choice_selected; i > 0; i--) {
				if (choice_option_available(overlay_choice, i - 1)) {
					overlay_choice_selected = i - 1;
					break;
				}
			}
		}

		if (keys & KEY_DOWN) {
			for (size_t i = overlay_choice_selected + 1; i < overlay_choice->option_count; i++) {
				if (choice_option_available(overlay_choice, i)) {
					overlay_choice_selected = i;
					break;
				}
			}
		}
		if (keys & KEY_TOUCH) {
			int index = select_option(touch.px, touch.py);
			if (index < 0) {
				return true;
			}
			overlay_choice_selected = index;
			keys |= KEY_A;
		}

		if (keys & KEY_A) {
			if (overlay_choice == &start_choice) {
				overlay_choice = pending_choice;
				pending_choice = NULL;

				if (overlay_choice_selected == 2) {
					return false;
				}
				timer_resume();

				if (overlay_choice_selected == 1) {
					game_show_message(save_write() ? "SAVE_SAVING_SUCCESS" : "SAVE_SAVING_ERROR", NULL);
				}
				overlay_choice_selected = pending_choice_selected;
			} else {
				room_choice_select(overlay_choice_selected);
			}
			return true;
		}
		// B closes the START menu, or leaves one level of room choice.
		if (keys & KEY_B) {
			if (overlay_choice == &start_choice) {
				overlay_choice = pending_choice;
				overlay_choice_selected = pending_choice_selected;
				pending_choice = NULL;
				timer_resume();
			} else {
				room_choice_exit();
			}
			return true;
		}

		if (!(keys & KEY_START)) {
			return true;
		}

		if (overlay_choice == &start_choice) {
			overlay_choice = pending_choice;
			overlay_choice_selected = pending_choice_selected;
			pending_choice = NULL;
			timer_resume();
			return true;
		}
	}

	if (keys & KEY_START)  {
		pending_choice = overlay_choice;
		overlay_choice = &start_choice;
		pending_choice_selected = overlay_choice_selected;
		overlay_choice_selected = choice_first_available(overlay_choice);
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
			if (!keys) {
				return true;
			}
			// A hotspot's own message has no callback: touching again acts on
			// the room (the player has read it). An action's message continues
			// its block (room.c always passes continue_actions): the touch
			// only closes it, or it would run the action again.
			bool has_callbacks = (game_resume_callback != NULL);
			examine_image = (C2D_Image){0};
			game_resume();
			// The callback may have changed the mode (timeline, mini-game, new
			// message...): the keys only act in GAME_NORMAL.
			if (game_mode != GAME_NORMAL) {
				return true;
			}
			keys &= ~KEY_A;
			if (has_callbacks) {
				keys &= ~KEY_TOUCH;
			}
			break;

		case GAME_WAIT_SFX:
			if (!sfx_is_playing(wait_sfx_channel)) {
				wait_sfx_channel = -1;
				game_resume();
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
	// Same rule as game_update: the START menu in every mode, a room choice
	// only in GAME_NORMAL.
	if (overlay_choice == &start_choice || (overlay_choice && game_mode == GAME_NORMAL)) {
		C2D_SceneBegin(bottom);
		game_draw_choice();
	}
}
