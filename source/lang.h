// lang.h
// Localization: every romfs:/lang file is a language made of KEY=value lines.
// Languages are sorted by their ORDER key; only the current one is loaded.
#pragma once

// Lists the language files and loads the first one. Returns false if the
// directory can't be read or the first language fails to load.
bool lang_init(void);

// Frees the loaded translations.
void lang_close(void);

// Switches to the next language (wrapping around) and reloads it.
// Pointers previously returned by lang_get become invalid.
void lang_next(void);

// Returns the translation of key, or key itself if it isn't translated. The
// returned string is owned by the module and valid until the next lang_next
// or lang_close.
const char *lang_get(const char *key);
