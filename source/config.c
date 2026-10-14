// config.c
// Owns and parses the game configuration. Other modules only see config_* getters.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <citro2d.h>

#include "config.h"
#include "audio.h"
#include "save.h"
#include "str_utils.h"

// --------- PRIVATE CONFIGURATION

#define GAME_CONFIG_MAX_ITEMS 16

typedef struct {
	u32 shadow;
	u32 outer_border;
	u32 outer_background;
	u32 inner_border;
	u32 inner_background;
} FrameStyle;

// CHOICE block: appearance of the START menu and scripted choices.
typedef struct {
	u32 button_color;
	u32 button_text_color;
	u32 prompt_text_color;
	float button_text_size;
	float prompt_text_size;
	float frame_width;
	float button_padding;
} ChoiceStyle;

// MESSAGE block: appearance of the message box.
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

// --------- CONFIGURATION ACCESSORS

const char * config_room(void) {
	return game_config.room;
}

const char * config_music(void) {
	return game_config.music;
}

const char * config_cannot_use_message(void) {
	return game_config.cannot_use_message;
}

size_t config_item_count(void) {
	return game_config.item_count;
}

u32 config_frame_shadow(void) {
	return game_config.frame.shadow;
}

u32 config_frame_outer_border(void) {
	return game_config.frame.outer_border;
}

u32 config_frame_outer_background(void) {
	return game_config.frame.outer_background;
}

u32 config_frame_inner_border(void) {
	return game_config.frame.inner_border;
}

u32 config_frame_inner_background(void) {
	return game_config.frame.inner_background;
}

u32 config_choice_button_color(void) {
	return game_config.choice.button_color;
}

u32 config_choice_button_text_color(void) {
	return game_config.choice.button_text_color;
}

u32 config_choice_prompt_text_color(void) {
	return game_config.choice.prompt_text_color;
}

float config_choice_button_text_size(void) {
	return game_config.choice.button_text_size;
}

float config_choice_prompt_text_size(void) {
	return game_config.choice.prompt_text_size;
}

float config_choice_frame_width(void) {
	return game_config.choice.frame_width;
}

float config_choice_button_padding(void) {
	return game_config.choice.button_padding;
}

u32 config_message_text_color(void) {
	return game_config.message.text_color;
}

float config_message_text_size(void) {
	return game_config.message.text_size;
}

const char *config_item(size_t index) {
	return game_config.items[index];
}

// --------- PARSING HELPERS

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

// --------- INITIALIZATION AND CLEANUP

bool config_load(const char *filename) {
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

void config_close(void) {
	free(game_config.room);
	free(game_config.music);
	free(game_config.cannot_use_message);
	for (size_t i = 0; i < game_config.item_count; i++) {
		free(game_config.items[i]);
	}
	memset(&game_config, 0, sizeof(game_config));
}
