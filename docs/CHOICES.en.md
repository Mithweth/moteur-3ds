# Choice files

This document describes the format of the files opened by the `CHOICE`
room action.

A choice shows a question on the bottom screen with a list of buttons.
The player picks one, and the actions of that option run. Choices can be
nested, which makes them suitable both for small menus ("What should you
do?") and for simulating a conversation, where each answer leads to new
questions.

## 1. Location and loading

A choice file is stored in the directory of the room that uses it, next
to its `room` script, and is referred to by its file name:

```text
resources/rooms/sample1/room       HOTSPOT ... ACTION ... CHOICE dustbin
resources/rooms/sample1/dustbin    the choice file
```

The build copies every file **without an extension** found in a room
directory to RomFS, so the file above is read from
`romfs:/rooms/sample1/dustbin`. Do not give a choice file an extension:
`dustbin.txt` would not be copied.

The file is read when the `CHOICE` action runs, not when the room is
loaded, and freed when the player leaves the outermost choice or the
room. A file that cannot be read or contains an error is reported in the
logs, and the `CHOICE` action is skipped: the action flow goes on with
the next action.

## 2. General syntax

The lexical rules are those of room files (see `ROOMS.en.md`):

- tokens are separated by **spaces**;
- leading and trailing spaces and tabs are ignored, so indentation is
  only for readability;
- empty lines and lines starting with `#` are ignored;
- commands are case-sensitive;
- a line must not exceed 511 characters.

## 3. Structure

A file contains one or more `CHOICE` blocks:

```text
CHOICE MAIN
    PROMPT DUSTBIN_WHAT_TO_DO

    OPTION EMPTY_BIN
        MESSAGE FLEMME
    END_OPTION

    OPTION LEAVE
        EXIT_CHOICE
    END_OPTION
END_CHOICE
```

| Directive                   | Where             | Description                                               |
|-----------------------------|-------------------|-----------------------------------------------------------|
| `CHOICE <name>`             | file level        | Opens a block. `<name>` is unique in the file.            |
| `PROMPT <key>`              | in a `CHOICE`     | Translation key of the question. Required, exactly once.  |
| `OPTION <key>`              | in a `CHOICE`     | Opens an option; `<key>` is the translation key of its button, unique in the block. |
| `WHEN <type> <name> <value>`| in an `OPTION`    | Condition for the option to be shown (see 4).             |
| *action*                    | in an `OPTION`    | Run when the player picks the option (see 5).             |
| `END_OPTION`                | in an `OPTION`    | Closes the option.                                        |
| `END_CHOICE`                | in a `CHOICE`     | Closes the block. It needs a `PROMPT` and at least one `OPTION`. |

The block named **`MAIN`** is required: it is the one shown by the
`CHOICE` action of the room. The other blocks are only reachable through
a `CHOICE` action inside an option (see 6).

Options are shown in file order.

Limits:

| What                               | Maximum |
|------------------------------------|---------|
| `CHOICE` blocks per file           | 16      |
| options per block                  | 16      |
| `WHEN` conditions per option       | 8       |
| actions per option                 | 16      |
| choices open at once (nesting)     | 8       |

The number of buttons that fit on screen also depends on the text size
set in the game file (see section 8).

## 4. Conditions

`WHEN` works as in room files, with the same types:

```text
OPTION THROW_CAN
    WHEN INVENTORY_HAS EMPTY_BEER true
    MESSAGE NO_PLACE
END_OPTION
```

An option is shown only if all its conditions match. A hidden option has
no button: the D-pad and the touch screen skip it.

Conditions are checked again every frame. If an action changes a state
or the inventory, options appear or disappear while the choice is shown;
if the selected option disappears, the selection moves to the first
option still shown.

A choice that has no option left is not shown: it is left at once, as if
`EXIT_CHOICE` had run (see 6).

## 5. Actions

An option accepts every room action (`SET`, `INVENTORY_ADD`,
`INVENTORY_REMOVE`, `MESSAGE`, `MESSAGE_IMAGE`, `SFX`, `WAIT_SFX`,
`ROOM`, `TIMELINE`, `MINIGAME`) with the same meaning, described in
[ROOMS.en.md, section 9](./ROOMS.en.md#9-available-actions), plus two
actions specific to choices:

| Directive        | Effect                                                                  |
|------------------|-------------------------------------------------------------------------|
| `CHOICE <name>`  | Opens the block `<name>` **of the same file** over the current choice.   |
| `EXIT_CHOICE`    | Leaves the current choice (see 6). Actions after it in the option never run. |

`MESSAGE_IMAGE` uses the images of the room the file belongs to. A
misspelled mini-game name or image name makes loading fail, as in a
room file.

An option may have no action at all: picking it then does nothing.

## 6. Action flow

### Opening

The room action `CHOICE <file>` suspends the room block, like `MESSAGE`:
the file is loaded and its `MAIN` block is shown, with the first option
shown selected. Later `ACTION` blocks of the hotspot or path are not run.

### Picking an option

The actions of the option run. When they are over and the option did
not leave the choice, **the same choice is shown again**, with the same
option selected. The player can therefore try several answers, which is
what a conversation needs.

While an action of the option runs (a message, a sound, a timeline, a
mini-game), the choice is hidden; it comes back once the option's flow is
over or suspended by a nested choice.

### Nested choices

`CHOICE <name>` inside an option opens another block of the file **over**
the current one. When the player leaves it, the parent choice is shown
again and the actions that followed `CHOICE <name>` in the option run:

```text
CHOICE MAIN
    PROMPT DUSTBIN_WHAT_TO_DO

    OPTION EMPTY_BIN
        CHOICE WHERE
        # Shown after leaving WHERE.
        MESSAGE BIN_STILL_FULL
    END_OPTION

    OPTION LEAVE
        EXIT_CHOICE
    END_OPTION
END_CHOICE

CHOICE WHERE
    PROMPT DUSTBIN_WHERE_TO_THROW

    # WHERE is shown again after the message.
    OPTION PAVEMENT
        MESSAGE WAY_TOO_FAR
    END_OPTION

    # Back to MAIN.
    OPTION GIVE_UP
        EXIT_CHOICE
    END_OPTION
END_CHOICE
```

A block may open itself or one of its parents again: each `CHOICE` adds
a level, up to 8. Beyond that, the `CHOICE` action is skipped.

### Leaving a choice

`EXIT_CHOICE` and the **B** button leave the current level only:

- in a nested choice, the parent choice is shown again, with its first
  shown option selected, and the rest of the option that opened the
  nested choice runs;
- in `MAIN`, the choice is closed and the file freed, and the rest of the
  room block that contained `CHOICE <file>` runs.

To close every level at once, put `EXIT_CHOICE` after the nested
`CHOICE` in the parent option as well.

A choice without any reachable `EXIT_CHOICE` can still be left with B.

### Other ways out

- `ROOM` changes room and closes every choice.
- `TIMELINE`: after `RETURN`, the choice comes back and the rest of the
  option runs; after `END`, the game goes back to the title screen.
- `MINIGAME`: the choice comes back once the mini-game has ended.

## 7. Controls

| Control         | Effect                                                |
|-----------------|-------------------------------------------------------|
| **↑ / ↓** (D-pad or Circle Pad) | moves the selection between the options shown |
| **A**           | picks the selected option                             |
| touch           | picks the touched option                              |
| **B**           | leaves the current choice (same as `EXIT_CHOICE`)     |
| **START**       | opens the START menu over the choice                  |

While a choice is shown, the room receives no input: the player cannot
move, touch a hotspot or use an item. The countdown of the HUD, if any,
keeps running; when the time is up, its timeline starts.

The START menu is drawn with the same box. Closing it brings the choice
back with its selection. "Save" is not offered while a choice is open,
since the save could not record it: the player has to leave the choice
first.

## 8. Appearance

The box is drawn with the frame colors of the game file, and its other
settings come from the `CHOICE` block of that file (`FRAME_WIDTH`, text
colors and sizes, color of the selected button, padding): see
`GAMESTART.en.md`. They apply to every choice and to the START menu.

The question is wrapped to the width of the box; the button labels are
not, so keep them short.

## 9. Load errors

Every error stops loading with `<file>:<line>: <message>` in the logs,
and the `CHOICE` action is skipped.

| Message                         | Cause                                                              |
|---------------------------------|--------------------------------------------------------------------|
| `Cannot open choice file`       | no such file in the room directory (misspelled name, extension)    |
| `invalid CHOICE declaration`    | missing name, more than 16 blocks, or `CHOICE` inside a block but outside an `OPTION` |
| `duplicate CHOICE`              | two blocks with the same name                                      |
| `invalid PROMPT`                | outside a block, inside an option, without a key, or repeated      |
| `invalid OPTION`                | outside a block, inside another option, without a key, or more than 16 |
| `duplicate OPTION`              | two options with the same key in a block                           |
| `END_OPTION without OPTION`     | `END_OPTION` with no open option                                   |
| `invalid END_CHOICE`            | open option, or block without `PROMPT` or without `OPTION`         |
| `invalid WHEN`                  | outside an option, missing argument, or more than 8 conditions     |
| `unknown WHEN condition`        | type other than `STATE_IS` or `INVENTORY_HAS`                      |
| `action outside OPTION`         | an action directly in a block                                      |
| `invalid action`                | unknown action, or action without argument                         |
| `unknown MESSAGE_IMAGE image`   | image not found in the room's images                               |
| `invalid MINIGAME`              | mini-game not registered in `extensions/callbacks.c`               |
| `too many ACTION`               | more than 16 actions in an option                                  |
| `unclosed CHOICE or OPTION`     | missing `END_OPTION` or `END_CHOICE` at the end of the file        |
| `missing CHOICE MAIN`           | no block named `MAIN`                                              |
| `invalid nested CHOICE`         | `CHOICE <name>` in an option names a block that does not exist     |

## 10. Complete example

The room `sample1` declares the hotspot:

```text
HOTSPOT DUSTBIN 212 160 43 59
    WHEN STATE_IS raytracing_enabled false
    ACTION
        CHOICE dustbin
    END_ACTION
END_HOTSPOT
```

and `resources/rooms/sample1/dustbin` contains:

```text
CHOICE MAIN
    PROMPT DUSTBIN_WHAT_TO_DO

    OPTION EMPTY_BIN
        CHOICE WHERE
    END_OPTION

    OPTION THROW_CAN
        WHEN INVENTORY_HAS EMPTY_BEER true
        MESSAGE NO_PLACE
    END_OPTION

    OPTION LEAVE
        EXIT_CHOICE
    END_OPTION
END_CHOICE

CHOICE WHERE
    PROMPT DUSTBIN_WHERE_TO_THROW

    OPTION NEIGHBOR_BIN
        MESSAGE TOO_FAR
    END_OPTION

    OPTION PAVEMENT
        MESSAGE WAY_TOO_FAR
    END_OPTION

    OPTION GIVE_UP
        EXIT_CHOICE
    END_OPTION
END_CHOICE
```

Every `PROMPT`, `OPTION` and `MESSAGE` key is defined in each `.lang`
file.

## 11. Common mistakes

### Naming a file in a nested CHOICE

Inside a choice file, `CHOICE <name>` names a **block of the same
file**, not another file: `CHOICE other_file` fails with
`invalid nested CHOICE`.

### Putting actions after EXIT_CHOICE

```text
OPTION LEAVE
    EXIT_CHOICE
    MESSAGE GOODBYE
END_OPTION
```

`GOODBYE` is never shown. Put the actions before `EXIT_CHOICE`, or after the `CHOICE` that opened
the choice (in the parent option or in the room block).

### Expecting the choice to close after an option

An option that does not run `EXIT_CHOICE` brings the same choice back.
That is the intended behavior for conversations; add `EXIT_CHOICE` at the
end of the option to close the choice instead.

### Hiding every option

If the `WHEN` conditions hide every option, the choice is never shown and
the room block goes on at once. Keep at least one option without
condition (typically the one that leaves).
