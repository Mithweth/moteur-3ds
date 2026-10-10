# Timeline description files

This document describes the declarative format used to define game
timelines.

A timeline is a scripted sequence of localized text, images, pauses,
music, sound effects and full-screen compositions. Timelines are used
for sequences such as the introduction, endings, game-over scenes and
cut-scenes played in the middle of the game.

The engine remains responsible for parsing the file and executing the
declared events in order.

## 1. Location and general structure

A timeline named `intro` is loaded from:

``` text
romfs:/timelines/intro/timeline
```

Its graphical assets are loaded from the corresponding timeline asset
set: the PNG files placed in the timeline directory. They are optional: a
timeline without any `IMAGE_LEFT`, `IMAGE_CENTER`, `IMAGE_RIGHT` or
`SPRITE` line (text, pauses and audio only) needs no image. If the
timeline uses one of these directives without any image available, it
is rejected at load time with an error naming the offending line.

A simple timeline can look like this:

``` text
MUSIC_START intro

IMAGE_LEFT scene1
TEXT RED TITLE_INTRO_SCENE_1
PAUSE 2000
END_SCENE

MUSIC_STOP
END
```

Blank lines are ignored. A line whose first non-whitespace character is
`#` is a comment. Comments should be written on their own line; inline
comments are not part of the format.

Tokens are whitespace-separated. Image names, localization IDs and
resource paths therefore do not contain spaces and are not quoted.

Events are executed sequentially in declaration order.

Every timeline must finish with `END` or `RETURN`:

``` text
END
```

- `END` terminates the timeline and goes back to the title screen. Use it
  for the introduction, endings and game overs.
- `RETURN` terminates the timeline and goes back to the game, in the room
  the timeline was started from. Use it for a cut-scene in the middle of
  the game. The room is left as it was, the game music restarts from the
  beginning and the rest of the action block that started the timeline
  runs, for example a `ROOM` change (see [ROOMS.en.md](./ROOMS.en.md)).
  After `END`, that rest is dropped. A timeline started from the title
  screen has no game to go back to: there, `RETURN` acts like `END`.

The file is read up to the first `END` or `RETURN`; anything after it is
ignored. Neither is the same as `END_SCENE`, which only clears the
current visual scene and allows the timeline to continue.

## 2. Standard scene layout

Outside a `FULL_SCREEN` block, a timeline uses the normal scene layout:

- the top screen contains up to three images: left, center and right;
- the bottom screen contains the timeline text.

The current images and text remain visible while subsequent events are
executed. They are not automatically cleared by `PAUSE`, audio events or
new text.

`END_SCENE` clears the current scene:

``` text
END_SCENE
```

It removes:

- the left, center and right images;
- the accumulated text;
- any active full-screen composition.

It does **not** stop the current music. Use `MUSIC_STOP` explicitly when
the music must end.

## 3. Text

Syntax:

``` text
TEXT <color> <message_id>
```

Example:

``` text
TEXT BLUE TITLE_INTRO_SCENE_1
```

`message_id` is a localization key. The displayed string is resolved
through the normal language system.

Supported colors are:

``` text
WHITE
RED
BLUE
YELLOW
GREEN
```

Text is displayed progressively, character by character.

Several `TEXT` events in the same scene are cumulative. A new `TEXT`
continues after the text already displayed; it does not clear the
previous text.

For example:

``` text
TEXT RED TITLE_INTRO_SCENE_3
IMAGE_CENTER scene4
PAUSE 100
TEXT RED TITLE_INTRO_SCENE_4
PAUSE 2000
END_SCENE
```

Both text events belong to the same scene. `END_SCENE` clears the text
before the next scene begins.

## 4. Standard images

Three directives control the image slots on the top screen:

``` text
IMAGE_LEFT <image>
IMAGE_CENTER <image>
IMAGE_RIGHT <image>
```

Example:

``` text
IMAGE_LEFT scene1
IMAGE_CENTER scene2
IMAGE_RIGHT scene3
```

`image` is the image name from the current timeline’s graphical asset
set. It corresponds to the PNG filename without the `.png` extension.
For example, `scene1` refers to `scene1.png`.

An image remains in its slot until it is replaced, explicitly removed,
or the scene ends.

Use `NONE` to clear one slot without clearing the rest of the scene:

``` text
IMAGE_CENTER NONE
```

For example:

``` text
IMAGE_LEFT scene10
IMAGE_CENTER scene11
IMAGE_RIGHT scene12
TEXT BLUE TITLE_INTRO_SCENE_16

IMAGE_CENTER NONE
TEXT BLUE TITLE_INTRO_SCENE_17

PAUSE 2000
END_SCENE
```

Only the center image disappears; the left image, right image and text
remain active.

## 5. Pauses

Syntax:

``` text
PAUSE <milliseconds>
```

Example:

``` text
PAUSE 2000
```

The timeline waits for the specified duration before continuing with the
next event.

A duration of `0` creates an indefinite pause:

``` text
PAUSE 0
```

An indefinite pause does not end automatically. The player must press
**A** to continue the timeline.

A pause does not clear or otherwise modify the current scene. Images,
text and full-screen compositions therefore remain visible while the
pause is active.

## 6. Music and sound effects

### MUSIC_START

Syntax:

``` text
MUSIC_START <path>
```

Example:

``` text
MUSIC_START intro
MUSIC_START romfs:/audio/ending.ogg
```

Starts the specified music and immediately continues with the next
timeline event.

The music can be specified either by a name relative to the timeline
directory or by an absolute `romfs:/` path. A relative name is given
without extension: it is resolved from the timeline directory and
automatically receives the `.ogg` extension (in the timeline `intro`,
`MUSIC_START intro` plays `romfs:/timelines/intro/intro.ogg`). An
absolute path is used exactly as written: no extension is added, so
write it in full, `.ogg` included (`MUSIC_START romfs:/audio/ending.ogg`).

The music continues independently of scene boundaries. `END_SCENE` does
not stop it.

### MUSIC_STOP

Syntax:

``` text
MUSIC_STOP
```

Stops the currently playing music and immediately continues.

Example:

``` text
MUSIC_STOP
FULL_SCREEN
    SPRITE bottom_background 0 240
    SPRITE top_background 0 0
END_FULL_SCREEN
```

### SFX

Syntax:

``` text
SFX <path>
```

Example:

``` text
SFX footsteps
SFX romfs:/audio/title_choice.raw
```

Starts a sound effect and immediately continues with the next event.
`SFX` does not wait for the sound to finish.

Sound effects can be specified either by a name relative to the timeline
directory or by an absolute `romfs:/` path. A relative name is given
without extension: it is resolved from the timeline directory and
automatically receives the `.raw` extension (in the timeline `intro`,
`SFX footsteps` plays `romfs:/timelines/intro/footsteps.raw`). An
absolute path is used exactly as written: no extension is added, so
write it in full, `.raw` included (`SFX romfs:/audio/title_choice.raw`).

Use a following `PAUSE` when the timeline must remain on the current
scene for a specific amount of time:

``` text
SFX footsteps
PAUSE 2000
```

## 7. Full-screen compositions

`FULL_SCREEN` describes a composition that can use both 3DS screens.

Syntax:

``` text
FULL_SCREEN
    SPRITE <image> <x> <y>
    [SPRITE <image> <x> <y> ...]
END_FULL_SCREEN
```

Example:

``` text
FULL_SCREEN
    SPRITE bottom_background 0 240
    SPRITE top_background 0 0
    SPRITE far_man 94 240
END_FULL_SCREEN

PAUSE 1500
```

A `FULL_SCREEN` composition supports up to **16 sprites** by default.

A full-screen composition uses a logical **320 × 480** coordinate space:

``` text
y =   0 .. 239   top screen
y = 240 .. 479   bottom screen
```

The top screen is physically 400 pixels wide. The 320-pixel logical
composition is centered on it, so timeline authors can use the same
320-pixel coordinate width for both screens.

For example:

``` text
SPRITE top_background    0   0
SPRITE bottom_background 0 240
```

places one background on each screen.

Sprites are declared from background to foreground. Later sprites are
drawn above earlier sprites.

### FULL_SCREEN is a complete composition

A `FULL_SCREEN` event replaces the previous full-screen composition.
Sprites are not inherited from the previous `FULL_SCREEN`.

This means persistent elements such as backgrounds must be repeated:

``` text
FULL_SCREEN
    SPRITE bottom_background 0 240
    SPRITE top_background 0 0
    SPRITE far_man 94 240
END_FULL_SCREEN
PAUSE 1500

FULL_SCREEN
    SPRITE bottom_background 0 240
    SPRITE top_background 0 0
    SPRITE close_man 144 264
END_FULL_SCREEN
PAUSE 1500
```

`END_FULL_SCREEN` only closes the `FULL_SCREEN` declaration in the
timeline file. It does **not** clear the composition from the display.
The composition remains visible until another `FULL_SCREEN` replaces it
or `END_SCENE` clears it.

Only `SPRITE` directives are valid between `FULL_SCREEN` and
`END_FULL_SCREEN`.

## 8. Scene boundaries

`END_SCENE` is the normal boundary between visual scenes:

``` text
TEXT BLUE TITLE_INTRO_SCENE_1
PAUSE 2000
END_SCENE

TEXT RED TITLE_INTRO_SCENE_2
PAUSE 1500
END_SCENE
```

Without `END_SCENE`, the second `TEXT` would continue in the same scene
and would be appended to the existing text.

The same rule applies to standard images: they remain present until
replaced, removed with `NONE`, or cleared by `END_SCENE`.

For full-screen compositions, `END_SCENE` also clears the active
composition and returns the display state to an empty standard scene.

Again, `END_SCENE` affects the visual scene only. Music continues until
`MUSIC_STOP` or until the timeline itself is closed.

## 9. Player controls

Timelines support two playback controls.

### A — next

Pressing **A** during a `TEXT` displays the rest of that text at once
and moves on to the next event. Pressing **A** during a `PAUSE` (timed or
indefinite) ends it.

This allows the player to advance dialogue without leaving a partially
displayed sentence on screen. A has no effect on the other events, which
take a single frame anyway.

### B — skip the timeline

Pressing **B** jumps straight to the final `END` or `RETURN`: the
timeline ends as if it had been played to the end. The events in
between are **not** run: a `MUSIC_START`, `SFX` or image that was not
reached yet is skipped too.

## 10. Available directives

| Directive                   | Effect                                                |
|-----------------------------|-------------------------------------------------------|
| `TEXT <color> <message_id>` | Displays localized text progressively.                |
| `PAUSE <milliseconds>`      | Waits before continuing.                              |
| `MUSIC_START <path>`        | Starts music and continues immediately.               |
| `MUSIC_STOP`                | Stops the current music.                              |
| `SFX <path>`                | Starts a sound effect and continues immediately.      |
| `IMAGE_LEFT <image>`        | Sets the left image on the top screen.                |
| `IMAGE_CENTER <image>`      | Sets the center image on the top screen.              |
| `IMAGE_RIGHT <image>`       | Sets the right image on the top screen.               |
| `IMAGE_* NONE`              | Clears the corresponding standard image slot.         |
| `FULL_SCREEN`               | Starts a full-screen composition declaration.         |
| `SPRITE <image> <x> <y>`    | Adds a sprite to the current full-screen composition. |
| `END_FULL_SCREEN`           | Ends the full-screen composition declaration.         |
| `END_SCENE`                 | Clears the current text and visual scene.             |
| `END`                       | Terminates the timeline, back to the title screen.    |
| `RETURN`                    | Terminates the timeline, back to the game.            |

## 11. Complete example

``` text
# Start the introduction music.

MUSIC_START intro

# Standard scene: text on the bottom screen.

TEXT BLUE TITLE_INTRO_SCENE_1
PAUSE 2000
END_SCENE

# Standard scene with three top-screen images.

IMAGE_LEFT scene1
TEXT RED TITLE_INTRO_SCENE_3

IMAGE_CENTER scene4
PAUSE 100
IMAGE_CENTER scene5
PAUSE 100
IMAGE_CENTER scene6

TEXT RED TITLE_INTRO_SCENE_4
PAUSE 2000
END_SCENE

# Stop the music and switch to full-screen compositions.

MUSIC_STOP
SFX footsteps

FULL_SCREEN
    SPRITE bottom_background 0 240
    SPRITE top_background 0 0
    SPRITE far_man 94 240
END_FULL_SCREEN
PAUSE 1500

FULL_SCREEN
    SPRITE bottom_background 0 240
    SPRITE top_background 0 0
    SPRITE close_man 144 264
END_FULL_SCREEN
PAUSE 1500

FULL_SCREEN
    SPRITE bottom_background 0 240
    SPRITE top_background 0 0
    SPRITE logo_top 15 62
    SPRITE logo_bottom 57 318
END_FULL_SCREEN
PAUSE 5000

END_SCENE
END
```

## 12. Execution model summary

The timeline format is deliberately sequential.

The main rules are:

1.  Events execute in declaration order.
2.  `TEXT` appends to the current scene until `END_SCENE`.
3.  Standard images persist until replaced, removed with `NONE`, or
    cleared by `END_SCENE`.
4.  `PAUSE` preserves the current display while waiting.
5.  Music is independent of scene boundaries.
6.  `SFX` does not wait for the sound to finish.
7.  A `FULL_SCREEN` block describes one complete composition.
8.  A full-screen composition remains visible after `END_FULL_SCREEN`.
9.  A later `FULL_SCREEN` replaces the previous composition.
10. `END_SCENE` clears the current visual state but continues the
    timeline.
11. `END` terminates the timeline and goes back to the title screen;
    `RETURN` terminates it and goes back to the game.

## 13. Common mistakes

### Forgetting END_SCENE between text scenes

Incorrect:

``` text
TEXT RED FIRST_SCENE
PAUSE 2000
TEXT BLUE SECOND_SCENE
```

`SECOND_SCENE` is appended to the text already displayed.

Correct:

``` text
TEXT RED FIRST_SCENE
PAUSE 2000
END_SCENE

TEXT BLUE SECOND_SCENE
```

### Expecting END_FULL_SCREEN to clear the display

Incorrect assumption:

``` text
FULL_SCREEN
    SPRITE background 0 0
END_FULL_SCREEN

PAUSE 2000
```

The background remains visible during the pause. `END_FULL_SCREEN`
closes the declaration; it is not a runtime clear operation.

Use `END_SCENE` when the composition must disappear.

### Forgetting persistent sprites in a new FULL_SCREEN

Each `FULL_SCREEN` is a complete composition.

If a background must remain visible while a character changes position,
repeat the background in every composition.

### Using SPRITE outside FULL_SCREEN

`SPRITE` belongs only inside:

``` text
FULL_SCREEN
    ...
END_FULL_SCREEN
```

### Expecting SFX to wait

This:

``` text
SFX footsteps
FULL_SCREEN
    ...
END_FULL_SCREEN
```

starts the sound and immediately proceeds to the full-screen event.

Add an explicit `PAUSE` when timing is required.

### Forgetting END or RETURN

Every valid timeline must terminate with `END` or `RETURN`; otherwise it
fails to load and the game goes back to the title screen.

### Ending a cut-scene with END

A cut-scene played in the middle of the game must end with `RETURN`.
With `END`, the player is sent back to the title screen and loses any
progress that was not saved.

## 14. Recommended style

Use blank lines to separate scenes and keep related visual and timing
events together.

For standard scenes, a readable order is usually:

``` text
images
text
pause
END_SCENE
```

For full-screen sequences, describe each frame as a complete composition
and place its timing immediately after it:

``` text
FULL_SCREEN
    sprites
END_FULL_SCREEN
PAUSE ...
```

Keep localization in the language files by using message IDs with
`TEXT`, rather than embedding displayed text directly in timeline
descriptors.

The timeline format is intentionally a small game-specific DSL. It
describes **what happens and in what order**; rendering, localization,
audio playback and input handling remain responsibilities of the engine.
