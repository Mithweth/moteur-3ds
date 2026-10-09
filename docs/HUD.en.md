# HUD description file

This document describes the declarative format used to define the game HUD.

A HUD description defines the layout and appearance of the interface displayed
on the Nintendo 3DS top screen: inventory, movement directions, selected
object, current target and timer.

## 1. Location and general structure

The HUD description is loaded from:

```text
romfs:/hud/hud
```

Its graphical assets are loaded from the corresponding HUD asset set.

The HUD uses the 400 × 240 coordinate space of the Nintendo 3DS top screen.

A typical HUD contains a global background followed by several blocks:

```text
BACKGROUND background

INVENTORY
    ...
END_INVENTORY

DIRECTIONS
    ...
END_DIRECTIONS

OBJECT
    ...
END_OBJECT

TARGET
    ...
END_TARGET

TIMER
    ...
END_TIMER
```

The global `BACKGROUND` and the `INVENTORY` block are required: the HUD fails
to load without them. `DIRECTIONS`, `OBJECT`, `TARGET` and `TIMER` are optional
and are enabled by declaring their corresponding block. Section 11 summarizes
every directive, with the ones that are required.

Blank lines are ignored. A line whose first non-whitespace character is `#` is
a comment. Comments should be written on their own line; inline comments are
not part of the format.

Tokens are separated by spaces. Identifiers such as image names, localization
keys and timeline names therefore do not contain spaces and are not quoted.

Indentation is only for readability; block structure is determined by the
`END_*` directives.

## 2. Global background

Syntax:

```text
BACKGROUND <image>
```

Example:

```text
BACKGROUND background
```

Defines the main HUD background. It is drawn at `(0, 0)`.

This directive is required.

Do not specify an image file extension.

## 3. Inventory

The inventory is described by an `INVENTORY` block:

```text
INVENTORY
    ...
END_INVENTORY
```

This block is required.

### Background

Syntax:

```text
BACKGROUND <image> <x> <y>
```

Example:

```text
BACKGROUND inventory 7 49
```

Defines an optional background image for the inventory area.

`x` and `y` are coordinates in the 400 × 240 HUD coordinate space.

### Title

Syntax:

```text
TEXT <color> <text> <x> <y> <size>
```

Example:

```text
TEXT BLACK HUD_INVENTORY 115 52 0.5
```

Defines the inventory title. This directive is required.

`text` is a localization key. `x` and `y` define the text position and `size`
defines its rendering scale.

### Item position

Syntax:

```text
ITEM_POSITION <x> <y>
```

Example:

```text
ITEM_POSITION 20 75
```

Defines the position of the first visible inventory item.

The position of the other items is calculated from this origin using
`ITEM_SIZE`, `SPACING`, `COLUMNS` and `ROWS`.

### Item size

Syntax:

```text
ITEM_SIZE <size>
```

Example:

```text
ITEM_SIZE 32
```

Defines the rendered size of inventory item icons.

Default:

```text
32
```

### Spacing

Syntax:

```text
SPACING <x> <y>
```

Example:

```text
SPACING 14 10
```

Defines the horizontal and vertical spacing added between inventory items.

The position of an item is calculated as:

```text
x = ITEM_POSITION.x + column * (ITEM_SIZE + SPACING.x)
y = ITEM_POSITION.y + row    * (ITEM_SIZE + SPACING.y)
```

Defaults:

```text
x = 14
y = 10
```

### Columns

Syntax:

```text
COLUMNS <count>
```

Example:

```text
COLUMNS 6
```

Defines the number of inventory columns.

It also determines the vertical navigation step when moving through the
inventory. The value must be at least 1.

Default:

```text
6
```

### Rows

Syntax:

```text
ROWS <count>
```

Example:

```text
ROWS 2
```

Defines the number of inventory rows displayed at once. The value must be at
least 1.

Default:

```text
2
```

### Selection marker

Syntax:

```text
SELECTION <image> <x> <y>
```

Example:

```text
SELECTION selected -4 -4
```

Defines the image used to highlight the currently selected inventory item.
This directive is required.

Unlike most HUD image coordinates, `x` and `y` are offsets relative to the
position of the selected inventory item. Negative values can therefore be used
to draw a selection frame around an item.

### Selected item preview

Syntax:

```text
SELECTED_ITEM <x> <y> <size>
```

Example:

```text
SELECTED_ITEM 20 169 0.5
```

Displays a miniature of the currently selected inventory item at the specified
HUD position.

`size` is the rendering scale of the miniature.

This directive is optional. If it is omitted, no separate miniature of the
selected item is displayed.

`SELECTION` and `SELECTED_ITEM` have different purposes: `SELECTION` highlights
the item inside the inventory grid, while `SELECTED_ITEM` displays a separate
preview elsewhere in the HUD.

### Examination background

Syntax:

```text
EXAMINE_BACKGROUND <image> <x> <y>
```

Example:

```text
EXAMINE_BACKGROUND examine_background 7 49
```

Defines an optional background displayed while examining an inventory item.

### Examination text

Syntax:

```text
EXAMINE_TEXT <color> <x> <y> <size>
```

Example:

```text
EXAMINE_TEXT WHITE 20 62 0.55
```

Defines how the description of the examined item is rendered.

The text itself comes from the inventory item definition; this directive only
defines its color, position and size.

## 4. Directions

The directional indicator is described by a `DIRECTIONS` block:

```text
DIRECTIONS
    ...
END_DIRECTIONS
```

The entire block is optional. If it is omitted, no directional indicator is
displayed.

### Background

Syntax:

```text
BACKGROUND <image> <x> <y>
```

Example:

```text
BACKGROUND directions 303 51
```

Defines an optional background image for the directional indicator.

### Direction images

Syntax:

```text
<direction> <image> <x> <y>
```

Supported directions are:

```text
NORTH
NORTHEAST
EAST
SOUTHEAST
SOUTH
SOUTHWEST
WEST
NORTHWEST
```

Example:

```text
NORTH arrow_n 339 62
NORTHEAST arrow_ne 363 74
```

Each direction has its own image and absolute HUD position.

A direction image is displayed when movement in that direction is currently
available from the active room.

## 5. Selected object

The `OBJECT` block controls the HUD area describing the currently selected
inventory object:

```text
OBJECT
    ...
END_OBJECT
```

The entire block is optional.

### Background

Syntax:

```text
BACKGROUND <image> <x> <y>
```

Example:

```text
BACKGROUND object_background 10 170
```

Defines an optional background image for the object area.

### Label

Syntax:

```text
TEXT <color> <text> <x> <y> <size>
```

Example:

```text
TEXT YELLOW HUD_OBJECT 60 172 0.5
```

Defines the static localized label of the panel. This directive is required
when the `OBJECT` block is present.

`text` is a localization key.

### Item name

Syntax:

```text
ITEM <color> <x> <y> <size>
```

Example:

```text
ITEM WHITE 115 172 0.5
```

Defines how the localized name of the currently selected inventory item is
displayed.

The optional image preview of the selected item is configured separately with
`SELECTED_ITEM` in the `INVENTORY` block.

## 6. Target

The `TARGET` block controls the HUD area describing the current interaction
target:

```text
TARGET
    ...
END_TARGET
```

The entire block is optional.

It supports the same directives as `OBJECT`:

```text
BACKGROUND <image> <x> <y>
TEXT <color> <text> <x> <y> <size>
ITEM <color> <x> <y> <size>
```

Example:

```text
TARGET
    TEXT YELLOW HUD_TARGET 60 208 0.5
    ITEM WHITE 115 208 0.5
END_TARGET
```

`TEXT` defines the static localized label and is required when the `TARGET`
block is present. `ITEM` defines how the localized name of the current target
is displayed.

## 7. Timer

The timer is described by a `TIMER` block:

```text
TIMER
    ...
END_TIMER
```

The entire block is optional. If it is omitted, no timer is displayed and no
timeout timeline is triggered.

### Background

Syntax:

```text
BACKGROUND <image> <x> <y>
```

Example:

```text
BACKGROUND timer_background 330 20
```

Defines an optional timer background.

### Text

Syntax:

```text
TEXT <color> <x> <y> <size>
```

Example:

```text
TEXT WHITE 335 23 0.4
```

Defines the color, position and size of the countdown.

The timer is displayed as:

```text
HH:MM:SS
```

### Maximum duration

Syntax:

```text
MAX_DURATION <seconds>
```

Example:

```text
MAX_DURATION 3600
```

Defines the initial duration of the countdown in seconds. This directive is
required when the `TIMER` block is present, and the value must be greater
than 0.

### Timeout timeline

Syntax:

```text
TIMELINE <timeline>
```

Example:

```text
TIMELINE gameover_timeup
```

Defines the timeline started when the timer reaches zero. This directive is
required when the `TIMER` block is present.

The timer fires only once per game. This timeline is normally a game over
ending with `END`, which goes back to the title screen. If it ends with
`RETURN`, the player goes back to the room and keeps playing without a
countdown.

## 8. Images

Every image referenced by the HUD description must exist in the HUD asset set.

Images are referenced by name:

```text
BACKGROUND inventory 7 49
SELECTION selected -4 -4
NORTH arrow_n 339 62
```

Do not specify an image file extension.

An unknown image causes HUD loading to fail.

## 9. Text and localization

User-visible labels stored in the HUD description use localization keys.

For example:

```text
TEXT YELLOW HUD_OBJECT 60 172 0.5
```

`HUD_OBJECT` is resolved through the current language.

The corresponding values are defined in the language files, for example:

```ini
HUD_OBJECT=Objet
HUD_TARGET=Cible
HUD_INVENTORY=INVENTAIRE
```

Dynamic object names, target names and examination descriptions are obtained
from their respective inventory or room definitions.

Text directives use named colors supported by the graphics color parser.

## 10. Complete example

```text
BACKGROUND background

# ---------------------------------------------------------------------------
# Inventory
# ---------------------------------------------------------------------------

INVENTORY
    BACKGROUND inventory 7 49
    TEXT BLACK HUD_INVENTORY 115 52 0.5

    ITEM_POSITION 20 75
    ITEM_SIZE 32
    SPACING 14 10
    COLUMNS 6
    ROWS 2

    SELECTION selected -4 -4
    SELECTED_ITEM 20 169 0.5

    EXAMINE_BACKGROUND examine_background 7 49
    EXAMINE_TEXT WHITE 20 62 0.55
END_INVENTORY

# ---------------------------------------------------------------------------
# Directions
# ---------------------------------------------------------------------------

DIRECTIONS
    BACKGROUND directions 303 51

    NORTH arrow_n 339 62
    NORTHEAST arrow_ne 363 74
    EAST arrow_e 369 98
    SOUTHEAST arrow_se 362 121
    SOUTH arrow_s 338 129
    SOUTHWEST arrow_sw 314 120
    WEST arrow_w 306 97
    NORTHWEST arrow_nw 314 73
END_DIRECTIONS

# ---------------------------------------------------------------------------
# Selected object
# ---------------------------------------------------------------------------

OBJECT
    #BACKGROUND object_background 10 170
    TEXT YELLOW HUD_OBJECT 60 172 0.5
    ITEM WHITE 115 172 0.5
END_OBJECT

# ---------------------------------------------------------------------------
# Target
# ---------------------------------------------------------------------------

TARGET
    #BACKGROUND target_background 200 170
    TEXT YELLOW HUD_TARGET 60 208 0.5
    ITEM WHITE 115 208 0.5
END_TARGET

# ---------------------------------------------------------------------------
# Timer
# ---------------------------------------------------------------------------

TIMER
    #BACKGROUND timer_background 330 20
    TEXT WHITE 335 23 0.4
    MAX_DURATION 3600
    TIMELINE gameover_timeup
END_TIMER
```

## 11. Directives summary

| Directive            | Block                         | Required        | Description                                              |
|----------------------|-------------------------------|-----------------|----------------------------------------------------------|
| `BACKGROUND`         | —                             | yes             | Global background of the top screen (section 2).         |
| `INVENTORY`          | —                             | yes             | Inventory block, closed by `END_INVENTORY` (section 3).  |
| `BACKGROUND`         | `INVENTORY`                   | no              | Background of the inventory.                             |
| `TEXT`               | `INVENTORY`                   | yes             | Title of the inventory.                                  |
| `ITEM_POSITION`      | `INVENTORY`                   | no (`0 0`)      | Position of the first item of the grid.                  |
| `ITEM_SIZE`          | `INVENTORY`                   | no (`32`)       | Size of an item in the grid.                             |
| `SPACING`            | `INVENTORY`                   | no (`14 10`)    | Horizontal and vertical space between items.             |
| `COLUMNS`            | `INVENTORY`                   | no (`6`)        | Number of columns of the grid, at least 1.               |
| `ROWS`               | `INVENTORY`                   | no (`2`)        | Number of rows of the grid, at least 1.                  |
| `SELECTION`          | `INVENTORY`                   | yes             | Marker drawn around the selected item.                   |
| `SELECTED_ITEM`      | `INVENTORY`                   | no              | Miniature of the selected item elsewhere in the HUD.     |
| `EXAMINE_BACKGROUND` | `INVENTORY`                   | no              | Background shown while an item is examined.              |
| `EXAMINE_TEXT`       | `INVENTORY`                   | no              | Style of the examination text.                           |
| `DIRECTIONS`         | —                             | no              | Directions block, closed by `END_DIRECTIONS` (section 4). |
| `BACKGROUND`         | `DIRECTIONS`                  | no              | Background of the directions.                            |
| `NORTH` … `NORTHWEST` | `DIRECTIONS`                 | no              | Image of one of the eight directions.                    |
| `OBJECT`             | —                             | no              | Selected object block, closed by `END_OBJECT` (section 5). |
| `TARGET`             | —                             | no              | Target block, closed by `END_TARGET` (section 6).        |
| `BACKGROUND`         | `OBJECT`, `TARGET`            | no              | Background of the panel.                                 |
| `TEXT`               | `OBJECT`, `TARGET`            | yes (if block)  | Label of the panel.                                      |
| `ITEM`               | `OBJECT`, `TARGET`            | no              | Style of the item or target name.                        |
| `TIMER`              | —                             | no              | Timer block, closed by `END_TIMER` (section 7).          |
| `BACKGROUND`         | `TIMER`                       | no              | Background of the timer.                                 |
| `TEXT`               | `TIMER`                       | no              | Style of the remaining time.                             |
| `MAX_DURATION`       | `TIMER`                       | yes (if block)  | Duration of the countdown in seconds, greater than 0.    |
| `TIMELINE`           | `TIMER`                       | yes (if block)  | Timeline started when the time is up.                    |

The value in brackets is the default used when the directive is omitted.

Notes:

- **Optional blocks.** `DIRECTIONS`, `OBJECT`, `TARGET` and `TIMER`
  enable their HUD component: omit the block to disable it. Inside a
  block, a directive marked "yes (if block)" is only required when that
  block is present. No `NONE`, `DISABLED` or `ENABLED` keyword exists.
- **Repeated directives.** Each directive may appear several times; the
  last value is used. The same goes for a block opened twice: the second
  one completes or overrides the first.
- **Errors.** Unlike the title screen, the HUD is strict: an unknown
  directive (or one placed in the wrong block), a missing argument, an
  unknown image, a block without its `END_*`, `COLUMNS 0`, `ROWS 0` or a
  missing required directive all make the HUD fail to load, with a
  message giving the file and the line.
## 12. Common mistakes

### Adding an image extension

Do not do this:

```text
BACKGROUND inventory.png 7 49
```

Use the image name without an extension:

```text
BACKGROUND inventory 7 49
```

### Confusing SELECTION and SELECTED_ITEM

`SELECTION` is the marker drawn around the selected item in the inventory grid.

`SELECTED_ITEM` is an optional separate miniature of that item elsewhere in the
HUD.

### Using literal text instead of localization keys

Do not put user-visible labels directly in a `TEXT` directive:

```text
TEXT YELLOW Object 60 172 0.5
```

Use a localization key:

```text
TEXT YELLOW HUD_OBJECT 60 172 0.5
```

### Forgetting END_* directives

Every block must be closed by its corresponding directive:

```text
END_INVENTORY
END_DIRECTIONS
END_OBJECT
END_TARGET
END_TIMER
```

## 13. Recommended style

For readability, the HUD description should normally be ordered as:

```text
Global background
Inventory
Directions
Selected object
Target
Timer
```

Use section comments and indent nested directives consistently. Keep related
layout directives together, and omit optional components instead of declaring
unused placeholders.

The HUD format is intentionally a small game-specific layout description, not
a general UI language. If the interface requires behavior that cannot be
expressed cleanly with the existing directives, prefer adding one small,
reusable primitive to the format.
