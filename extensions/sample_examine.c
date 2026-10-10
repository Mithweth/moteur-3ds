#include <citro2d.h>

#include "sample_examine.h"
#include "lang.h"

static C2D_TextBuf text_buf;
static C2D_Text text;

void sample_examine_init(void) {
	text_buf = C2D_TextBufNew(1024);
}

void sample_examine_draw(void) {
	C2D_TextBufClear(text_buf);

	C2D_TextParse(&text, text_buf, lang_get("ITEM_GPU_SPECS"));
	C2D_TextOptimize(&text);

	C2D_DrawText(
		&text,
		C2D_WithColor,
		20.0f, 70.0f, 0.9f,
		0.45f, 0.45f,
		C2D_Color32(192, 192, 192, 255)
	);
}

void sample_examine_close(void) {
	C2D_TextBufDelete(text_buf);
}