// room.c
// Loads, parses, draws and executes ROOM hotspots and paths.
// Shared script execution lives in script.c; choices live in choice.c.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "room.h"
#include "choice.h"
#include "script.h"
#include "gfxmap.h"
#include "callbacks.h"
#include "str_utils.h"

// --------- MODULE STATE

static Room *room = NULL;

// --------- ROOM DATA ACCESS

bool room_is_loaded(void) {
	return room != NULL;
}

const char *room_get_path(void) {
	return room ? room->path : NULL;
}

C2D_Image room_get_image(const char *name) {
	return room ? gfxmap_get_image(&room->assets, name) : (C2D_Image){0};
}

// --------- ROOM LOADING

static Path *get_path(Room *room, const char *direction) {
	if (strcmp(direction, "NORTH") == 0) {
		return &room->north;
	}

	if (strcmp(direction, "SOUTH") == 0) {
		return &room->south;
	}

	if (strcmp(direction, "EAST") == 0) {
		return &room->east;
	}

	if (strcmp(direction, "WEST") == 0) {
		return &room->west;
	}

	if (strcmp(direction, "NORTHWEST") == 0) {
		return &room->northwest;
	}

	if (strcmp(direction, "SOUTHWEST") == 0) {
		return &room->southwest;
	}

	if (strcmp(direction, "NORTHEAST") == 0) {
		return &room->northeast;
	}

	if (strcmp(direction, "SOUTHEAST") == 0) {
		return &room->southeast;
	}

	return NULL;
}

// Parse the ROOM script directly into its owner; errors leave cleanup to
// room_close(), which also handles partially constructed room data.
static bool room_load(Room *room, const char *filename) {
	if (!room) {
		return false;
	}

	FILE *f = fopen(filename, "r");

	if (!f) {
		printf("Cannot open room: %s\n", filename);
		return false;
	}

	char line[512];

	// Currently open blocks. They are tracked independently, so the parser
	// does not check proper nesting: a missing END_* leaves a block open.
	RoomImage *image = NULL;
	Hotspot *hotspot = NULL;
	Path *path = NULL;
	RoomActionBlock *action_block = NULL;
	RoomUse *use = NULL;

	size_t line_number = 0;

	while (fgets(line, sizeof(line), f)) {
		line_number++;

		char *p = str_trim(line);

		if (!*p || *p == '#') {
			continue;
		}

		char *command = strtok(p, " ");

		if (!command) {
			continue;
		}

		if (strcmp(command, "IMAGE") == 0) {
			char *image_name = strtok(NULL, " ");
			char *x = strtok(NULL, " ");
			char *y = strtok(NULL, " ");
			char *z = strtok(NULL, " ");

			if (!image_name || !x || !y || !z) {
				printf("%s:%zu: invalid IMAGE\n", filename, line_number);
				fclose(f);
				return false;
			}

			if (room->image_count >= ROOM_MAX_IMAGES) {
				printf("%s:%zu: too many images\n", filename, line_number);
				fclose(f);
				return false;
			}

			image = &room->images[room->image_count++];
			memset(image, 0, sizeof(*image));

			// An unknown image name only prints a warning and leaves an empty
			// image (tex == NULL) in the room, so check the log when adding one.
			image->image = gfxmap_get_image(&room->assets, image_name);
			image->x = atof(x);
			image->y = atof(y);
			image->z = atof(z);

			continue;
		}

		if (strcmp(command, "END_IMAGE") == 0) {
			image = NULL;
			continue;
		}


		if (strcmp(command, "HOTSPOT") == 0) {
			char *id = strtok(NULL, " ");
			char *x = strtok(NULL, " ");
			char *y = strtok(NULL, " ");
			char *width = strtok(NULL, " ");
			char *height = strtok(NULL, " ");

			if (!id || !x || !y || !width || !height) {
				printf("%s:%zu: invalid HOTSPOT\n", filename, line_number);
				fclose(f);
				return false;
			}

			if (room->hotspot_count >= ROOM_MAX_HOTSPOTS) {
				printf("%s:%zu: too many hotspots\n", filename, line_number);
				fclose(f);
				return false;
			}

			hotspot = &room->hotspots[room->hotspot_count++];
			memset(hotspot, 0, sizeof(*hotspot));

			hotspot->id = strdup(id);
			hotspot->x = atoi(x);
			hotspot->y = atoi(y);
			hotspot->width = atoi(width);
			hotspot->height = atoi(height);
			continue;
		}

		if (strcmp(command, "END_HOTSPOT") == 0) {
			hotspot = NULL;
			action_block = NULL;
			use = NULL;
			continue;
		}

		if (strcmp(command, "PATH") == 0) {
			char *direction = strtok(NULL, " ");

			if (!direction) {
				printf("%s:%zu: missing PATH direction\n", filename, line_number);
				fclose(f);
				return false;
			}
			path = get_path(room, direction);
			if (!path) {
				printf("%s:%zu: invalid PATH direction: %s\n", filename, line_number, direction);
				fclose(f);
				return false;
			}
			path->exists = true;
			continue;
		}

		if (strcmp(command, "END_PATH") == 0) {
			path = NULL;
			continue;
		}

		// ACTION belongs to the open hotspot first, otherwise to the open path.
		if (strcmp(command, "ACTION") == 0) {
			if (hotspot) {
				if (hotspot->action_block_count >= ROOM_MAX_ACTION_BLOCKS) {
					printf("%s:%zu: too many ACTION blocks\n",filename, line_number);
					fclose(f);
					return false;
				}

				action_block = &hotspot->action_blocks[hotspot->action_block_count++];
			} else if (path) {
				if (path->action_block_count >= ROOM_MAX_ACTION_BLOCKS) {
					printf("%s:%zu: too many PATH ACTION blocks\n", filename, line_number);
					fclose(f);
					return false;
				}

				action_block = &path->action_blocks[path->action_block_count++];
			} else {
				printf("%s:%zu: ACTION outside HOTSPOT/PATH\n", filename, line_number);
				fclose(f);
				return false;
			}
			continue;
		}

		if (strcmp(command, "END_ACTION") == 0) {
			action_block = NULL;
			continue;
		}

		if (strcmp(command, "USE") == 0) {
			char *item = strtok(NULL, " ");

			if (!hotspot || !item) {
				printf("%s:%zu: invalid USE\n", filename, line_number);
				fclose(f);
				return false;
			}

			if (hotspot->use_count >= ROOM_MAX_USES) {
				printf("%s:%zu: too many USE blocks\n", filename, line_number);
				fclose(f);
				return false;
			}

			use = &hotspot->uses[hotspot->use_count++];
			memset(use, 0, sizeof(*use));

			use->item = strdup(item);

			continue;
		}

		if (strcmp(command, "END_USE") == 0) {
			use = NULL;
			continue;
		}

		if (strcmp(command, "WHEN") == 0) {
			char *type = strtok(NULL, " ");
			char *name = strtok(NULL, " ");
			char *value = strtok(NULL, " ");

			if (!type || !name || !value) {
				printf("%s:%zu: invalid WHEN\n", filename, line_number);
				fclose(f);
				return false;
			}

			RoomCondition condition = script_condition_add(type, name, value);

			if (!condition.name) {
				fclose(f);
				return false;
			}

			// Attach the condition to the innermost open block, in this order of
			// precedence: action block > use > image > hotspot > path.
			if (action_block) {
				if (action_block->condition_count >= ROOM_MAX_CONDITIONS) {
					printf("%s:%zu: too many ACTION conditions\n", filename, line_number);
					script_condition_remove(&condition);
					fclose(f);
					return false;
				}

				action_block->conditions[action_block->condition_count++] = condition;

			} else if (use) {
				if (use->condition_count >= ROOM_MAX_CONDITIONS) {
					printf("%s:%zu: too many USE conditions\n", filename, line_number);
					script_condition_remove(&condition);
					fclose(f);
					return false;
				}

				use->conditions[use->condition_count++] = condition;

			} else if (image) {
				if (image->condition_count >= ROOM_MAX_CONDITIONS) {
					printf("%s:%zu: too many IMAGE conditions\n", filename, line_number);
					script_condition_remove(&condition);
					fclose(f);
					return false;
				}

				image->conditions[image->condition_count++] = condition;

			} else if (hotspot) {
				if (hotspot->condition_count >= ROOM_MAX_CONDITIONS) {
					printf("%s:%zu: too many HOTSPOT conditions\n", filename, line_number);
					script_condition_remove(&condition);
					fclose(f);
					return false;
				}

				hotspot->conditions[hotspot->condition_count++] = condition;

			} else if (path) {
				if (path->condition_count >= ROOM_MAX_CONDITIONS) {
					printf("%s:%zu: too many PATH conditions\n", filename, line_number);
					script_condition_remove(&condition);
					fclose(f);
					return false;
				}

				path->conditions[path->condition_count++] = condition;

			} else {
				printf("%s:%zu: WHEN outside condition block\n", filename, line_number);
				script_condition_remove(&condition);
				fclose(f);
				return false;
			}
			continue;
		}

		// MESSAGE or MESSAGE_IMAGE directly inside a HOTSPOT (not in
		// ACTION/USE) is the hotspot's first-touch message (MESSAGE wins
		// when both are set); elsewhere it is a regular action.
		if (strcmp(command, "MESSAGE") == 0 && hotspot && !action_block && !use) {
			char *message = strtok(NULL, " ");

			if (!message) {
				printf("%s:%zu: invalid MESSAGE\n", filename, line_number);
				fclose(f);
				return false;
			}
			if (hotspot->message_id) {
				printf("%s:%zu: duplicate MESSAGE in HOTSPOT %s\n", filename, line_number, hotspot->id);
				fclose(f);
				return false;
			}
			hotspot->message_id = strdup(message);
			continue;
		}
		if (strcmp(command, "MESSAGE_IMAGE") == 0 && hotspot && !action_block && !use) {
			char *image = strtok(NULL, " ");

			if (!image) {
				printf("%s:%zu: invalid MESSAGE_IMAGE\n", filename, line_number);
				fclose(f);
				return false;
			}
			if (hotspot->message_image.tex) {
				printf("%s:%zu: duplicate MESSAGE_IMAGE in HOTSPOT %s\n", filename, line_number, hotspot->id);
				fclose(f);
				return false;
			}

			hotspot->message_image = gfxmap_get_image(&room->assets, image);
			if (!hotspot->message_image.tex) {
				printf("%s:%zu: unknown image %s\n", filename, line_number, image);
				fclose(f);
				return false;
			}
			continue;
		}

		// Inside ACTION or USE, any other directive is an action.
		if (action_block || use) {
			char *argument = strtok(NULL, " ");

			RoomAction action = script_action_add(command, argument);

			if (!action.argument && action.type != ROOM_ACTION_EXIT_CHOICE) {
				printf("%s:%zu: invalid action\n", filename, line_number);
				fclose(f);
				return false;
			}

			if (action.type == ROOM_ACTION_MESSAGE_IMAGE) {
				action.image = gfxmap_get_image(&room->assets, action.argument);
				if (!action.image.tex) {
					printf("%s:%zu: unknown image %s\n", filename, line_number, action.argument);
					script_action_remove(&action);
					fclose(f);
					return false;
				}
			}

			// Checked here rather than when the action runs, so a typo fails
			// the room load instead of making the hotspot silently do nothing.
			if (action.type == ROOM_ACTION_MINIGAME && !callbacks_minigame_find(action.argument)) {
				printf("%s:%zu: unknown mini-game %s\n", filename, line_number, action.argument);
				script_action_remove(&action);
				fclose(f);
				return false;
			}

			if (action_block) {
				if (action_block->action_count >= ROOM_MAX_ACTIONS) {
					printf("%s:%zu: too many ACTION actions\n", filename, line_number);
					script_action_remove(&action);
					fclose(f);
					return false;
				}

				action_block->actions[action_block->action_count++] = action;

			} else if (use) {
				if (use->action_count >= ROOM_MAX_ACTIONS) {
					printf("%s:%zu: too many USE actions\n", filename, line_number);
					script_action_remove(&action);
					fclose(f);
					return false;
				}

				use->actions[use->action_count++] = action;
			}
			continue;
		}

		printf("%s:%zu: unexpected directive: %s\n", filename, line_number, command);
		fclose(f);
		return false;
	}

	printf("Loaded room: %zu images, %zu hotspots\n", room->image_count, room->hotspot_count);
	fclose(f);
	return true;
}

// --------- PATHS AND MOVEMENT

static bool path_is_available(Path *path) {
	if (!path->exists) {
		return false;
	}
	return script_match_conditions(path->conditions, path->condition_count);
}

static void path_execute(Path *path) {
	if (path_is_available(path)) {
		script_execute_action_blocks(path->action_blocks,path->action_block_count);
	}
}

void room_move_north(void) {
	if (!room) {
		return;
	}
	path_execute(&room->north);
}

void room_move_northeast(void) {
	if (!room) {
		return;
	}
	path_execute(&room->northeast);
}

void room_move_east(void) {
	if (!room) {
		return;
	}
	path_execute(&room->east);
}

void room_move_southeast(void) {
	if (!room) {
		return;
	}
	path_execute(&room->southeast);
}

void room_move_south(void) {
	if (!room) {
		return;
	}
	path_execute(&room->south);
}

void room_move_southwest(void) {
	if (!room) {
		return;
	}
	path_execute(&room->southwest);
}

void room_move_west(void) {
	if (!room) {
		return;
	}
	path_execute(&room->west);
}

void room_move_northwest(void) {
	if (!room) {
		return;
	}
	path_execute(&room->northwest);
}

bool room_can_move_north(void) {
	return room && path_is_available(&room->north);
}

bool room_can_move_south(void) {
	return room && path_is_available(&room->south);
}

bool room_can_move_east(void) {
	return room && path_is_available(&room->east);
}

bool room_can_move_west(void) {
	return room && path_is_available(&room->west);
}

bool room_can_move_northwest(void) {
	return room && path_is_available(&room->northwest);
}

bool room_can_move_southwest(void) {
	return room && path_is_available(&room->southwest);
}

bool room_can_move_northeast(void) {
	return room && path_is_available(&room->northeast);
}

bool room_can_move_southeast(void) {
	return room && path_is_available(&room->southeast);
}

// --------- HOTSPOTS

bool room_execute_hotspot_use(Hotspot *hotspot, const char *id) {
	for (size_t i = 0; i < hotspot->use_count; i++) {
		RoomUse *use = &hotspot->uses[i];
		if (strcmp(use->item, "*") != 0 && strcmp(use->item, id) != 0) {
			continue;
		}
		if (!script_match_conditions(use->conditions, use->condition_count)) {
			continue;
		}
		// USE blocks are alternatives: only the first match runs.
		script_execute_actions(use->actions, use->action_count, 0);
		return true;
	}

	return false;
}

void room_execute_hotspot_action(Hotspot *hotspot) {
	script_execute_action_blocks(hotspot->action_blocks,hotspot->action_block_count);
}

bool room_hotspot_is_available(Hotspot *hotspot) {
	if (!hotspot) {
		return false;
	}
	return script_match_conditions(hotspot->conditions, hotspot->condition_count);
}

Hotspot *room_find_hotspot(int x, int y) {
	if (!room) {
		return NULL;
	}
	for (size_t i = 0; i < room->hotspot_count; i++) {
		Hotspot *hotspot = &room->hotspots[i];

		if (!room_hotspot_is_available(hotspot)) {
			continue;
		}

		if (x >= hotspot->x && x < hotspot->x + hotspot->width && y >= hotspot->y && y < hotspot->y + hotspot->height) {
			return hotspot;
		}
	}
	return NULL;
}

Hotspot *room_find_hotspot_by_id(const char *id) {
	if (!room) {
		return NULL;
	}
	for (size_t i = 0; i < room->hotspot_count; i++) {
		Hotspot *hotspot = &room->hotspots[i];

		if (strcmp(hotspot->id, id) == 0 && room_hotspot_is_available(hotspot)) {
			return hotspot;
		}
	}

	return NULL;
}

// --------- GRAPHIC FUNCTIONS

void room_draw(void) {
	if (!room) {
		return;
	}
	for (size_t i = 0; i < room->image_count; i++) {
		RoomImage *image = &room->images[i];
		if (script_match_conditions(image->conditions, image->condition_count)) {
			C2D_DrawImageAt(image->image, image->x, image->y, image->z, NULL, 1.0f, 1.0f);
		}
	}
}

// --------- INITIALIZATION AND CLEANUP

bool room_init(const char *name) {
	char path[256];
	if (room) {
		return false;
	}

	room = calloc(1, sizeof(Room));
	if (!room) {
		return false;
	}

	snprintf(path, sizeof(path), "romfs:/rooms/%s", name);
	room->path = strdup(path);
	if (!gfxmap_load_assets(path, &room->assets)) {
		printf("Cannot load room assets\n");
		room_close();
		return false;
	}
	snprintf(path, sizeof(path), "romfs:/rooms/%s/room", name);
	if (!room_load(room, path)) {
		printf("Cannot load rooms: %s\n", room->path);
		room_close();
		return false;
	}
	printf("entering Room: %s\n", name);
	return true;
}

static void room_clear_data(Room *room) {
	for (size_t i = 0; i < room->image_count; i++) {
		RoomImage *image = &room->images[i];
		for (size_t j = 0; j < image->condition_count; j++) {
			script_condition_remove(&image->conditions[j]);
		}
	}

	for (size_t i = 0; i < room->hotspot_count; i++) {
		Hotspot *hotspot = &room->hotspots[i];

		free(hotspot->id);
		free(hotspot->message_id);

		for (size_t j = 0; j < hotspot->condition_count; j++) {
			script_condition_remove(&hotspot->conditions[j]);
		}

		for (size_t j = 0; j < hotspot->action_block_count; j++) {
			script_action_block_remove(&hotspot->action_blocks[j]);
		}

		for (size_t j = 0; j < hotspot->use_count; j++) {
			RoomUse *use = &hotspot->uses[j];
			free(use->item);
			for (size_t k = 0; k < use->condition_count; k++) {
				script_condition_remove(&use->conditions[k]);
			}
			for (size_t k = 0; k < use->action_count; k++) {
				script_action_remove(&use->actions[k]);
			}
		}
	}

	Path *paths[] = {
		&room->north,
		&room->south,
		&room->east,
		&room->west,
		&room->northeast,
		&room->southeast,
		&room->northwest,
		&room->southwest,
	};

	for (size_t i = 0; i < 8; i++) {
		Path *path = paths[i];

		for (size_t j = 0; j < path->condition_count; j++) {
			script_condition_remove(&path->conditions[j]);
		}

		for (size_t j = 0; j < path->action_block_count; j++) {
			script_action_block_remove(&path->action_blocks[j]);
		}
	}
}

void room_close(void) {
	if (!room) {
		return;
	}

	// A suspended block points into this room's memory or its choice file;
	// an open choice is dropped with its file.
	script_reset();
	choice_clear();
	room_clear_data(room);
	free(room->path);
	gfxmap_free_assets(&room->assets);
	free(room);
	room = NULL;
}
