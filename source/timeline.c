// timeline.c

// Implementation notes: the whole script is parsed up front into events[];
// timeline_update then runs the current event, so each instant event (music,
// image, scene change...) takes one frame. Images and sprites are resolved at
// load time from the timeline's own spritesheet. Script format:
// docs/TIMELINES.en.md.
// The timeline never changes the game mode: timeline_update returns false on
// the final END or RETURN and game.c reads timeline_exit() to pick the title
// screen or the game. load_timeline only accepts a script that ends with one
// of them, so the last event is always a valid target for B.
#include <citro2d.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "audio.h"
#include "lang.h"
#include "game.h"
#include "gfxmap.h"
#include "str_utils.h"
#include "timeline.h"

#define TIMELINE_MAX_EVENTS   256
#define TIMELINE_MAX_SPRITES  16
#define TIMELINE_CHAR_DELAY   100

typedef enum {
	TIMELINE_TEXT,
	TIMELINE_PAUSE,
	TIMELINE_MUSIC_START,
	TIMELINE_MUSIC_STOP,
	TIMELINE_SFX,
	TIMELINE_IMAGE_LEFT,
	TIMELINE_IMAGE_CENTER,
	TIMELINE_IMAGE_RIGHT,
	TIMELINE_END_SCENE,
	TIMELINE_FULL_SCREEN,
	TIMELINE_END,
	TIMELINE_RETURN
} TimelineEventType;

typedef struct {
	C2D_Image image;
	float x;
	float y;
} TimelineSprite;

typedef struct {
	TimelineEventType type;
	char *text;
	u32 duration;
	u32 color;
	C2D_Image image;
	TimelineSprite sprites[TIMELINE_MAX_SPRITES];
	size_t sprite_count;
	char *sound;
} TimelineEvent;

// Directory of the running timeline (e.g. "romfs:/timelines/intro"), owned by
// timeline_init/timeline_close. Relative MUSIC_START and SFX names are
// resolved against it at load time.
static char *directory = NULL;

// Typewriter state. event_pos is the byte offset reached in the current TEXT
// string; text_position is the length of current_str, which accumulates every
// TEXT of the scene until END_SCENE. previous_str is current_str minus the last
// character typed: it is drawn in the event color over current_str (drawn in
// light grey), so the newest character stands out.
static size_t event_pos = 0;
static size_t text_position = 0;
static char current_str[2048];
static char previous_str[2048];
static C2D_Text text_current;
static C2D_Text text_previous;
static C2D_TextBuf text_buf;
static u32 current_color;

static size_t current_event;
static u64 next_char_time;
// 0 when no PAUSE is running.
static u64 pause_start;
// Set by FULL_SCREEN until END_SCENE: its sprites replace the text and the
// left/center/right images.
static const TimelineEvent *active_full_screen = NULL;
static C2D_Image image_left;
static C2D_Image image_center;
static C2D_Image image_right;
static GfxAssets assets;
static TimelineEvent events[TIMELINE_MAX_EVENTS];
static size_t event_count = 0;
// Set by the final END or RETURN, reset by timeline_init.
static TimelineExit exit_status;

// Appends a zeroed event; returns NULL when TIMELINE_MAX_EVENTS is reached.
static TimelineEvent *add_event(TimelineEventType type) {
	if (event_count >= TIMELINE_MAX_EVENTS) {
		return NULL;
	}
	TimelineEvent *event = &events[event_count++];

	memset(event, 0, sizeof(*event));
	event->type = type;

	return event;
}

// Parses the script into events[]. The script must end with END: a missing
// END, an unknown command or a full event table makes the load fail. On
// failure the events parsed so far stay in events[]; timeline_init frees them
// with timeline_close.
static bool load_timeline(const char *filename) {
	FILE *f = fopen(filename, "r");

	if (!f) {
		printf("Cannot open timeline: %s\n", filename);
		return false;
	}

	event_count = 0;

	char line[512];
	size_t line_number = 0;

	TimelineEvent *full_screen = NULL;

	while (fgets(line, sizeof(line), f)) {
		line_number++;
		char *p = str_trim(line);

		if (*p == '\0' || *p == '#') {
			continue;
		}

		char *command = strtok(p, " ");

		if (!command) {
			continue;
		}

		// Inside FULL_SCREEN ... END_FULL_SCREEN, only SPRITE lines are allowed.
		if (full_screen) {
			if (strcmp(command, "SPRITE") == 0) {
				char *image_name = strtok(NULL, " ");
				char *x_str      = strtok(NULL, " ");
				char *y_str      = strtok(NULL, " ");

				if (!image_name || !x_str || !y_str) {
					printf( "%s:%zu: invalid SPRITE\n", filename, line_number);
					fclose(f);
					return false;
				}

				if (full_screen->sprite_count >= TIMELINE_MAX_SPRITES) {

					printf("%s:%zu: too many sprites\n", filename, line_number);
					fclose(f);
					return false;
				}

				TimelineSprite *sprite = &full_screen->sprites[full_screen->sprite_count];

				sprite->image = gfxmap_get_image(&assets, image_name);
				if (!sprite->image.tex) {
					printf("%s:%zu: unknown image %s\n", filename, line_number, image_name);
					fclose(f);
					return false;
				}

				sprite->x = atoi(x_str);
				sprite->y = atoi(y_str);
				full_screen->sprite_count++;
				continue;
			}

			if (strcmp(command, "END_FULL_SCREEN") == 0) {
				full_screen = NULL;
				continue;
			}

			fclose(f);
			return false;
		}

		if (strcmp(command, "TEXT") == 0) {
			char *color = strtok(NULL, " ");
			char *text  = strtok(NULL, " ");
			if (!color || !text) {
				printf("%s:%zu: syntax error: %s\n", filename, line_number, command);
				fclose(f);
				return false;
			}

			TimelineEvent *event = add_event(TIMELINE_TEXT);

			if (!event) {
				printf("%s:%zu: too many timeline events\n", filename, line_number);
				fclose(f);
				return false;
			}
			event->color = gfxmap_parse_color(color);
			event->text = strdup(text);
			continue;
		}

		if (strcmp(command, "PAUSE") == 0) {
			char *duration = strtok(NULL, " ");
			if (!duration) {
				printf("%s:%zu: syntax error: %s\n", filename, line_number, command);
				fclose(f);
				return false;
			}

			TimelineEvent *event = add_event(TIMELINE_PAUSE);

			if (!event) {
				printf("%s:%zu: too many timeline events\n", filename, line_number);
				fclose(f);
				return false;
			}
			event->duration = (u32)atoi(duration);
			continue;
		}

		if (strcmp(command, "MUSIC_START") == 0) {
			char *sound = strtok(NULL, " ");
			if (!sound) {
				printf("%s:%zu: syntax error: %s\n", filename, line_number, command);
				fclose(f);
				return false;
			}

			TimelineEvent *event = add_event(TIMELINE_MUSIC_START);

			if (!event) {
				printf("%s:%zu: too many timeline events\n", filename, line_number);
				fclose(f);
				return false;
			}
			event->sound = audio_resolve_path(directory, sound, ".ogg");
			continue;
		}

		if (strcmp(command, "MUSIC_STOP") == 0) {
			TimelineEvent *event = add_event(TIMELINE_MUSIC_STOP);
			if (!event) {
				printf("%s:%zu: too many timeline events\n", filename, line_number);
				fclose(f);
				return false;
			}
			continue;
		}

		if (strcmp(command, "SFX") == 0) {
			char *sound = strtok(NULL, " ");
			if (!sound) {
				printf("%s:%zu: syntax error: %s\n", filename, line_number, command);
				fclose(f);
				return false;
			}

			TimelineEvent *event = add_event(TIMELINE_SFX);

			if (!event) {
				printf("%s:%zu: too many timeline events\n", filename, line_number);
				fclose(f);
				return false;
			}

			event->sound = audio_resolve_path(directory, sound, ".raw");
			continue;
		}

		if ((strcmp(command, "IMAGE_LEFT") == 0) || (strcmp(command, "IMAGE_CENTER") == 0) || (strcmp(command, "IMAGE_RIGHT") == 0)) {
			TimelineEventType image_type;
			if (strcmp(command, "IMAGE_LEFT") == 0) {
				image_type = TIMELINE_IMAGE_LEFT;
			} else if (strcmp(command, "IMAGE_CENTER") == 0) {
				image_type = TIMELINE_IMAGE_CENTER;
			} else if (strcmp(command, "IMAGE_RIGHT") == 0) {
				image_type = TIMELINE_IMAGE_RIGHT;
			}
			char *image_name = strtok(NULL, " ");
			if (!image_name) {
				printf("%s:%zu: syntax error: %s\n", filename, line_number, command);
				fclose(f);
				return false;
			}

			TimelineEvent *event = add_event(image_type);

			if (!event) {
				printf("%s:%zu: too many timeline events\n", filename, line_number);
				fclose(f);
				return false;
			}

			if (strcmp(image_name, "NONE") == 0) {
				event->image = (C2D_Image){0};
			} else {
				event->image = gfxmap_get_image(&assets, image_name);
				if (!event->image.tex) {
					printf("%s:%zu: unknown image %s\n", filename, line_number, image_name);
					fclose(f);
					return false;
				}
			}

			continue;
		}

		if (strcmp(command, "END_SCENE") == 0) {
			TimelineEvent *event = add_event(TIMELINE_END_SCENE);
			if (!event) {
				printf("%s:%zu: too many timeline events\n", filename, line_number);
				fclose(f);
				return false;
			}
			continue;
		}

		if (strcmp(command, "FULL_SCREEN") == 0) {
			full_screen = add_event(TIMELINE_FULL_SCREEN);
			if (!full_screen) {
				printf("%s:%zu: too many timeline events\n", filename, line_number);
				fclose(f);
				return false;
			}

			continue;
		}
		if (strcmp(command, "END") == 0) {
			TimelineEvent *event = add_event(TIMELINE_END);
			if (!event) {
				printf("%s:%zu: too many timeline events\n", filename, line_number);
				fclose(f);
				return false;
			}
			fclose(f);
			return true;
		}
		if (strcmp(command, "RETURN") == 0) {
			TimelineEvent *event = add_event(TIMELINE_RETURN);
			if (!event) {
				printf("%s:%zu: too many timeline events\n", filename, line_number);
				fclose(f);
				return false;
			}
			fclose(f);
			return true;
		}

		printf("%s:%zu: unknown command: %s\n", filename, line_number, command);
		fclose(f);
		return false;
	}
	fclose(f);
	return false;
}

// Byte length of the UTF-8 character starting at s, so the typewriter never
// splits a multi-byte character.
static size_t utf8_char_size(const char *s) {
	unsigned char c = *s;

	if ((c & 0x80) == 0x00) return 1;
	if ((c & 0xE0) == 0xC0) return 2;
	if ((c & 0xF0) == 0xE0) return 3;
	if ((c & 0xF8) == 0xF0) return 4;

	return 1;
}

// Re-parses both strings; called whenever they change, not every frame.
static void update_text(void) {
	C2D_TextBufClear(text_buf);
	C2D_TextParse(&text_current, text_buf, current_str);
	C2D_TextParse(&text_previous, text_buf, previous_str);
	C2D_TextOptimize(&text_current);
	C2D_TextOptimize(&text_previous);
}

TimelineExit timeline_exit(void) {
	return exit_status;
}

bool timeline_update(u32 keys) {
	if (keys & KEY_B) {
		current_event = event_count - 1;
	}
	const TimelineEvent *event = &events[current_event];

	if ((keys & KEY_A) && (event->type == TIMELINE_TEXT || event->type == TIMELINE_PAUSE)) {
		if (event->type == TIMELINE_TEXT) {
			const char *str = lang_get(event->text);
			const char *remaining = &str[event_pos];
			size_t len = strlen(remaining);
			if (text_position + len >= sizeof(current_str)) {
				printf("Timeline text buffer overflow\n");
			} else {
				memcpy(&current_str[text_position], remaining, len + 1);
				text_position += len;
			}
			memcpy(previous_str, current_str, text_position + 1);
			event_pos = 0;
			update_text();
		}
		pause_start = 0;
		current_event++;

		if (current_event >= event_count) {
			current_event = event_count - 1;
		}
		return true;
	}

	u64 now = osGetTime();

	switch (event->type) {
	case TIMELINE_MUSIC_START:
		if (event->sound) {
			music_play(event->sound);
		}
		current_event++;
		break;
	case TIMELINE_MUSIC_STOP:
		music_stop();
		current_event++;
		break;
	case TIMELINE_SFX:
		if (event->sound) {
			sfx_play(event->sound);
		}
		current_event++;
		break;
	case TIMELINE_TEXT:
		if (now >= next_char_time) {
			const char *str = lang_get(event->text);
			current_color = event->color;

			if (str[event_pos] != '\0') {
				size_t len = utf8_char_size(&str[event_pos]);
				if (text_position + len >= sizeof(current_str)) {
					printf("Timeline text buffer overflow\n");
					event_pos = 0;
					current_event++;
					break;
				}
				memcpy(previous_str, current_str, text_position);
				previous_str[text_position] = '\0';
				memcpy(&current_str[text_position], &str[event_pos], len);
				text_position += len;
				current_str[text_position] = '\0';
				event_pos += len;
				next_char_time = now + TIMELINE_CHAR_DELAY;
				update_text();
			} else {
				memcpy(previous_str, current_str, text_position + 1);
				event_pos = 0;
				current_event++;
			}
		}
		break;

	case TIMELINE_PAUSE:
		// duration is in milliseconds; PAUSE 0 waits until the player presses A.
		if (event->duration == 0) {
			break;
		}
		if (pause_start == 0) {
			pause_start = now;
		}
		if (now - pause_start >= event->duration) {
			pause_start = 0;
			current_event++;
		}
		break;

	case TIMELINE_IMAGE_LEFT:
		image_left = (event->image.tex) ? event->image : (C2D_Image){0};
		current_event++;
		break;

	case TIMELINE_IMAGE_CENTER:
		image_center = (event->image.tex) ? event->image : (C2D_Image){0};
		current_event++;
		break;

	case TIMELINE_IMAGE_RIGHT:
		image_right = (event->image.tex) ? event->image : (C2D_Image){0};
		current_event++;
		break;

	case TIMELINE_END_SCENE:
		image_left = (C2D_Image){0};
		image_center = (C2D_Image){0};
		image_right = (C2D_Image){0};
		active_full_screen = NULL;
		current_str[0] = '\0';
		previous_str[0] = '\0';
		text_position = 0;
		event_pos = 0;
		update_text();
		current_event++;
		break;

	case TIMELINE_FULL_SCREEN:
		active_full_screen = event;
		current_event++;
		break;

	case TIMELINE_END:
		exit_status = TIMELINE_EXIT_END;
		return false;

	case TIMELINE_RETURN:
		exit_status = TIMELINE_EXIT_RETURN;
		return false;
	}
	return true;
}

void timeline_draw_bottom(void) {
   if (active_full_screen) {
		for (int i = 0; i < active_full_screen->sprite_count; i++) {
			float z = i * 0.01f;
			const TimelineSprite *sprite = &active_full_screen->sprites[i];
			if ((sprite->image.tex) && (sprite->y >= 240.0f)) {
				C2D_DrawImageAt(sprite->image, sprite->x, sprite->y - 240, z, NULL, 1.0f, 1.0f);
			}
		}
		return;
	}
	C2D_DrawText(&text_current, C2D_WithColor, 7.0f, 70.0f, 0.0f, 0.6f, 0.65f, C2D_Color32(192, 192, 192, 255));
	C2D_DrawText(&text_previous, C2D_WithColor, 7.0f, 70.0f, 0.01f, 0.6f, 0.65f, current_color);
}

void timeline_draw_top(void) {
	if (active_full_screen) {
		for (int i = 0; i < active_full_screen->sprite_count; i++) {
			float z = i * 0.01f;
			const TimelineSprite *sprite = &active_full_screen->sprites[i];
			if ((sprite->image.tex) && (sprite->y < 240.0f)) {
				// Sprite coordinates treat both screens as one 320x480 area: y < 240 is the
				// top screen, 400 px wide, hence the 40 px offset to center it.
				C2D_DrawImageAt(sprite->image, sprite->x + 40.0f, sprite->y, z, NULL, 1.0f, 1.0f);
			}
		}
		return;
	}
	if (image_left.tex) {
		C2D_DrawImageAt(image_left, 0.0f, 0.0f, 0.0f, NULL, 1.0f, 1.0f);
	}
	if (image_center.tex) {
		C2D_DrawImageAt(image_center, 133.0f, 0.0f, 0.0f, NULL, 1.0f, 1.0f);
	}
	if (image_right.tex) {
		C2D_DrawImageAt(image_right, 266.0f, 0.0f, 0.0f, NULL, 1.0f, 1.0f);
	}
}

void timeline_close(void) {
	for (size_t i = 0; i < event_count; i++) {
		free(events[i].text);
		free(events[i].sound);
	}
	free(directory);
	directory = NULL;
	event_count = 0;
	gfxmap_free_assets(&assets);
	if (text_buf) {
		C2D_TextBufDelete(text_buf);
		text_buf = NULL;
	}
	music_stop();
}

bool timeline_init(const char *d) {
	char script_path[256];

	timeline_close();

	directory = strdup(d);
	if (!directory) {
		return false;
	}
	snprintf(script_path, sizeof(script_path), "%s/timeline", directory);
	printf("starting timeline: %s\n", script_path);

	if (!text_buf) {
		text_buf = C2D_TextBufNew(4096);
	}
	if (!gfxmap_load_assets(directory, &assets)) {
		printf("Cannot load timeline assets: %s\n", script_path);
		timeline_close();
		return false;
	}

	if (!load_timeline(script_path)) {
		printf("Cannot load timeline: %s\n", script_path);
		timeline_close();
		return false;
	}

	active_full_screen = NULL;
	current_event = 0;
	event_pos = 0;
	text_position = 0;
	exit_status = TIMELINE_EXIT_NONE;
	current_str[0] = '\0';
	previous_str[0] = '\0';
	image_left = (C2D_Image){0};
	image_center = (C2D_Image){0};
	image_right = (C2D_Image){0};
	pause_start = 0;
	current_color = C2D_Color32(164, 164, 164, 255);
	next_char_time = osGetTime();
	update_text();
	return true;
}
