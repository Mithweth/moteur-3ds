// text_render.c
// Implementation notes: text_render_wrap is a greedy word wrap. Each chunk
// (the spaces before a word plus the word) is appended to the current line,
// and the line is measured with C2D_TextGetDimensions in a private text buffer,
// so callers' buffers are untouched. When a chunk does not fit, its first
// space becomes the line break; no character is ever added or dropped.
// timeline.c relies on this to index the wrapped text with the offsets of the
// raw text. Words are measured as a whole line each time, which is quadratic
// in the line length but lines are short and wrapping happens once per text.
#include <3ds.h>
#include <citro2d.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "text_render.h"

static float text_width(C2D_TextBuf buf, const char *str, float scale) {
	C2D_Text text;
	float width;

	C2D_TextBufClear(buf);
	C2D_TextParse(&text, buf, str);
	C2D_TextGetDimensions(&text, scale, scale, &width, NULL);

	return width;
}

char *text_render_wrap(const char *str, float max_width, float scale) {
	if (!str) {
		return NULL;
	}
	size_t len = strlen(str);
	char *result = malloc(len + 1);
	char *line = malloc(len + 1);
	char *candidate = malloc(len + 1);
	C2D_TextBuf measure_buf = C2D_TextBufNew(len + 1);

	if (!result || !line || !candidate || !measure_buf) {
		free(result);
		free(line);
		free(candidate);
		if (measure_buf) {
			C2D_TextBufDelete(measure_buf);
		}
		return NULL;
	}

	result[0] = '\0';
	line[0] = '\0';

	const char *p = str;

	while (*p) {
		if (*p == '\n') {
			strcat(result, line);
			strcat(result, "\n");
			line[0] = '\0';
			p++;
			continue;
		}

		const char *start = p;
		// The chunk keeps its leading spaces: nothing is dropped, so the result
		// has the same length and offsets as str (timeline.c relies on it).
		while (*p == ' ') {
			p++;
		}
		while (*p && *p != ' ' && *p != '\n') {
			p++;
		}

		size_t word_len = p - start;

		snprintf(candidate, len + 1, "%s%.*s", line, (int)word_len, start);

		if (line[0] && text_width(measure_buf, candidate, scale) > max_width) {
			strcat(result, line);
			strcat(result, "\n");
			// The chunk's first space becomes the newline.
			snprintf(line, len + 1, "%.*s", (int)word_len - 1, start + 1);
		} else {
			strcpy(line, candidate);
		}
	}

	strcat(result, line);

	C2D_TextBufDelete(measure_buf);
	free(line);
	free(candidate);

	return result;
}