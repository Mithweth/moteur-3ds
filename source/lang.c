// lang.c
// Implementation notes: lang_init reads each file only up to its ORDER key to
// sort the languages; lang_load then parses the current one fully. Lookups are
// a linear search over the loaded keys. Values may contain \n escapes, which
// are turned into new lines.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "lang.h"

#define MAX_LANGUAGES 16
#define MAX_TRANSLATIONS 512
#define MAX_LINE_LENGTH  1024


typedef struct {
    char *key;
    char *value;
} Translation;

typedef struct {
    char filename[512];
    int order;
} Language;

static Language languages[MAX_LANGUAGES];
static size_t language_count;
static size_t current_language;
static Translation translations[MAX_TRANSLATIONS];
static size_t translation_count = 0;
static const char *LANG_DIR = "romfs:/lang";

// qsort comparator: ascending ORDER (a file without ORDER sorts as 0).
static int compare_languages(const void *a, const void *b) {
    const Language *lang_a = a;
    const Language *lang_b = b;
    return lang_a->order - lang_b->order;
}

// Replaces each two-character sequence \n with a newline, in place.
static void unescape(char *str) {
    char *src = str;
    char *dst = str;

    while (*src) {
        if (src[0] == '\\' && src[1] == 'n') {
            *dst++ = '\n';
            src += 2;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

void lang_close(void) {
    for (size_t i = 0; i < translation_count; i++) {
        free(translations[i].key);
        free(translations[i].value);
    }

    translation_count = 0;
}

// Loads the language at current_language, replacing the current translations.
// If the file can't be opened, the previous translations stay loaded.
static bool lang_load(void) {
    Language *lang = &languages[current_language];
    FILE *file = fopen(lang->filename, "r");

    if (!file) {
        return false;
    }

    lang_close();

    char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), file)) {

        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        char *separator = strchr(line, '=');

        if (!separator) {
            continue;
        }

        *separator = '\0';

        char *key = line;
        char *value = separator + 1;

        if (*key == '\0') {
            continue;
        }

        if (translation_count >= MAX_TRANSLATIONS) {
            break;
        }

        unescape(value);
        translations[translation_count].key = strdup(key);
        translations[translation_count].value = strdup(value);

        if (!translations[translation_count].key || !translations[translation_count].value) {
            free(translations[translation_count].key);
            free(translations[translation_count].value);
            fclose(file);
            lang_close();
            return false;
        }
        translation_count++;
    }
    printf("%s: Loaded %zu translations\n", lang->filename, translation_count);
    fclose(file);
    return true;
}

size_t lang_count(void) {
    return language_count;
}

void lang_next(void) {
    current_language++;

    if (current_language >= language_count) {
        current_language = 0;
    }

    if (!lang_load()) {
        printf("Error loading language: %s\n", languages[current_language].filename);
    }
}

bool lang_init(void) {
    struct dirent *entry;
    Language *lang = NULL;
    DIR *dir = opendir(LANG_DIR);
    if (!dir) {
        return false;
    }
    while ((entry = readdir(dir))) {
        char line[MAX_LINE_LENGTH];
        char fullname[512];
        snprintf(fullname, sizeof(fullname), "%s/%s", LANG_DIR, entry->d_name);
        FILE *file = fopen(fullname, "r");
        if (!file) {
            continue;
        }
        // Checked on an actual language file, and leaving through the normal
        // path below so the languages found so far are still sorted.
        if (language_count >= MAX_LANGUAGES) {
            printf("Too many languages in %s (max %d)\n", LANG_DIR, MAX_LANGUAGES);
            fclose(file);
            break;
        }
        printf("Browsing lang file %s\n", entry->d_name);
        lang = &languages[language_count++];
        memset(lang, 0, sizeof(*lang));
        strcpy(lang->filename, fullname);

        while (fgets(line, sizeof(line), file)) {
            line[strcspn(line, "\r\n")] = '\0';
            if (line[0] == '\0' || line[0] == '#') {
                continue;
            }
            char *separator = strchr(line, '=');
            if (!separator) {
                continue;
            }
            *separator = '\0';
            char *key = line;
            char *value = separator + 1;

            if (*key == '\0') {
                continue;
            }
            if (strcmp(key, "ORDER") == 0) {
                // Only ORDER is needed here; lang_load parses the full file later.
                lang->order = atoi(value);
                break;
            }
        }
        lang = NULL;
        fclose(file);
    }
    qsort(languages, language_count, sizeof(Language), compare_languages);
    current_language = 0;
    closedir(dir);
    return lang_load();
}

const char *lang_get(const char *key) {
    for (size_t i = 0; i < translation_count; i++) {
        if (strcmp(translations[i].key, key) == 0)
            return translations[i].value;
    }
// Missing keys fall back to the key itself, so untranslated text shows up on
// screen instead of crashing.
    return key;
}
