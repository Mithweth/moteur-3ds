# Title screen description file

This document describes the declarative format used to define the game
title screen.

A title description defines the top and bottom backgrounds, menu layout
and order, title-screen sound effects and music, controls page and
credits page.
Menu behavior itself remains implemented by the game.

## 1. Location and general structure

The title description is loaded from:

``` text
romfs:/game/title
```

Its graphical assets are loaded from the sprite sheet in:

``` text
romfs:/game
```

The top screen uses a 400 × 240 coordinate space. The menu, controls and
credits are displayed on the 320 × 240 bottom screen.

A typical title description has the following structure:

``` text
BACKGROUND_TOP background_top
BACKGROUND_BOTTOM background_bottom

SFX_SELECT title_select
SFX_CHOICE title_choice
MUSIC title_music

MENU
    ORDER LANG,INTRO,GAME,CONTROLS,CREDITS
    DEFAULT GAME
    ...
END_MENU

CONTROLS
    ...
END_CONTROLS

CREDITS
    ...
END_CREDITS
```

`MENU` defines the main menu and its order. `CONTROLS` and `CREDITS`
define the two optional sub-pages opened by their corresponding menu
entries.

Blank lines are ignored. A line whose first non-whitespace character is
`#` is a comment. Comments should be written on their own line; inline
comments are not part of the format.

Tokens are separated by spaces. Identifiers such as image names,
localization keys and sound names therefore do not contain spaces and
are not quoted. The exception is the person name in a `CREDIT`
directive: everything after the role key belongs to the name and may
contain spaces.

Indentation is only for readability; block structure is determined by
the `END_*` directives.

`BACKGROUND_TOP`, `BACKGROUND_BOTTOM`, `SFX_SELECT`, `SFX_CHOICE` and
`MUSIC` must be placed outside of any block, before or after them.
Inside a `MENU`, `CONTROLS` or `CREDITS` block, they are unknown
directives and make loading fail.

## 2. Backgrounds

Syntax:

``` text
BACKGROUND_TOP <image>
BACKGROUND_BOTTOM <image>
```

Example:

``` text
BACKGROUND_TOP background_top
BACKGROUND_BOTTOM background_bottom
```

`BACKGROUND_TOP` defines the image drawn at `(0, 0)` on the top screen.

`BACKGROUND_BOTTOM` defines the image drawn at `(0, 0)` behind the main
menu on the bottom screen. It is not drawn on the controls or credits
pages, which have their own `BACKGROUND` directive (see sections 5 and
6).

Both directives are optional. If a background is omitted, no image is
drawn for that screen and the background remains black.

Image names refer to images in the `romfs:/game` sprite sheet. Do not
specify an image file extension.

## 3. Sound effects and music

Syntax:

``` text
SFX_SELECT <sound>
SFX_CHOICE <sound>
```

Example:

``` text
SFX_SELECT title_select
SFX_CHOICE title_choice
```

`SFX_SELECT` is played when the highlighted menu entry changes.

`SFX_CHOICE` is played when a menu entry is activated and when the
controls or credits page is closed with A or B.

Both directives are optional.

A relative sound name is given without extension: it is resolved from
`romfs:/game` and automatically receives the `.raw` extension:

``` text
SFX_SELECT title_select
```

resolves to:

``` text
romfs:/game/title_select.raw
```

An absolute `romfs:/` path is also accepted. It is used exactly as
written: no extension is added, so write it in full, `.raw` included:

``` text
SFX_CHOICE romfs:/audio/menu_choice.raw
```

### Music

Syntax:

``` text
MUSIC <music>
```

Example:

``` text
MUSIC title
MUSIC romfs:/audio/title.ogg
```

Background music played in a loop while the title screen is shown. The
file must be an Ogg Vorbis file, mono or stereo.

This directive is optional. If it appears several times, the last one
is used.

A relative name is given without extension: it is resolved from
`romfs:/game` and automatically receives the `.ogg` extension:
`MUSIC title` resolves to `romfs:/game/title.ogg`. An absolute
`romfs:/` path is used exactly as written: no extension is added, so
write it in full, `.ogg` included (`MUSIC romfs:/audio/title.ogg`).

The music starts once the whole description has been loaded
successfully. It stops when the title screen is left: intro, new game
or loaded game. The game music (`MUSIC` in `romfs:/game/game`, see
[GAMESTART.en.md](./GAMESTART.en.md)) then takes over when the game
starts. A missing file does not prevent the title screen from loading:
it only prints `File not found: <path>` and the screen stays silent.

## 4. Menu

The main menu is described by a `MENU` block:

``` text
MENU
    ...
END_MENU
```

The menu must contain an `ORDER` directive with at least one valid
entry.

### Order

Syntax:

``` text
ORDER <entry>,<entry>,...
```

Example:

``` text
ORDER LANG,INTRO,GAME,CONTROLS,CREDITS
```

The supported entries are:

  Entry        Displayed localization key   Action
  ------------ ---------------------------- -----------------------------
  `LANG`       `LANG_NAME`                  Switch to the next language
  `INTRO`      `TITLE_INTRO`                Start the intro
  `CONTINUE`   `TITLE_CONTINUE`             Load the saved game
  `GAME`       `TITLE_GAME`                 Start a new game
  `CONTROLS`   `TITLE_CONTROLS`             Open the controls page
  `CREDITS`    `TITLE_CREDITS`              Open the credits page

The entries are displayed in exactly the order given by `ORDER`. Entries
that are not listed are not displayed.

Three entries are also hidden when they have nothing to do:

- `LANG` when there is only one language file in `romfs:/lang` (see
  [TRANSLATIONS.en.md](./TRANSLATIONS.en.md));
- `INTRO` without an `INTRO` directive (see [Intro](#intro));
- `CONTINUE` when there is no save, which is checked each time the title
  screen is opened. Saving requires the `SAVE` command of the game
  configuration file (`romfs:/game/game`); without it, `CONTINUE` never
  appears.

`GAME` deletes the existing save before starting the new game, so
`CONTINUE` is gone the next time the title screen is shown, until the
player saves again. If the save cannot be read, `CONTINUE` starts a new
game instead and tells the player so.

Do not put spaces around the commas:

``` text
ORDER INTRO,GAME,CONTROLS,CREDITS,LANG
```

### Default selection

Syntax:

``` text
DEFAULT <entry>
```

Example:

``` text
DEFAULT GAME
```

Defines the menu entry highlighted when the title screen is opened.

`DEFAULT` may appear before or after `ORDER`; the selection is resolved
after the complete description has been read.

The selected entry should also be present in `ORDER`. If `DEFAULT` is
omitted or designates an entry that is not displayed (missing from
`ORDER`, or hidden as described above), the first displayed entry is
selected.

### Intro

Syntax:

``` text
INTRO <timeline>
```

Example:

``` text
INTRO intro
```

Names the timeline played by the `INTRO` menu entry. `<timeline>` is a
directory name under `resources/timelines/` (`romfs:/timelines/` at run
time), without that prefix: `INTRO romfs:/timelines/intro` does not
work. The name is at most 255 characters long.

Without an `INTRO` directive, the `INTRO` entry is removed from the menu
even if it is listed in `ORDER`.

### Position

Syntax:

``` text
POSITION <x> <y>
```

Example:

``` text
POSITION 160 70
```

`x` is the horizontal center of every menu entry. `y` is the vertical
position of the first entry.

Default:

``` text
160 70
```

### Spacing

Syntax:

``` text
SPACING <spacing>
```

Example:

``` text
SPACING 30
```

Defines the vertical distance between two consecutive menu entries.

Default:

``` text
30
```

### Text size

Syntax:

``` text
TEXT_SIZE <size>
```

Example:

``` text
TEXT_SIZE 0.65
```

Defines the rendering scale of the menu labels.

Default:

``` text
0.65
```

### Colors

Syntax:

``` text
COLOR <color>
SELECTED_COLOR <color>
```

Example:

``` text
COLOR GRAY
SELECTED_COLOR LIGHTGRAY
```

`COLOR` defines the color of normal menu entries. `SELECTED_COLOR`
defines the color of the highlighted entry.

Color names are resolved by the graphical color parser used by the game.

Defaults:

``` text
COLOR GRAY
SELECTED_COLOR LIGHTGRAY
```

### Version

The game version is not configurable in the title description. It is
always drawn by the game in the bottom-right corner of the menu.

## 5. Controls page

The controls page is described by a `CONTROLS` block:

``` text
CONTROLS
    ...
END_CONTROLS
```

It supports `BACKGROUND`, `IMAGE` and `TEXT` directives. The page is
opened by the `CONTROLS` menu entry and closed with A or B.

### Background

Syntax:

``` text
BACKGROUND <image>
```

Example:

``` text
BACKGROUND controls_background
```

Defines the image drawn at `(0, 0)` on the bottom screen while the
controls page is open, behind its images and texts.

This directive is optional. If it is omitted, the page has no
background. If it appears several times, the last one is used.

### Image

Syntax:

``` text
IMAGE <image> <x> <y>
```

Example:

``` text
IMAGE analogpad 10 0
```

Draws an image from the `romfs:/game` sprite sheet at the specified
position on the bottom screen.

Do not specify an image file extension.

A controls page can contain up to 8 `IMAGE` directives.

### Text

Syntax:

``` text
TEXT <localization_key> <x> <y> <size> [alignment]
```

Example:

``` text
TEXT TITLE_CONTROLS_MOVE 36 15 0.55 CENTER
```

Draws the localized text associated with `localization_key` at the
specified position and scale.

The optional `alignment` defines what `x` refers to:

  Alignment    `x` is
  ------------ ------------------------------
  `LEFT`       the left edge of the text
  `CENTER`     the horizontal center of the text
  `RIGHT`      the right edge of the text

If `alignment` is omitted, the text is aligned on the left. An unknown
alignment makes the title screen fail to load, with
`<file>:<line>: incorrect argument: <alignment>`. `y` is always the top
of the text.

A localized text may contain `\n` line breaks.

The text color is fixed by the title-screen implementation and is not
part of the description format.

A controls page can contain up to 8 `TEXT` directives.

## 6. Credits page

The credits page is described by a `CREDITS` block:

``` text
CREDITS
    ...
END_CREDITS
```

It supports `BACKGROUND`, `CREDIT` and `IMAGE` directives. The page is
opened by the `CREDITS` menu entry and closed with A or B.

### Background

Syntax:

``` text
BACKGROUND <image>
```

Example:

``` text
BACKGROUND credits_background
```

Defines the image drawn at `(0, 0)` on the bottom screen while the
credits page is open, behind its credit lines and images.

This directive is optional. If it is omitted, the page has no
background. If it appears several times, the last one is used.

### Credit line

Syntax:

``` text
CREDIT <role_key> <name>
```

Example:

``` text
CREDIT TITLE_CREDITS_PROGRAMMER Jane Doe
```

`role_key` is a localization key. `name` is literal, untranslated text
and extends to the end of the line, so it may contain spaces:

``` text
CREDIT TITLE_CREDITS_GRAPHICS Alex Martin & Sam Lee
```

Credit layout is fixed by the title-screen implementation: roles are
drawn on the left, names on the right, and successive lines are
vertically spaced automatically.

A credits page can contain up to 11 `CREDIT` directives.

### Image

Syntax:

``` text
IMAGE <image> <x> <y>
```

Example:

``` text
IMAGE publisher_logo 85 160
```

Draws an image from the `romfs:/game` sprite sheet at the specified
position on the bottom screen.

A credits page can contain up to 8 `IMAGE` directives.

## 7. Images

Every image referenced by the title description must exist in the
graphical asset set loaded from `romfs:/game`.

Images are referenced by name:

``` text
BACKGROUND_TOP background_top
BACKGROUND_BOTTOM background_bottom
BACKGROUND controls_background
IMAGE publisher_logo 85 160
```

Do not specify an image file extension.

## 8. Text and localization

Menu labels are associated with localization keys by the title-screen
implementation. The description controls which entries are displayed and
in which order; it does not redefine their labels.

The mapping is:

``` text
LANG     -> LANG_NAME
INTRO    -> TITLE_INTRO
CONTINUE -> TITLE_CONTINUE
GAME     -> TITLE_GAME
CONTROLS -> TITLE_CONTROLS
CREDITS  -> TITLE_CREDITS
```

`TEXT` directives in the `CONTROLS` block and role identifiers in
`CREDIT` directives are also localization keys.

Credit names are deliberately not localized.

Localization keys are resolved when the screen is drawn. Changing the
language from the `LANG` entry therefore updates the visible title text
immediately.

## 9. Complete example

The following example illustrates all supported directives:

``` text
BACKGROUND_TOP background_top
BACKGROUND_BOTTOM background_bottom

SFX_SELECT title_select
SFX_CHOICE title_choice
MUSIC title_music

# ---------------------------------------------------------------------------
# Menu
# ---------------------------------------------------------------------------

MENU
    ORDER LANG,INTRO,GAME,CONTROLS,CREDITS
    DEFAULT GAME
    INTRO intro
    POSITION 160 70
    SPACING 30
    TEXT_SIZE 0.65
    COLOR GRAY
    SELECTED_COLOR LIGHTGRAY
END_MENU

# ---------------------------------------------------------------------------
# Controls
# ---------------------------------------------------------------------------

CONTROLS
    BACKGROUND controls_background

    TEXT TITLE_CONTROLS_MOVE 36 15 0.55 CENTER
    TEXT TITLE_CONTROLS_INVENTORY 40 160 0.55 CENTER
    TEXT TITLE_CONTROLS_EXAMINE 281 15 0.55 CENTER
    TEXT TITLE_CONTROLS_USE 283 72 0.55 RIGHT
    TEXT TITLE_CONTROLS_CANCEL 280 135 0.55 CENTER
    TEXT TITLE_CONTROLS_ACTION 160 90 0.55 CENTER
    TEXT TITLE_CONTROLS_QUIT 160 210 0.55 CENTER
END_CONTROLS

# ---------------------------------------------------------------------------
# Credits
# ---------------------------------------------------------------------------

CREDITS
    BACKGROUND credits_background

    CREDIT TITLE_CREDITS_PROGRAMMER Jane Doe
    CREDIT TITLE_CREDITS_GAME_DESIGN John Smith
    CREDIT TITLE_CREDITS_GRAPHICS Alex Martin & Sam Lee
    CREDIT TITLE_CREDITS_SCENARIO Chris Taylor
    CREDIT TITLE_CREDITS_MUSIC Pat Brown
    CREDIT TITLE_CREDITS_TESTER Robin Davis

    IMAGE publisher_logo 85 160
END_CREDITS
```

The asset and localization names in this example illustrate the format;
they are not additional syntax.

## 10. Directives summary

| Directive           | Block      | Required | Repeatable          | Description                                              |
|---------------------|------------|----------|---------------------|----------------------------------------------------------|
| `BACKGROUND_TOP`    | —          | no       | —                   | Top screen background (section 2).                       |
| `BACKGROUND_BOTTOM` | —          | no       | —                   | Bottom screen background behind the menu (section 2).    |
| `SFX_SELECT`        | —          | no       | —                   | Sound played when the highlighted entry changes (section 3). |
| `SFX_CHOICE`        | —          | no       | —                   | Sound played when an entry is activated (section 3).     |
| `MUSIC`             | —          | no       | —                   | Title screen music (section 3).                          |
| `MENU`              | —          | yes      | —                   | Main menu block, closed by `END_MENU` (section 4).       |
| `ORDER`             | `MENU`     | yes      | yes (6 entries max) | Menu entries and their order.                            |
| `DEFAULT`           | `MENU`     | no       | —                   | Entry highlighted when the screen opens.                 |
| `INTRO`             | `MENU`     | no       | —                   | Timeline played by the `INTRO` entry.                    |
| `POSITION`          | `MENU`     | no       | —                   | Position of the first entry.                             |
| `SPACING`           | `MENU`     | no       | —                   | Vertical distance between two entries.                   |
| `TEXT_SIZE`         | `MENU`     | no       | —                   | Text size of the entries.                                |
| `COLOR`             | `MENU`     | no       | —                   | Color of the entries.                                    |
| `SELECTED_COLOR`    | `MENU`     | no       | —                   | Color of the highlighted entry.                          |
| `CONTROLS`          | —          | no       | —                   | Controls page block, closed by `END_CONTROLS` (section 5). |
| `BACKGROUND`        | `CONTROLS` | no       | —                   | Background of the controls page.                         |
| `IMAGE`             | `CONTROLS` | no       | yes (8)             | Image drawn on the controls page.                        |
| `TEXT`              | `CONTROLS` | no       | yes (8)             | Localized text on the controls page.                     |
| `CREDITS`           | —          | no       | —                   | Credits page block, closed by `END_CREDITS` (section 6). |
| `BACKGROUND`        | `CREDITS`  | no       | —                   | Background of the credits page.                          |
| `CREDIT`            | `CREDITS`  | no       | yes (11)            | One role / name line.                                    |
| `IMAGE`             | `CREDITS`  | no       | yes (8)             | Image drawn on the credits page.                         |

Notes:

- **Required.** The title screen fails to load when its menu is empty,
  so a `MENU` block with an `ORDER` listing at least one displayed entry
  is required. Everything else can be omitted: no `NONE`, `DISABLED` or
  `ENABLED` keyword exists. The `alignment` argument of `TEXT` is
  optional too.
- **Repeated directives.** When a directive marked `—` appears several
  times, the last value is used. Successive `ORDER` lines add their
  entries to the menu. Beyond the limit given in brackets, extra lines
  are ignored and logged; loading does not fail.
- **Missing argument.** A directive without its arguments makes the
  title screen fail to load.
- **Unknown directive.** A directive that is unknown, or placed outside
  its block, makes the title screen fail to load, with
  `<file>:<line>: unknown command: <directive>` in the logs.
- **Sub-pages.** A `CONTROLS` or `CREDITS` block is only useful when
  the corresponding entry is listed in `ORDER`.

## 11. Common mistakes

### Adding an image extension

Do not do this:

``` text
BACKGROUND_TOP background_top.png
```

Use the image name from the sprite sheet:

``` text
BACKGROUND_TOP background_top
```

### Adding spaces to ORDER

Do not write:

``` text
ORDER LANG, INTRO, GAME
```

`ORDER` is a single comma-separated token. Write:

``` text
ORDER LANG,INTRO,GAME
```

### Using a localization key as an ORDER entry

Do not write:

``` text
ORDER TITLE_INTRO,TITLE_GAME,TITLE_CREDITS
```

`ORDER` uses the title menu identifiers:

``` text
ORDER INTRO,GAME,CREDITS
```

### Localizing credit names

The first argument of `CREDIT` is localized; the rest of the line is a
literal name:

``` text
CREDIT TITLE_CREDITS_PROGRAMMER Jane Doe
```

### Misspelling a TEXT alignment

Do not write:

``` text
TEXT TITLE_CONTROLS_MOVE 36 15 0.55 CENTRE
```

Alignments are spelled in English and are case-sensitive. An unknown
alignment makes the title screen fail to load. Use `LEFT`, `CENTER` or
`RIGHT`.

### Trying to configure the version

The version position, size and color are intentionally hardcoded and are
not part of the title description file.

## 12. Recommended style

For readability, the title description should normally be ordered as:

``` text
Backgrounds
Sound effects and music
Menu
Controls
Credits
```

Use section comments and indent nested directives consistently. Keep
menu behavior in the game code and use the description only for
title-screen content, order and layout.

The title format is intentionally a small game-specific interface
description, not a general menu scripting language. If the title screen
requires behavior that cannot be expressed cleanly with the existing
directives, prefer adding one small, reusable primitive to the format.
