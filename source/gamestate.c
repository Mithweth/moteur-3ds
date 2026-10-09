// gamestate.c
// Fixed-size table of named boolean flags loaded from game.state.
// Lookups are linear strcmp scans: the table is small (GAMESTATE_MAX) and
// conditions are re-evaluated every frame by room_draw() and the HUD.

#include "gamestate.h"
#include "str_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GAMESTATE_MAX 128

static GameState states[GAMESTATE_MAX];
static size_t state_count = 0;


static GameState *gamestate_find(const char *name) {
	for (size_t i = 0; i < state_count; i++) {
		if (strcmp(states[i].name, name) == 0) {
			return &states[i];
		}
	}

	return NULL;
}

size_t gamestate_get_count(void) {
	return state_count;
}

const GameState *gamestate_get_index(size_t index) {
	if (index >= state_count) {
		return NULL;
	}
	return &states[index];
}

bool gamestate_init(const char *filename) {
	FILE *file = fopen(filename, "r");

	if (!file) {
		printf("Cannot open %s\n", filename);
		return false;
	}

	char line[256];
	size_t line_number = 0;

	while (fgets(line, sizeof(line), file)) {
		line_number++;
		char *p = str_trim(line);

		if (*p == '\0' || *p == '#')
			continue;

		char *name = strtok(p, " ");
		char *type = strtok(NULL, " ");

		if (!name || !type) {
			printf("romfs:/game.state:%zu: syntax error\n", line_number);
			fclose(file);
			return false;
		}

		name = str_trim(name);
		type = str_trim(type);

		if (state_count >= GAMESTATE_MAX) {
			printf("romfs:/game.state:%zu: too many gamestates\n", line_number);
			fclose(file);
			return false;
		}

		GameStateType state_type;

		if (strcmp(type, "TOGGLE") == 0) {
			state_type = GAMESTATE_TOGGLE;
		} else if (strcmp(type, "KEEP") == 0) {
			state_type = GAMESTATE_KEEP;
		} else {
			printf("romfs:/game.state:%zu: unknown type: %s\n", line_number, type);
			fclose(file);
			return false;
		}

		if (gamestate_find(name)) {
			printf("romfs:/game.state:%zu: duplicate gamestate: %s\n", line_number, name);
			fclose(file);
			return false;
		}

		// All flags start false; gamestate_reset() restores that state.
		GameState *state = &states[state_count++];

		state->name = strdup(name);
		state->type = state_type;
		state->value = false;
	}

	fclose(file);

	printf("Loaded %zu gamestates\n", state_count);

	return true;
}


bool gamestate_get(const char *name) {
	GameState *state = gamestate_find(name);

	if (!state) {
		printf("Unknown gamestate: %s\n", name);
		return false;
	}

	return state->value;
}


void gamestate_set(const char *name) {
	GameState *state = gamestate_find(name);

	if (!state) {
		printf("Unknown gamestate: %s\n", name);
		return;
	}

	// KEEP flags are one-way: once true they never go back to false.
	if (state->type == GAMESTATE_KEEP && state->value) {
		return;
	}
	// TOGGLE flags invert on every set, so scripts calling SET twice on the
	// same flag end up where they started.
	state->value = !state->value;
	printf("State: %s is %s\n", name, state->value ? "true" : "false");
}


void gamestate_close(void) {
	for (size_t i = 0; i < state_count; i++) {
		free(states[i].name);
		states[i].name = NULL;
	}

	state_count = 0;
}

void gamestate_reset(void) {
	for (size_t i = 0; i < state_count; i++) {
		states[i].value = false;
	}
}
