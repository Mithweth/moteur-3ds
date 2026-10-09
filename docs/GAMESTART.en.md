# Game configuration file

This document describes the `game` file, which defines how a new game
starts (the first room, the background music and the items already in the
inventory), where the game is saved, and the colors of the message box and
of the Start menu.

## 1. Location and loading

The file is written in:

```text
resources/game/game
```

The build copies every file of `resources/game/` to RomFS, and the game
reads it from:

```text
romfs:/game/game
```

The file is read **once**, when the game starts up, before the inventory
and the HUD are initialized. If the file is missing or invalid, the game
does not start.

Its contents are then applied **every time a new game starts** from the
title screen ("New game"):

1. the inventory, the game states and the HUD are reset;
2. the music is started, if `MUSIC` is set;
3. the items listed by `ITEM` are added to the inventory;
4. the player enters the room named by `ROOM`.

When the player chooses "Continue", only steps 1 and 2 apply: the
inventory and the room come from the save, not from `ITEM` and `ROOM`.

The colors and the text size apply for the whole game.

## 2. General syntax

The file contains one command per line, followed by its arguments:

```text
ROOM hall
MUSIC romfs:/audio/background.ogg
ITEM FLASHLIGHT
```

- The command and its arguments are separated by **spaces**. Tabs are not
  accepted as separators.
- Spaces and tabs at the start and at the end of a line are ignored.
- Commands are case-sensitive: `ROOM` is valid, `Room` is not.
- Empty lines or lines starting with `#` are ignored.
- Extra words after the expected arguments are ignored.
- Paths and names containing spaces are **not** supported.
- An unknown command makes loading fail.
- A line must not exceed 511 characters.

### 2.1. Colors

Color commands expect four integers separated by spaces (`A` means opacity). The valid values are from 0 to 255.

```text
<R> <G> <B> <A>
```

Example: `150 120 55 255` is an opaque gold.


## 3. Commands

| Command        | Required | Repeatable | Description                                         |
|----------------|----------|------------|-----------------------------------------------------|
| `ROOM`         | yes      | no         | Room in which a new game starts.                    |
| `MUSIC`        | no       | no         | Background music played during the game.            |
| `ITEM`         | no       | yes (16)   | Item already in the inventory when the game starts. |
| `TEXT_COLOR`   | no       | —          | Text color of the message box and of the menu.      |
| `TEXT_SIZE`    | no       | —          | Text size of the Start menu buttons.                |
| `BUTTON_COLOR` | no       | —          | Background of the selected Start menu button.       |
| `FRAME_COLORS` | yes      | no         | Frame colors block (see 3.7).                       |
| `SAVE`         | no       | —          | Save file; enables saving (see 3.8).                |
| `CANNOT_USE_MESSAGE` | no | no         | Message when an item cannot be used (see 3.9).      |

`ROOM`, `MUSIC` and `CANNOT_USE_MESSAGE` may appear only once: a second occurrence makes loading
fail. For the color commands, `TEXT_SIZE` and `SAVE`, if a command is
repeated, the last value wins.

### 3.1. ROOM

```text
ROOM hall
```

Name of the starting room, that is the name of its directory in
`resources/rooms/` (loaded from `romfs:/rooms/<name>`).

This command is required: without it, loading fails with
`<file>: Missing ROOM`.

The room is only loaded when a game starts, not when the file is read. A
misspelled room name is therefore only reported at that moment, with
`Cannot enter room: <name>`, and the game goes back to the title screen.

### 3.2. MUSIC

```text
MUSIC background
MUSIC romfs:/audio/background.ogg
```

An Ogg Vorbis file (mono or stereo), given in one of two ways (pick one:
`MUSIC` may appear only once):

- a relative name, **without extension**, resolved from `romfs:/game`
  with `.ogg` appended: `MUSIC background` plays
  `romfs:/game/background.ogg`;
- an absolute path starting with `romfs:/`, used exactly as written: no
  extension is added, so write it in full, `.ogg` included
  (`MUSIC romfs:/audio/background.ogg`).

The music is played when a game starts, and played again when the
player leaves a mini-game or a timeline ending with `RETURN`.

Without this command, the game is silent: no music is played, neither at
the start nor after a mini-game.

### 3.3. ITEM

```text
ITEM FLASHLIGHT
ITEM SCREWDRIVER
```

One item identifier per line, as declared by the `ITEM` directives of
`resources/inventory/inventory`. Items are added to the inventory in file
order.

- 16 items at most: a 17th `ITEM` makes loading fail with
  `too many ITEM (max 16)`.
- An unknown identifier does not prevent the game from starting: the game
  prints `Unknown inventory item: <id>` and skips it.
- An item listed twice is only added once.
- Without `ITEM`, the inventory starts empty.

### 3.4. TEXT_COLOR

```text
TEXT_COLOR 255 255 255 255
```

Text color of the message box and of the Start menu buttons (see 2.1).
Default: opaque white, `255 255 255 255`.

### 3.5. TEXT_SIZE

```text
TEXT_SIZE 0.6
```

Text scale of the Start menu buttons, as a decimal number with a dot
(`0.6`, not `0,6`). `1.0` is the normal font size. Default: `0.6`.

The text size of the message box is not affected.

### 3.6. BUTTON_COLOR

```text
BUTTON_COLOR 105 82 40 255
```

Color of the rectangle drawn behind the selected Start menu button (see
2.1). Default: `105 82 40 255`.

### 3.7. FRAME_COLORS ... END_FRAME_COLORS

```text
FRAME_COLORS
    SHADOW 0 0 0 150
    OUTER_BORDER 150 120 55 255
    OUTER_BACKGROUND 18 24 34 235
    INNER_BORDER 105 82 40 255
    INNER_BACKGROUND 22 28 40 245
END_FRAME_COLORS
```

Colors of the frame of the message box and of the Start menu. The frame is
drawn in five layers, from the outside in:

| Command            | Layer                                                 |
|--------------------|-------------------------------------------------------|
| `SHADOW`           | drop shadow, offset by 3 pixels down and to the right |
| `OUTER_BORDER`     | outer border (2 pixels)                               |
| `OUTER_BACKGROUND` | background between the two borders (3 pixels)        |
| `INNER_BORDER`     | inner border (1 pixel)                                |
| `INNER_BACKGROUND` | background the text is drawn on                       |

- Between `FRAME_COLORS` and `END_FRAME_COLORS`, only these five commands
  are accepted. Any other command makes loading fail.
- A layer missing from the block stays **transparent** (`0 0 0 0`): it is
  not drawn.
- The block is required and must be closed by `END_FRAME_COLORS`

### 3.8. SAVE

```text
SAVE sdmc:/moteur.save
```

Full path of the save file. The game has a single save slot.

- The file must be on the SD card, so the path starts with `sdmc:/`. RomFS
  is read-only: a `romfs:/` path makes every save fail.
- The directory must already exist: the game creates the file, not the
  directories leading to it. A file at the root of the card avoids the
  problem.
- The path is at most 251 characters long, because the game also writes
  a temporary file named `<path>.tmp` (see below).

Without this command, saving is disabled: the Start menu has no "Save"
button and the title screen never shows `CONTINUE`.

With it:

- "Save" in the Start menu writes the elapsed time, the current room, the
  inventory, the game states that are true and the data of the
  extensions. It is only offered while the player explores a room, not
  during a message, a mini-game or a timeline.
- The save is written to `<path>.tmp` first, then renamed: a failed or
  interrupted save keeps the previous one.
- `CONTINUE` on the title screen loads it (see the title screen
  documentation); "New game" deletes it.

### 3.9. CANNOT_USE_MESSAGE

```text
CANNOT_USE_MESSAGE GAME_CANNOT_USE_MESSAGE
```

Translation key of the message shown when the player uses an item (A)
on a target that has no matching `USE` block, and the item has no
`USE_CALLBACK`.

- Without this command, nothing is shown: the A press simply does
  nothing. It is up to each game to decide whether this feedback is
  wanted.
- The message is only shown when a hotspot is targeted. With no target,
  nothing happens.
- The value is a translation key, to define in every `.lang` file (see
  the translations documentation).

## 4. Complete example

```text
# Game configuration

# First room
ROOM hall

# Background music
MUSIC romfs:/audio/background.ogg

# Starting inventory
ITEM FLASHLIGHT
ITEM MAGNIFYING_GLASS

# Save slot on the SD card
SAVE sdmc:/moteur.save

# Feedback when an item is used in the wrong place
CANNOT_USE_MESSAGE GAME_CANNOT_USE_MESSAGE

# Message box and Start menu
TEXT_COLOR 255 255 255 255
TEXT_SIZE 0.6
BUTTON_COLOR 105 82 40 255

FRAME_COLORS
    SHADOW 0 0 0 150
    OUTER_BORDER 150 120 55 255
    OUTER_BACKGROUND 18 24 34 235
    INNER_BORDER 105 82 40 255
    INNER_BACKGROUND 22 28 40 245
END_FRAME_COLORS
```

## 5. Common mistakes

### Forgetting `END_FRAME_COLORS`

As long as the block is not closed, only the frame color commands are
accepted: a `ROOM` or `MUSIC` line placed after it makes loading fail
with `unknown command`. If the block is the last one in the file, loading
fails with `missing END_FRAME_COLORS`.

### Giving a color name

```text
TEXT_COLOR WHITE
```

Unlike the title screen file, this file does not accept color names: give
the four `R G B A` values.

### Forgetting the opacity

```text
TEXT_COLOR 255 255 255
```

The opacity is required: without it, loading fails with
`invalid TEXT_COLOR`. Use `255` for an opaque color.

### Giving an extension, or forgetting romfs:/, in the music path

```text
MUSIC background.ogg
MUSIC audio/background.ogg
```

Without the `romfs:/` prefix, the name is relative to `romfs:/game` and
`.ogg` is appended: these lines look for
`romfs:/game/background.ogg.ogg` and `romfs:/game/audio/background.ogg.ogg`.
Write `MUSIC background` for `romfs:/game/background.ogg`, or the full
path `MUSIC romfs:/audio/background.ogg` for a file elsewhere.

### Misspelling the SD card prefix

```text
SAVE smdc:/moteur.save
```

The prefix is `sdmc:`. With any other prefix the file cannot be created:
loading the game file still succeeds, but every save fails with the
"save error" message and `CONTINUE` never appears on the title screen.

### Using the item name instead of its identifier

`ITEM` expects the identifier declared after `ITEM` in
`resources/inventory/inventory` (e.g. `FLASHLIGHT`), not the translation
key nor the image name.
