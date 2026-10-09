// title.h
// Title screen: menu (language, intro, new game, controls, credits) on the
// bottom screen, artwork on the top screen, both described by the
// romfs:/game/title configuration file. Driven by game.c while in GAME_TITLE.
#pragma once

// Loads the title graphics (romfs:/game sprite sheet), parses the
// romfs:/game/title configuration and creates the text buffer. Returns false
// if any of them fails. Called by game_title_start.
bool title_init(void);

// Handles menu navigation for this frame. Choosing "intro" or "new game" calls
// game_intro / game_start, which close the title screen.
void title_update(u32 keys, touchPosition touch);

// Draws the top screen artwork.
void title_draw_top(void);

// Draws the menu, or the controls / credits page when one is open.
void title_draw_bottom(void);

// Releases the title graphics, text buffer and parsed configuration, so the
// next title_init starts from scratch. Safe to call more than once.
void title_close(void);
