// text_render.h
// Text layout helpers for the citro2d system font.
#pragma once

// Returns a copy of str where some spaces are replaced with '\n' so that no
// line is wider than max_width pixels once drawn at the given scale. Existing
// '\n' are kept as forced breaks. Only spaces are ever replaced: the result
// has the same length and byte offsets as str. A word wider than max_width is
// not split and overflows. Returns NULL if str is NULL or an allocation fails;
// the caller frees the result.
char *text_render_wrap(const char *str, float max_width, float scale);
