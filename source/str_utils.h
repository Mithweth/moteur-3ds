// str_utils.h
// Small string helpers shared by the text file parsers (rooms, inventory,
// timelines, game states).
#pragma once
#include <stdbool.h>

// Trims leading and trailing whitespace in place. Returns a pointer inside
// str (past the leading whitespace); the trailing whitespace is overwritten
// with '\0'.
char *str_trim(char *str);

// Returns true only for the exact string "true"; anything else is false.
bool str_to_bool(const char *str);
