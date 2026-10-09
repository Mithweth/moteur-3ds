// save.c
// Save slot: writes and restores the game progress. See save.h for the file
// format and the public API.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "save.h"
#include "game.h"
#include "hud.h"
#include "inventory.h"
#include "gamestate.h"
#include "callbacks.h"

static char save_file[256];
static char save_tmp[256];

void save_set_filename(const char *path) {
	save_file[0] = '\0';
	save_tmp[0] = '\0';
	snprintf(save_file, sizeof(save_file), "%s", path);
	snprintf(save_tmp, sizeof(save_tmp), "%s.tmp", path);
}

bool save_enabled(void) {
	return save_file[0] != '\0';
}

// True if path can be opened for reading.
static bool file_exists(const char *path) {
	if (!save_enabled()) {
		return false;
	}
	FILE *file = fopen(path, "r");
	if (!file) {
		return false;
	}
	fclose(file);
	return true;
}

bool save_exists(void) {
	// A power cut between remove and rename in save_write() leaves only the
	// complete temp file: finish the rename.
	if (!file_exists(save_file) && file_exists(save_tmp)) {
		rename(save_tmp, save_file);
	}
	return file_exists(save_file);
}

bool save_delete(void) {
	if (!save_exists()) {
		return true;
	}
	return remove(save_file) == 0;
}

bool save_write(void) {
	FILE *file = fopen(save_tmp, "w");
	if (!file) {
		printf("Cannot open save file\n");
		return false;
	}
	fprintf(file, "TIME %llu\n", timer_get_elapsed_time());
	fprintf(file, "ROOM %s\n", game_get_room());

	for (size_t i = 0; i < inventory_get_count(); i++) {
		const Item *item = inventory_get_item(i);
		fprintf(file, "ITEM %s\n", item->id);
	}

	for (size_t i = 0; i < gamestate_get_count(); i++) {
		const GameState *state = gamestate_get_index(i);
		if (state->value) {
			fprintf(file, "STATE %s\n", state->name);
		}
	}
	for (size_t i = 0; i < callbacks_get_count(); i++) {
		const InventoryCallback *callback = callbacks_get_index(i);
		if (!callback->serialize) {
			continue;
		}
		char *data = callback->serialize();
		if (!data) {
			fclose(file);
			remove(save_tmp);
			return false;
		}
		fprintf(file, "CALLBACK %s %s\n", callback->name, data);
		free(data);
	}
	// fclose flushes the buffer: on the SD card, this is where a write
	// usually fails, so its result matters as much as ferror.
	if (ferror(file)) {
		fclose(file);
		remove(save_tmp);
		return false;
	}
	if (fclose(file) != 0) {
		remove(save_tmp);
		return false;
	}
	// The 3DS file system refuses to rename onto an existing file.
	remove(save_file);
	return rename(save_tmp, save_file) == 0;
}

bool save_read(void) {
	FILE *file = fopen(save_file, "r");
	if (!file) {
		return false;
	}
	char line[256];
	while (fgets(line, sizeof(line), file)) {
		char *command = strtok(line, " ");
		if (!command) {
			continue;
		}
		if (strcmp(command, "TIME") == 0) {
			char *data = strtok(NULL, "\n");
			if (!data) {
				fclose(file);
				return false;
			}
			timer_set_elapsed_time(strtoull(data, NULL, 10));
		} else if (strcmp(command, "ROOM") == 0) {
			char *name = strtok(NULL, "\n");
			if (!name) {
				fclose(file);
				return false;
			}
			if (!game_set_room(name)) {
				fclose(file);
				return false;
			}
		} else if (strcmp(command, "ITEM") == 0) {
			char *name = strtok(NULL, "\n");
			if (!name) {
				fclose(file);
				return false;
			}
			inventory_add(name);
		} else if (strcmp(command, "STATE") == 0) {
			// Only true flags are saved and the game was reset, so setting
			// the flag once makes it true, whether it is TOGGLE or KEEP.
			char *name = strtok(NULL, "\n");
			if (!name) {
				fclose(file);
				return false;
			}
			gamestate_set(name);
		} else if (strcmp(command, "CALLBACK") == 0) {
			char *name = strtok(NULL, " ");
			char *data = strtok(NULL, "\n");
			if (!name) {
				fclose(file);
				return false;
			}
			// An empty serialize() result is a valid state: hand "" to the
			// callback rather than rejecting the whole save.
			if (!data) {
				data = "";
			}
			// Data of an extension that no longer exists is ignored.
			for (size_t i = 0; i < callbacks_get_count(); i++) {
				const InventoryCallback *callback = callbacks_get_index(i);
				if (strcmp(callback->name, name) == 0 && callback->deserialize) {
					callback->deserialize(data);
					break;
				}
			}
		} else {
			fclose(file);
			return false;
		}
	}
	if (ferror(file)) {
		fclose(file);
		return false;
	}
	fclose(file);
	return true;
}
