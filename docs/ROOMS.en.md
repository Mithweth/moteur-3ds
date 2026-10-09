# Room description files

This document describes the declarative format used to define game
rooms.

A room description defines the content and behavior of a room: displayed
images, hotspots, conditions, interactions, inventory use and exits.

## 1. Location and general structure

A room named `livingroom` is loaded from:

```text
romfs:/rooms/livingroom/room
```

Its graphical assets are loaded from the corresponding room asset set.

A typical room contains three sections:

```text
# Images

IMAGE bg 0 0 0
END_IMAGE

# Hotspots

HOTSPOT EXAMPLE_OBJECT 10 20 40 30
    MESSAGE EXAMPLE_OBJECT_EXAMINE
END_HOTSPOT

# Exits

PATH SOUTH
    ACTION
        WAIT_SFX door_open
        ROOM corridor
    END_ACTION
END_PATH
```

Blank lines are ignored. A line whose first non-whitespace character is
`#` is a comment. Comments should be written on their own line; inline
comments are not part of the format.

Tokens are separated by spaces. Identifiers such as state names, item
names, message IDs, room names, sound names and image names therefore do
not contain spaces and are not quoted.

Indentation is only for readability; block structure is determined by
the `END_*` directives.

## 2. Conditions

Conditions are introduced by `WHEN`:

```text
WHEN STATE_IS <state> <true|false>
WHEN INVENTORY_HAS <item> <true|false>
```

Examples:

```text
WHEN STATE_IS underground_dug true
WHEN INVENTORY_HAS SHOVEL false
```

Several `WHEN` directives in the same block are combined with logical
**AND**: every condition must match.

A condition applies to the block in which it appears. It can therefore
control an `IMAGE`, a `HOTSPOT`, an `ACTION`, a `USE` or a `PATH`.

State and inventory identifiers must correspond to identifiers known by
the game-state and inventory systems.

## 3. Images

Syntax:

```text
IMAGE <image> <x> <y> <z>
    [WHEN ...]
END_IMAGE
```

Example:

```text
IMAGE hole_dug 261 174 0.3
    WHEN STATE_IS underground_dug true
    WHEN STATE_IS underground_card_taken false
END_IMAGE
```

Do not specify an image file extension. Image names may contain letters,
digits and underscores (`_`); hyphens (`-`) must not be used.

`x` (from 0 to 320) and `y` (from 0 to 240) are the drawing coordinates.
`z` (from -1.0 to 1.0) controls the drawing depth.


The image is drawn only while all its conditions are true. With no
`WHEN`, the image is always drawn.

Conditional images are independent. If two image blocks have true
conditions, both are drawn. When only one variant should be visible, make
their conditions explicitly mutually exclusive:

```text
IMAGE full ...
    WHEN STATE_IS card_taken false
END_IMAGE

IMAGE empty ...
    WHEN STATE_IS card_taken true
END_IMAGE
```

## 4. Hotspots

Syntax:

```text
HOTSPOT <id> <x> <y> <width> <height>
    [WHEN ...]
    [MESSAGE <message_id>]
    [MESSAGE_IMAGE <image>]
    [ACTION ... END_ACTION]
    [USE ... END_USE]
END_HOTSPOT
```

Example:

```text
HOTSPOT UNDERGROUND_MAGNETIC_CARD 275 182 24 14
    WHEN STATE_IS underground_card_taken false
    WHEN STATE_IS underground_dug true
    ACTION
        SET underground_card_taken
        INVENTORY_ADD MAGNETIC_CARD
    END_ACTION
END_HOTSPOT
```

The rectangle is expressed as `x`, `y`, `width`, `height`.

Hotspot-level `WHEN` directives control whether the hotspot exists from
the player’s point of view. An inactive hotspot is ignored during hit
testing.

### Hotspot order matters

Hotspots are tested in declaration order. The first active hotspot whose
rectangle contains the selected point wins.

This is intentionally used for overlapping hotspots. A large generic
hotspot should therefore normally be declared **after** smaller, more
specific hotspots that overlap it.

For example:

```text
HOTSPOT UNDERGROUND_X_FORM 141 190 17 16
    ...
END_HOTSPOT

HOTSPOT UNDERGROUND_GROUND 45 171 275 68
    ...
END_HOTSPOT
```

Putting the large ground hotspot first would hide the smaller X-shaped
hotspot.

## 5. Examine messages

A `MESSAGE` directly inside a `HOTSPOT`, outside an `ACTION` or `USE`,
defines the message shown when the object is examined:

```text
HOTSPOT LIVINGROOM_FIREPLACE 163 72 26 17
    MESSAGE LIVINGROOM_FIREPLACE_EXAMINE
END_HOTSPOT
```

This is different from `MESSAGE` used as an action:

```text
ACTION
    MESSAGE CELLAR_DISABLE_ALARM_BOX
END_ACTION
```

In that case the message is displayed when the action block executes.

A `MESSAGE_IMAGE` directly inside a `HOTSPOT` works the same way, but
shows an image from the room's assets, centered over the room, instead
of a text:

```text
HOTSPOT STUDY_PAINTING 120 40 60 45
    MESSAGE_IMAGE painting_closeup
END_HOTSPOT
```

If a hotspot defines both `MESSAGE` and `MESSAGE_IMAGE`, `MESSAGE` takes
precedence and the image is never shown.

A hotspot has at most one `MESSAGE` and one `MESSAGE_IMAGE` outside its
`ACTION` and `USE` blocks: a second one makes the room fail to load with
`duplicate MESSAGE in HOTSPOT <id>` (or `duplicate MESSAGE_IMAGE`).

## 6. ACTION blocks

An `ACTION` block describes what happens when the player performs the
normal action on a hotspot:

```text
HOTSPOT UNDERGROUND_SHOVEL 18 89 37 100
    WHEN INVENTORY_HAS SHOVEL false
    ACTION
        INVENTORY_ADD SHOVEL
    END_ACTION
END_HOTSPOT
```

An `ACTION` may itself have conditions:

```text
ACTION
    WHEN STATE_IS cellar_alarm_box_unscrewed true
    SET cellar_alarm_box_opened
END_ACTION
```

### Multiple ACTION blocks are sequential

`ACTION` blocks are **not** `if / else if` alternatives.

All matching action blocks are evaluated in declaration order. Their
conditions are re-evaluated when each block is reached, so an earlier
block may change game state and thereby affect a later block.

This is intentional and is useful for state transitions:

```text
ACTION
    SET diningroom_lasers_disabled
END_ACTION

ACTION
    WHEN STATE_IS diningroom_lasers_disabled true
    MESSAGE CELLAR_DISABLE_ALARM_BOX
END_ACTION

ACTION
    WHEN STATE_IS diningroom_lasers_disabled false
    MESSAGE CELLAR_ENABLE_ALARM_BOX
END_ACTION
```

If `diningroom_lasers_disabled` is a toggle state, the first block
changes it. The following blocks then inspect the **new** value.

This is different from `USE` blocks: matching `ACTION` blocks do not stop
after the first match.

## 7. USE blocks

`USE` describes the use of an inventory item on a hotspot.

Syntax:

```text
USE <item>
    [WHEN ...]
    <actions>
END_USE
```

Example:

```text
USE SCREWDRIVER
    WHEN STATE_IS cellar_alarm_box_unscrewed false
    SET cellar_alarm_box_unscrewed
    MESSAGE CELLAR_OPEN_ALARM_BOX
END_USE
```

A `USE` block directly contains actions. There is **no nested `ACTION`
block** inside `USE`.

Incorrect:

```text
USE SCREWDRIVER
    ACTION
        SET cellar_alarm_box_unscrewed
    END_ACTION
END_USE
```

Correct:

```text
USE SCREWDRIVER
    SET cellar_alarm_box_unscrewed
END_USE
```

This distinction is important because `ACTION` belongs to the enclosing
hotspot/path grammar, not to `USE`.

### Multiple USE blocks

Several `USE` blocks may refer to the same item and use conditions to
select the appropriate behavior:

```text
USE SCREWDRIVER
    WHEN STATE_IS cellar_alarm_box_unscrewed false
    SET cellar_alarm_box_unscrewed
    MESSAGE CELLAR_OPEN_ALARM_BOX
END_USE

USE SCREWDRIVER
    WHEN STATE_IS cellar_alarm_box_unscrewed true
    MESSAGE CELLAR_ALARM_BOX_ALREADY_OPENED
END_USE
```

Unlike hotspot `ACTION` blocks, `USE` blocks are alternatives: the first
block matching the item and all its conditions is executed, then
item-use processing stops.

### Wildcard USE

`USE *` matches any inventory item:

```text
HOTSPOT STUDY_DARK 0 0 320 240
    WHEN STATE_IS study_lights_on false
    MESSAGE STUDY_MESSAGE_NO_LIGHT
    USE *
        MESSAGE STUDY_USE_NO_LIGHT
    END_USE
END_HOTSPOT
```

Because `USE` blocks are checked in declaration order, a wildcard should
be placed after more specific item handlers when both are present.

## 8. Paths and exits

A `PATH` declares an available movement direction.

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

Basic example:

```text
PATH EAST
    ACTION
        ROOM secondunderground
    END_ACTION
END_PATH
```

A path can have conditions controlling whether the direction is
available:

```text
PATH NORTH
    WHEN STATE_IS livingroom_secret_passage_opened true

    ACTION
        WHEN STATE_IS livingroom_rope_in_hearth_bound false
        WAIT_SFX falling_down
        TIMELINE gameover_falldown
    END_ACTION

    ACTION
        WHEN STATE_IS livingroom_rope_in_hearth_bound true
        WAIT_SFX rope_climbing
        ROOM cryoroom
    END_ACTION
END_PATH
```

Path-level conditions determine whether the player can use the exit at
all.

Conditions inside a path’s `ACTION` blocks select what happens after the
path is used.

Path `ACTION` blocks follow the same sequential semantics as hotspot
`ACTION` blocks.

## 9. Available actions

Actions are valid inside `ACTION` and `USE` blocks.

| Directive                 | Effect                                                                                                    |
|---------------------------|-----------------------------------------------------------------------------------------------------------|
| `SET <state>`             | Updates the state according to its declared type. For `TOGGLE`, it flips the current value.               |
| `INVENTORY_ADD <item>`    | Adds an item to the inventory.                                                                            |
| `INVENTORY_REMOVE <item>` | Removes an item from the inventory.                                                                       |
| `MESSAGE <message_id>`    | Displays a localized game message.                                                                        |
| `MESSAGE_IMAGE <image>`   | Displays an image from the room's assets, centered over the room.                                         |
| `SFX <name>`              | Starts a sound effect and continues immediately.                                                          |
| `WAIT_SFX <name>`         | Starts a sound effect and pauses execution until it finishes.                                             |
| `ROOM <room>`             | Changes to another room. **Terminates the current action flow and must be the last action in its block.** |
| `TIMELINE <name>`         | Starts a timeline and suspends execution until it ends. After `RETURN`, the rest of the block runs; after `END`, it is dropped. |
| `MINIGAME <name>`         | Starts a minigame. **Terminates the current action flow and must be the last action in its block.**       |

Sound effects can be specified either by a name relative to the room directory
or by an absolute `romfs:/` path:

```text
SFX closet_open
WAIT_SFX metal_ladder
SFX romfs:/audio/title_choice.raw
```

A relative name is given **without extension**: it is resolved from the
room directory and automatically receives the `.raw` extension. In the
room `hall`, `SFX closet_open` plays `romfs:/rooms/hall/closet_open.raw`.
An absolute path, starting with `romfs:/`, is used exactly as written:
no extension is added, so write it in full, `.raw` included
(`SFX romfs:/audio/title_choice.raw`). Use it to share a sound between
rooms.

### SET does not necessarily mean “set to true”

The effect of `SET` depends on the type of the state.

For a state declared as `TOGGLE`, `SET` **inverts its current value**:

- `false` becomes `true`;
- `true` becomes `false`.

For example:

```text
ACTION
    SET livingroom_piano_opened
END_ACTION
```

opens a closed piano if `livingroom_piano_opened` is currently `false`,
and closes it if the state is currently `true`.

Do not interpret `SET foo` as equivalent to `foo = true` without
checking the state definition.

## 10. Sound effects and action flow

`SFX` starts a sound effect and immediately continues with the next
action:

```text
SFX closet_open
SET closet_opened
```

`WAIT_SFX` starts a sound effect and waits for it to finish before
continuing with the next action **in the same block**:

```text
ACTION
    WAIT_SFX metal_ladder
    ROOM cellar
END_ACTION
```

Do not rely on later `ACTION` blocks to execute after a `WAIT_SFX`. Any
actions that must follow the sound should be placed after `WAIT_SFX` in
the same block.

### Actions after a timeline

`TIMELINE` suspends the block the same way as `WAIT_SFX`, until the
timeline ends:

```text
ACTION
    SET cellar_door_opened
    TIMELINE cellar_discovery
    ROOM cellar
END_ACTION
```

- If the timeline ends with `RETURN`, the player is back in the room and
  the rest of the block runs: here, the player is moved to `cellar`.
- If it ends with `END`, the game goes back to the title screen and the
  rest of the block is dropped.

Pressing **B** to skip the timeline still reaches its final `RETURN` or
`END`, so the outcome is the same. As with `WAIT_SFX`, later `ACTION`
blocks are not executed after the timeline. Actions placed **before**
`TIMELINE` take effect immediately, but the player only sees their
result once back in the room.

### Terminal actions

`ROOM` and `MINIGAME` terminate the current action flow.
They must therefore always be the **last action in their block**.

Correct:

```text
ACTION
    WAIT_SFX door_open
    ROOM corridor
END_ACTION
```

Incorrect:

```text
ACTION
    ROOM corridor
    MESSAGE UNREACHABLE_MESSAGE
END_ACTION
```

The `MESSAGE` will never be executed because `ROOM` terminates the
action flow.

Do not place `WAIT_SFX`, `ROOM`, `TIMELINE` or `MINIGAME` after a
`MESSAGE` in the same block. The following action immediately replaces the
message state, so the message will not be displayed.

## 11. Complete example

```text
# ---------------------------------------------------------------------------
# Images
# ---------------------------------------------------------------------------

IMAGE bg 0 0 0
END_IMAGE

IMAGE hole_dug 261 174 0.3
    WHEN STATE_IS underground_dug true
    WHEN STATE_IS underground_card_taken false
END_IMAGE

IMAGE hole_empty 265 175 0.3
    WHEN STATE_IS underground_dug true
    WHEN STATE_IS underground_card_taken true
END_IMAGE

# ---------------------------------------------------------------------------
# Hotspots
# ---------------------------------------------------------------------------

HOTSPOT UNDERGROUND_MAGNETIC_CARD 275 182 24 14
    WHEN STATE_IS underground_card_taken false
    WHEN STATE_IS underground_dug true
    ACTION
        SET underground_card_taken
        INVENTORY_ADD MAGNETIC_CARD
    END_ACTION
END_HOTSPOT

HOTSPOT UNDERGROUND_GROUND 284 176 12 12
    WHEN STATE_IS underground_dug false
    USE SHOVEL
        SET underground_dug
        MESSAGE UNDERGROUND_DIG
    END_USE
END_HOTSPOT

HOTSPOT UNDERGROUND_X_FORM 141 190 17 16
    MESSAGE UNDERGROUND_X_FORM_EXAMINE
    USE MEASURING_TAPE
        MINIGAME measure
    END_USE
END_HOTSPOT

# Generic hotspot deliberately declared after the more specific hotspots.
HOTSPOT UNDERGROUND_GROUND 45 171 275 68
    USE SHOVEL
        MESSAGE UNDERGROUND_DIG_ANYWHERE
    END_USE
END_HOTSPOT

# ---------------------------------------------------------------------------
# Exits
# ---------------------------------------------------------------------------

PATH EAST
    ACTION
        ROOM secondunderground
    END_ACTION
END_PATH

PATH NORTH
    ACTION
        WAIT_SFX metal_ladder
        ROOM cellar
    END_ACTION
END_PATH
```

## 12. Execution model summary

When a room is active:

1.  Images whose conditions match are drawn.
2.  Hotspot hit testing scans active hotspots in declaration order and
    selects the first matching rectangle.
3.  Examining a hotspot uses its direct `MESSAGE`, if any, otherwise
    its direct `MESSAGE_IMAGE`, if any.
4.  Normal hotspot actions scan all `ACTION` blocks in declaration
    order. Every matching block executes unless the action flow is
    suspended by `WAIT_SFX` or `TIMELINE`, or terminated by `ROOM` or
    `MINIGAME`.
5.  Item use scans `USE` blocks in declaration order and executes the
    first matching item/condition block.
6.  A path exists only when its path-level conditions match; using it
    evaluates its `ACTION` blocks in declaration order.

Room files are expected to be authored correctly. Syntax and runtime
errors are reported through the normal debug output.

## 13. Common mistakes

### Nesting ACTION inside USE

Do not do this:

```text
USE KEY_ONE
    ACTION
        MESSAGE SOMETHING
    END_ACTION
END_USE
```

Actions belong directly in the `USE` block.

### Treating ACTION blocks as else-if

Multiple matching `ACTION` blocks can run. State changes in one block
can affect conditions in following blocks.

### Forgetting hotspot priority

A large hotspot declared before a smaller overlapping hotspot can make
the smaller one unreachable.

### Forgetting that conditional images are independent

Images are evaluated independently. Add complementary conditions when
only one variant must be visible.

### Assuming SET forces a boolean to true

Check the state definition. On a `TOGGLE` state, `SET` inverts the
current value: `false` becomes `true`, and `true` becomes `false`.

### Splitting WAIT_SFX and its continuation into different ACTION blocks

Actions that must occur after a `WAIT_SFX` belong in the same block:

```text
ACTION
    WAIT_SFX door_open
    ROOM hall
END_ACTION
```

### Placing actions after ROOM or MINIGAME

`ROOM` and `MINIGAME` terminate the current action flow. Nothing should
follow them in the same block.

### Expecting actions after TIMELINE to always run

Actions after `TIMELINE` only run if the timeline ends with `RETURN`.
After a game over ending with `END`, they are dropped.

### Giving a hotspot two examine messages

```text
HOTSPOT HALL_CLOSET 40 60 30 50
    MESSAGE HALL_CLOSET_EXAMINE
    MESSAGE HALL_CLOSET_EXAMINE_AGAIN
END_HOTSPOT
```

The room fails to load with `duplicate MESSAGE in HOTSPOT HALL_CLOSET`.
A hotspot has a single examine message; to show different messages
depending on the game state, use conditional `ACTION` blocks. This
typically happens when `scripts/add_events.sh` is run twice on the same
file.

## 14. Recommended style

For readability, room files should normally be ordered as:

```text
Images
Hotspots
Exits
```

Use section comments and indent nested directives consistently. Keep
specific overlapping hotspots before generic ones. Keep related
`WAIT_SFX` and transition actions together.

The room format is intentionally a small game-specific DSL, not a
general scripting language. If a room requires behavior that cannot be
expressed cleanly with the existing primitives, prefer adding one small,
reusable primitive to the format.
