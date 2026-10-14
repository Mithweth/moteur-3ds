// game.c
// Central controller: game modes, room input, and delegation to the renderer.
// No UI layout or drawing primitives are owned by this module.
#include <3ds.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "room.h"
#include "choice.h"
#include "ui.h"
#include "config.h"
#include "inventory.h"
#include "gamestate.h"
#include "hud.h"
#include "audio.h"
#include "title.h"
#include "timeline.h"
#include "callbacks.h"
#include "save.h"

// --------- MODULE STATE

static GameMode game_mode = GAME_NORMAL;
static bool circle_ready = true;
static Hotspot *active_hotspot = NULL;
static Hotspot *target = NULL;
static void (*game_resume_callback)(void) = NULL;
static MiniGame *active_minigame = NULL;
static int wait_sfx_channel = -1;
static char current_room_name[64];
static GameMode timeline_return_mode;

// --------- STATE AND MODE HELPERS

static void refresh_target(void) {
	if (target && !room_hotspot_is_available(target)) {
		target = room_find_hotspot_by_id(target->id);
	}
}

static void game_resume(void) {
	void (*callback)(void) = game_resume_callback;
	game_resume_callback = NULL;
	game_mode = GAME_NORMAL;
	if (callback) {
		callback();
	}
}

static void game_close_modes(void) {
	if (active_minigame && active_minigame->close) {
		active_minigame->close();
	}
	active_minigame = NULL;
	room_close();
	active_hotspot = NULL;
	target = NULL;
	ui_message_clear_text();
	game_resume_callback = NULL;
	title_close();
	timeline_close();
	music_stop();
}

static void game_reset(void) {
	title_close();
	callbacks_reset();
	inventory_reset();
	gamestate_reset();
	hud_reset();
	game_mode = GAME_NORMAL;
	ui_message_clear_text();
	active_hotspot = NULL;
	// No room yet in this game: game_set_room must not fall back to the room
	// of the previous game, and save_write must not save it.
	current_room_name[0] = '\0';
	if (config_music()) {
		music_play(config_music());
	}
}

static void game_timeline_stop(void) {
	if (timeline_exit() != TIMELINE_EXIT_RETURN || timeline_return_mode == GAME_TITLE) {
		game_title_start();
		return;
	}
	timeline_close();
	if (config_music()) {
		music_play(config_music());
	}
	game_resume();
}

// --------- ROOM INPUT HELPERS

// One move per circle-pad push; the stick must return to the dead zone.
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

// First tap selects a hotspot, second tap activates it when it has a message.
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

// --------- DRAW DELEGATION

// Only chooses the rendering order; layout and drawing live elsewhere.
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
			room_draw();
			if (game_mode == GAME_MESSAGE) {
				ui_draw_message();
			}
			C2D_SceneBegin(top);
			hud_draw();
			break;
	}
	// Same rule as game_update: the START menu in every mode, a room choice
	// only in GAME_NORMAL.
	if (choice_is_active(game_mode == GAME_NORMAL)) {
		C2D_SceneBegin(bottom);
		choice_draw();
	}
}

// --------- ROOM AND INVENTORY API

GameMode game_get_mode(void) {
	return game_mode;
}

const char *game_get_room(void) {
	return current_room_name;
}

const char *game_target_name(void) {
	refresh_target();
	if (!target) {
		return NULL;
	}
	return target->id;
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
	ui_message_clear_text();
	ui_message_clear_image();
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
	if (!target || !config_cannot_use_message()) {
		return;
	}
	game_show_message(config_cannot_use_message(), NULL);
}

// --------- MESSAGES AND SOUND

// UI owns the text; game.c owns the mode and the resume callback.
void game_show_message(const char *message_id, void (*callback)(void)) {
	ui_show_message(message_id);
	printf("Print: %s\n", message_id);
	game_mode = GAME_MESSAGE;
	game_resume_callback = callback;
}

void game_show_image(C2D_Image image, void (*callback)(void)) {
	ui_show_image(image);
	game_mode = GAME_MESSAGE;
	game_resume_callback = callback;
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

// --------- MODE TRANSITIONS

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
	if (config_music()) {
		music_play(config_music());
	}
	active_minigame = NULL;
	game_mode = GAME_NORMAL;
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
	for (size_t i = 0; i < config_item_count(); i++) {
		inventory_add(config_item(i));
	}
	if (!game_set_room(config_room())) {
		// No previous room to fall back to: the game cannot start.
		game_title_start();
		return false;
	}
	return true;
}

// --------- UPDATE FUNCTIONS

bool game_update(u32 keys, circlePosition analog, touchPosition touch) {
	if (choice_is_active(game_mode == GAME_NORMAL)) {
		return choice_update(keys, touch);
	}
	if (keys & KEY_START) {
		choice_start_open();
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
			ui_message_clear_image();
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

// --------- INITIALIZATION AND CLEANUP

bool game_init(void) {
	callbacks_init();
	if (!config_load("romfs:/game/game")) {
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
	if (!ui_init()) {
		return false;
	}
	if (!choice_init()) {
		return false;
	}
	return game_title_start();
}

void game_close(void) {
	game_close_modes();
	choice_close();
	ui_close();
	config_close();
	hud_close();
	inventory_close();
	// Last, in reverse order of game_init, which starts with callbacks_init.
	callbacks_close();
}
