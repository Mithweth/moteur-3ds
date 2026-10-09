// timeline.h
// Scripted cut-scenes (intro, game overs, ending): text typed character by
// character on the bottom screen, images and sprites on the top screen,
// music and sound effects. Script format: docs/TIMELINES.en.md.
#pragma once

// How the last timeline ended, as reported by timeline_exit().
typedef enum {
	TIMELINE_EXIT_NONE,
	TIMELINE_EXIT_END,
	TIMELINE_EXIT_RETURN
} TimelineExit;

// Loads <directory>/timeline, <directory>/gfx.t3x and <directory>/gfx.h and
// rewinds to the first event. Returns false if any of them fails to load.
// Relative MUSIC_START (.ogg) and SFX (.raw) names are looked up in
// <directory>; "romfs:/" paths are used as is.
// Called by game.c, which then switches to GAME_TIMELINE.
bool timeline_init(const char *directory);

// Frees the events, the spritesheet and the text buffer, and stops the music.
// Safe to call when no timeline is loaded (game_title_start always calls it).
void timeline_close();

// Advances the timeline by one frame. A completes the current TEXT or ends the
// current PAUSE; B jumps to the final END or RETURN. Returns false once that
// final event is reached: the caller must then read timeline_exit() and leave
// GAME_TIMELINE. Never changes the game mode itself.
bool timeline_update(u32 keys);

// Draw the current scene; must be called inside the matching C2D scene.
void timeline_draw_bottom(void);
void timeline_draw_top(void);

// TIMELINE_EXIT_END (back to the title screen) or TIMELINE_EXIT_RETURN (back
// to the game) once timeline_update has returned false; TIMELINE_EXIT_NONE
// while the timeline is still running.
TimelineExit timeline_exit(void);
