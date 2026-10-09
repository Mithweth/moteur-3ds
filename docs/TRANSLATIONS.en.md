# Translations

This document describes the translation system used by the engine:
language file location and format, key/value pairs, line breaks, and the
special `LANG_NAME` and `ORDER` fields.

## 1. Language files

Translations are stored in:

``` text
resources/lang/
```

During compilation, this directory is copied into the game’s RomFS and
becomes:

``` text
romfs:/lang/
```

By convention, language files use the `.lang` extension, for example:

``` text
resources/lang/en.lang
resources/lang/fr.lang
resources/lang/ja.lang
```

The engine scans the files present in `romfs:/lang/` to build the list
of available languages. It does not use the filename to determine the
language or its display order: that information is stored inside the
file itself.

The extension is not currently checked by the engine. It is therefore
preferable to keep only language files in `resources/lang/`.

The engine supports at most 16 languages. Extra files are ignored, with
`Too many languages` in the log; which ones are ignored depends on the
order in which the files are listed, which is not defined.

## 2. General format

A language file is a simple text file made of pairs:

``` text
KEY=Value
```

For example:

``` ini
TITLE_GAME=New Game
TITLE_CONTROLS=Controls
HUD_OBJECT=Object
HALL_CLOSET=An old wooden cabinet
```

The part before the first `=` is the **key**. The part after it is the
**translated value**.

The first `=` found on the line is used as the separator. A value may
therefore itself contain the `=` character:

``` ini
EXAMPLE=2 + 2 = 4
```

The engine looks up translations by key. These keys are used by rooms,
timelines, the HUD, the inventory, the title screen and, occasionally,
by the C code.

For example, if a room contains:

``` text
MESSAGE HALL_MESSAGE
```

the engine looks up the `HALL_MESSAGE` key in the currently loaded
language:

``` ini
HALL_MESSAGE=There is a message under the carpet
```

Keys must remain **identical in every language**. Only their values
change.

## 3. Spaces and formatting

Spaces around the `=` are not removed automatically.

You must therefore write:

``` ini
TITLE_GAME=New Game
```

and not:

``` ini
TITLE_GAME = New Game
```

In the second case, the key would be `TITLE_GAME`, with a trailing
space, and would therefore not match `TITLE_GAME`.

Likewise, a space placed immediately after `=` is part of the
translation.

Empty lines are ignored. Lines beginning with `#` are comments:

``` ini
# Main menu
TITLE_GAME=New Game
TITLE_CONTROLS=Controls
```

There is no section syntax: all translations belong to the same key
namespace.

## 4. Line breaks

A translation occupies a single physical line in the file. To insert a
line break in the displayed text, use the `\n` sequence:

``` ini
STUDY_NO_LIGHTBULB=No use! There is no\nlight bulb...
```

The engine replaces each `\n` with an actual line break when loading the
file.

Several line breaks can be used:

``` ini
TITLE_INTRO_SCENE_1=January 1985\n\nThe beginning of my career
```

A value must therefore not be split directly across several lines in the
file:

``` text
# Incorrect
MESSAGE=First line
Second line
```

The second line has no `=` and is not part of `MESSAGE`.

## 5. `LANG_NAME`

Each file must define the special key:

``` ini
LANG_NAME=English
```

`LANG_NAME` is the name of the language as it appears in the game menu.

It must be written **in the language it identifies**, rather than
translated from the currently selected language. For example:

``` ini
# en.lang
LANG_NAME=English

# fr.lang
LANG_NAME=Français

# ja.lang
LANG_NAME=日本語
```

When the player changes language, the new file is loaded immediately and
the menu uses that language’s `LANG_NAME`.

`LANG_NAME` is also a normal translation: the translation engine loads
it like any other key/value pair. Its special role comes from the title
screen using it as the label of the entry used to change languages.
With a single language file, that entry is hidden (see
[TITLE.en.md](./TITLE.en.md)).

## 6. `ORDER`

Each file must also define:

``` ini
ORDER=10
```

`ORDER` determines the order in which languages are cycled through.

Files are sorted by increasing numeric value. For example:

``` text
en.lang    ORDER=10
fr.lang    ORDER=20
ja.lang    ORDER=30
```

gives the order:

``` text
English -> Français -> 日本語 -> English -> ...
```

The language with the lowest `ORDER` value becomes the initial language
when the game starts.

It is recommended to use distinct, spaced values such as `10`, `20`,
`30`, making it easy to insert another language later:

``` text
en.lang    ORDER=10
de.lang    ORDER=15
fr.lang    ORDER=20
ja.lang    ORDER=30
```

If `ORDER` is missing, its value is considered to be `0`. A file without
`ORDER` may therefore become the first language. For this reason, every
language file should explicitly define this field.

Like `LANG_NAME`, `ORDER` is also loaded into the translation table when
the complete file is read, but its special purpose is to let the engine
sort the available languages.

## 7. Minimal example

A minimal language file might look like this:

``` ini
LANG_NAME=English
ORDER=10

TITLE_GAME=New Game
TITLE_CONTINUE=Continue
TITLE_CONTROLS=Controls

HUD_OBJECT=Object
HUD_TARGET=Target
HUD_INVENTORY=INVENTORY

GAME_CANNOT_USE_MESSAGE=How am I supposed to use that?
```

The French version would keep exactly the same keys:

``` ini
LANG_NAME=Français
ORDER=20

TITLE_GAME=Nouvelle partie
TITLE_CONTINUE=Continuer
TITLE_CONTROLS=Commandes

HUD_OBJECT=Objet
HUD_TARGET=Cible
HUD_INVENTORY=INVENTAIRE

GAME_CANNOT_USE_MESSAGE=Comment suis-je censé utiliser ça ?
```

The order of keys in the file does not matter for their use. `ORDER` can
technically appear anywhere, but it is preferable to place `LANG_NAME`
and `ORDER` at the beginning of the file so their purpose is immediately
visible.

## 8. Missing translations

When a key requested by the game does not exist in the current language,
the engine displays **the key itself**.

For example, if the game requests:

``` text
HALL_CLOSET
```

but `HALL_CLOSET` is missing from the loaded file, the screen will
display:

``` text
HALL_CLOSET
```

This behavior makes it easy to spot a forgotten translation without
causing an error or interrupting the game.

There is no automatic fallback to another language: each file must
therefore contain all the keys required by the game.

## 9. Adding a language

To add a translation, simply create a new file in `resources/lang/`,
assign it a `LANG_NAME` and an `ORDER`, then copy the existing keys with
their new values.

For example:

``` ini
LANG_NAME=Deutsch
ORDER=15
TITLE_GAME=Neues Spiel
TITLE_CONTINUE=Fortsetzen
TITLE_CONTROLS=Steuerung
...
```

No change to the C code or title screen configuration is required: the
engine automatically discovers the files present in the language
directory.

After adding or modifying a file, rebuild the game normally:

``` sh
make
```

The file will be copied into the RomFS during compilation.

## 10. Current limits

The translation system is deliberately simple. The limits currently
defined by the engine are:

- a maximum of **16 languages**;
- **512 translations** loaded for one language;
- **1024 bytes** when reading a line from the file.

There are no built-in plural forms, variable substitutions or
grammatical rules. When text needs to be built dynamically, the relevant
code or extension assembles the required translations.

The format is therefore essentially a text dictionary:

``` text
key -> displayed text
```

which makes it possible to add or modify a translation without changing
the behavior of rooms, timelines or the engine.
