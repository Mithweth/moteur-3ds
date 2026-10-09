// str_utils.c
// See str_utils.h.
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

char *str_trim(char *str) {
    while (*str && isspace((unsigned char)*str)) {
        str++;
    }
    if (*str == '\0') {
        return str;
    }
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        *end-- = '\0';
    }
    return str;
}


bool str_to_bool(const char *str) {
    return strcmp(str, "true") == 0;
}
