# Playing a MOTEUR game

This document describes the **gameplay conventions shared by MOTEUR
games**.

MOTEUR provides the mechanisms required by an adventure — rooms,
interactions, inventory, items, messages, timelines, and minigames —
while each game remains free to define its own content, interface
layout, additional menus, indicators, and game-specific commands.

For installation and build instructions, see the [project
README](../README.md) and [BUILD.en.md](./BUILD.en.md).

## 1. Common controls

The following controls are common to MOTEUR games:

| Control        | Action                    |
|:---------------|:--------------------------|
| **Circle Pad** | Move between rooms        |
| **D-Pad**      | Browse the inventory      |
| **A**          | Use the selected item     |
| **X**          | Examine the selected item |
| **B**          | Cancel / close / go back  |

A game may add other controls for its own menus, interactions, or
mechanics.

## 2. Inventory

A MOTEUR game can provide an inventory containing items collected during
the adventure.

When an item is selected:

- **X** examines it, when the item provides an examine action;
- **A** uses it.

Depending on the item and the game, using it may trigger an action
directly or apply it to a **target** selected in the current room.

An item may disappear from the inventory after use if it has been
placed, given away, consumed, or otherwise used by the adventure.

## 3. Examining an item

Press **X** to examine the currently selected item.

Examining an item may display text or an image, or trigger game-specific
behaviour. Not every item necessarily provides a detailed view or a
special examine action.

Press **B** to close the examination view or return to the game.

## 4. Using an item

Press **A** to use the currently selected item.

MOTEUR supports two kinds of item use:

- an item may provide its own action and be used directly;
- an item may be used on a **target** in the current room.

How a target is selected depends on the game's interface. If an
item/target combination is not supported, the game may simply reject it
or display a message.

## 5. Messages and timelines

Adventures can display messages, images, and scripted timelines.

**B** is generally used to close, cancel, or leave the current screen
when that operation is available.

The exact behaviour of a timeline — whether it can be accelerated,
skipped, or return to the game — is defined by the adventure.

## 6. Minigames

MOTEUR allows a game to provide its own minigames written in C.

Their controls are therefore game-specific. When a minigame can be left,
**B** keeps its usual role as the back or cancel button.

## 7. Movement, inventory, and interface

Use the **Circle Pad** to move between rooms. Movement is only possible
in a direction available from the current room.

Use the **D-Pad** to browse the inventory and select an item. Press
**A** to use the selected item and **X** to examine it.

The adventure remains free to choose how this information is displayed
and may add its own menus, indicators, or additional controls. Saving,
for example, is optional and its presentation depends on the game.

## 8. Saving

When an adventure enables saving, MOTEUR uses **a single save slot**.

The saved game can be resumed with **Continue** from the title screen.
Saving again replaces the previous save.

**Starting a New Game deletes the existing save.** It is therefore not
possible to keep several saved games or return to an older save after
choosing to start over.

## 9. Summary

| Control        | Common function           |
|:---------------|:--------------------------|
| **Circle Pad** | Move between rooms        |
| **D-Pad**      | Browse the inventory      |
| **A**          | Use the selected item     |
| **X**          | Examine the selected item |
| **B**          | Cancel / close / go back  |

Additional controls and their functions depend on the adventure.
