// hud.h
// Top-screen HUD during gameplay: inventory panel, selected item and target
// names, available directions and the countdown timer. The layout is described
// by romfs:/hud/hud (see docs/HUD.en.md). Initialized once by game_init and
// closed by game_close.
#pragma once

#include <stdbool.h>

// Loads the HUD spritesheet (romfs:/hud) and the layout file, and gives the
// inventory its column count. Returns false on any load error, after freeing
// what was loaded. Call once, from game_init, after inventory_init.
bool hud_init(void);

// Restarts the countdown (new game).
void hud_reset(void);

// Updates the timer once per frame. When time runs out, starts the TIMER
// block's TIMELINE (once per game) and returns true: the game mode has then
// changed (GAME_TIMELINE, or GAME_TITLE if the timeline can't be loaded), so
// the caller must stop handling the current frame. Returns false otherwise,
// including when there is no TIMER block.
bool hud_update(void);

// Draws the whole HUD; call inside the top screen scene.
void hud_draw(void);

// Frees the HUD spritesheet, text buffer and layout strings. Safe if
// hud_init failed or was never called.
void hud_close(void);

// Countdown helpers. timer_resume must be called after the application was
// suspended (home menu, sleep) so the suspended time isn't counted.
// There is no timer_pause: the countdown only advances in hud_update, so
// skipping hud_update pauses it (e.g. while game.c shows the quit confirmation)
// and timer_resume must be called when hud_update runs again.
void timer_start(void);
void timer_update(void);
void timer_resume(void);
u64 timer_get_elapsed_time(void);
void timer_set_elapsed_time(u64 e);
