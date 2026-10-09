// game.h
// Central game controller: owns the current GameMode and routes input and
// drawing to the title screen, timelines, mini-games or the room view.
// Also the API that rooms, inventory and extensions use to change mode
// (messages, room changes, timelines, mini-games, waiting for a sound).
#pragma once

#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>
#include <stddef.h>
#include "inventory.h"

// Not used anywhere at the moment.
typedef struct {
	const char *name;
	void (*callback)(void);
} GameCallbackEntry;

// A mini-game plugged into the game loop (see extensions/ and
// callbacks_minigame_find). Every callback is optional.
//   init:   called by game_minigame_start; returning false aborts the start.
//   update: called every frame with the keys pressed this frame.
//   draw:   called on the bottom screen, after the room has been drawn.
//   close:  called by game_minigame_stop; must release what init allocated.
//           Also called when init fails, so it must cope with a partial init
//           (test every resource before freeing it).
// A mini-game ends itself by calling game_minigame_stop() from update.
typedef struct {
	bool (*init)(void);
	void (*update)(u32 keys, touchPosition touch);
	void (*draw)(void);
	void (*close)(void);
} MiniGame;

typedef enum {
	GAME_NORMAL,    // exploring a room: movement, touch and inventory input
	GAME_MESSAGE,   // a message or examine image is shown; A, B or touch dismisses it
	GAME_BUSY,      // input blocked until a sound effect ends (see game_wait_for_sfx)
	GAME_MINIGAME,  // a MiniGame receives the input
	GAME_TITLE,     // title screen
	GAME_TIMELINE   // a scripted sequence (intro, game over, ending) is playing
} GameMode;

// Per-frame update, called by main. keys are the keys pressed this frame
// (hidKeysDown); dispatches on the current GameMode. START opens the start
// menu (Back, Save, Quit), which takes all the input until it is closed; Save
// is only offered in GAME_NORMAL, and START dismisses a pending message first.
// Returns false when the player chose Quit: main must then leave its loop.
bool game_update(u32 keys, circlePosition analog, touchPosition touch);

// Per-frame drawing on both screens, called by main between
// C3D_FrameBegin and C3D_FrameEnd.
void game_draw(C3D_RenderTarget *top, C3D_RenderTarget *bottom);

// Starts the timeline in romfs:/timelines/<name> and switches to
// GAME_TIMELINE. Returns true if it started; otherwise goes back to the title
// screen and returns false. When it ends, END goes back to the title screen
// and RETURN to GAME_NORMAL in the current room, then calls callback (may be
// NULL). callback is never called after END, after a RETURN from the title
// screen, or when the timeline fails to load.
bool game_timeline_start(const char *name, void (*callback)(void));

// Releases everything the game holds: the active mini-game, the room, the
// title screen, the timeline, the music, the message text buffer, the HUD,
// the inventory and the inventory callbacks (callbacks_close). Called once by
// main at exit, before audio_close, since closing a mini-game or a timeline
// may still use NDSP.
void game_close(void);

// Loads the inventory, the HUD and the extensions, then shows the title
// screen. Must be called exactly once, from main, at start-up; a second call
// is refused. Use game_title_start to come back to the title screen.
bool game_init(void);

// Goes (back) to the title screen from any state: closes the mini-game, the
// room and the timeline, and stops the music. May be called any number of
// times. Returns false if the title screen can't be loaded.
bool game_title_start(void);

// Leaves the title screen and continues the saved game: resets the game like
// game_start, then restores the save (see save_read). If the save cannot be
// read, starts a new game instead. Either way, shows a message telling the
// player whether loading succeeded, unless the new game could not start
// either (back on the title screen, see game_start).
void game_load(void);

// Leaves the title screen and starts a new game: resets the extensions (new
// secret code), inventory, game states and timer, starts the music, gives the
// starting ITEMs and enters the ROOM set in romfs:/game/game. Returns false,
// back on the title screen, if that room cannot be loaded.
bool game_start(void);

// Name of the current room, as last passed to game_set_room (empty before the
// first room). Used by save_write. Points to a static buffer overwritten by
// the next game_set_room.
const char *game_get_room(void);

// Leaves the current room and loads romfs:/rooms/<name>. Clears the target,
// any pending message and switches to GAME_NORMAL. name may point into the
// current room's data: it is copied before the room is freed.
// Returns false if the room cannot be loaded: the previous room, if any, is
// then loaded again (its target and pending message are lost) and
// game_get_room keeps naming it. With no previous room (start of a game), the
// screen is left empty and the caller must handle it.
bool game_set_room(const char *name);

// Id of the hotspot currently targeted (shown in the HUD), or NULL.
// Revalidates the target first, since an action may have hidden it.
const char *game_target_name(void);

// Uses inventory item id on the current target by running the target's
// matching USE block. Returns true if a USE block ran, false otherwise
// (including when there is no target).
bool game_use_item(const char *id);

// Shows the game's CANNOT_USE_MESSAGE, if one is configured and there is
// a target. Does nothing otherwise.
void game_cannot_use_item(void);

// Shows the translated message for message_id and switches to GAME_MESSAGE.
// A later mode-changing action in the same frame (room change, timeline,
// mini-game, WAIT_SFX) replaces the message before it is ever displayed.
void game_show_message(const char *message_id);

// Shows an image centered over the room and switches to GAME_MESSAGE.
// The image must stay valid until the message is dismissed.
void game_show_image(C2D_Image image);

// Starts the mini-game registered under name (see callbacks.c), stops the
// music and switches to GAME_MINIGAME. Does nothing if the name is unknown or
// its init fails (the music then stays stopped).
void game_minigame_start(const char *name);

// Closes the active mini-game, restarts the background music and switches
// back to GAME_NORMAL. Usually called by the mini-game itself.
void game_minigame_stop(void);

// Plays sfx (a full romfs path) and blocks input in GAME_BUSY until it ends,
// then calls callback (may be NULL). Returns false without changing mode if
// the sound could not be started; the caller must then continue on its own.
bool game_wait_for_sfx(const char *sfx, void (*callback)(void));
