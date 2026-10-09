// save.h
// Single save slot on the SD card (the path given by SAVE in romfs:/game/game). The file is a
// text file, one entry per line: TIME, ROOM, then one ITEM per inventory item,
// one STATE per flag set to true and one CALLBACK per extension that has a
// serialize function. Written atomically through a temporary file, so a
// failed or interrupted save never destroys the previous one.
#pragma once

#include <stdbool.h>

// True if a save can be loaded. If a power cut interrupted save_write right
// after the old save was removed, finishes the rename of the temporary file
// first, so the save is not lost.
bool save_exists(void);
// Removes the save (new game). Returns true if there was no save to remove.
bool save_delete(void);
// Writes the elapsed time, current room, inventory, game states and extension
// data. Returns false, keeping the previous save, if any step fails.
bool save_write(void);
// Restores what save_write wrote, on top of a game reset by the caller
// (game_load). Returns false on an unreadable file or an unknown entry; the
// game may then be partially restored, so the caller starts a new game.
bool save_read(void);

void save_set_filename(const char *path);
bool save_enabled(void);
