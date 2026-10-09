# MOTEUR

> [English](#english) · [Français](#français)

------------------------------------------------------------------------

## English

### The project

**MOTEUR — Modular Open Text-driven Engine for User-defined Rooms** is a
small, data-driven adventure game engine for the Nintendo 3DS.

Its goal is to make it possible to build point-and-click adventures
while keeping game content largely separate from the C engine. Rooms,
images, hotspots, paths, conditions, interactions, inventory objects,
scripted sequences, HUD layout and title-screen behaviour can be
described in compact text files stored in `resources/` and packed into
the RomFS.

MOTEUR is deliberately designed around the Nintendo 3DS rather than as a
portable, general-purpose game engine. The console’s two screens, touch
screen, physical controls and modest hardware are treated as part of the
design rather than limitations to abstract away.

The project has several goals:

- keep the engine small, understandable and easy to modify;
- let an adventure be built mostly from declarative resource files
  rather than hard-coded C;
- provide reusable mechanics for rooms, hotspots, inventory, state,
  messages, audio, timelines and navigation;
- support localization through separate `.lang` files;
- allow game-specific behaviour to be added in C when the data format is
  not the right tool, notably through callbacks and mini-game
  extensions;
- remain simple enough that the resource format describes an adventure
  game without turning into a general-purpose scripting language.

A small sample adventure is included to demonstrate the engine’s
features and provide a starting point for new projects.

### Built with devkitPro

MOTEUR is written in **C** using the open-source Nintendo 3DS homebrew
toolchain from [devkitPro](https://devkitpro.org/):

- **devkitARM** provides the ARM cross-compilation toolchain;
- **libctru** provides access to 3DS system services, input, RomFS and
  NDSP audio;
- **Citro2D / Citro3D** provide hardware-accelerated rendering through
  the 3DS GPU;
- **Tremor** (`libvorbisidec`) provides integer Ogg Vorbis decoding for
  music playback.

The build can produce both a `.3dsx`, which can be launched from the
Homebrew Launcher, and a `.cia`, which can be installed as a title with
its own HOME Menu icon and banner.

📖 Full build instructions: [docs/BUILD.en.md](./docs/BUILD.en.md)

### How MOTEUR works

MOTEUR separates reusable engine code from the adventure itself.

The C code implements the runtime: rendering, input, rooms, inventory,
game state, audio, timelines, localization, save support and the
interfaces used by extensions. Adventure-specific content lives
primarily in `resources/`, where text files describe rooms and their
interactions.

A room can define its background and additional images, interactive
hotspots, exits, conditional content and sequences of actions. Actions
can display messages, manipulate game state and inventory, play sounds,
move to another room, start timelines and invoke other engine features.
Conditions allow the same room to react to what the player has already
done.

Timelines provide data-driven scripted sequences. They can be used for
introductions, endings and other sequences, as well as in-game cutscenes
that return control to the action flow that launched them.

The engine deliberately does not try to express everything in its text
formats. Behaviours that genuinely benefit from code can be implemented
as C extensions, including inventory callbacks and mini-games. This
keeps the common adventure logic declarative without forcing complex
mechanics into an oversized scripting language.

Detailed documentation for the data formats and extension interfaces is
here: [docs/DEVELOPING.en.md](./docs/DEVELOPING.en.md)

### Creating an adventure

A MOTEUR project is built around two complementary parts:

- `resources/` contains the declarative game data and assets that are
  packed into RomFS;
- C extensions implement behaviours that need native code.

A typical room definition combines images, hotspots, conditions and
actions. Global game state and inventory state can then make those
definitions evolve as the player progresses. Text is kept separately in
`.lang` files so that game logic does not need to be duplicated for each
language.

The included sample is intended both as a functional demonstration and
as a compact reference: it shows how rooms connect, how objects are
collected and used, how state changes affect the world, how timelines
interrupt and resume action flows, and how C callbacks can extend the
declarative layer.

### Controls

The default adventure controls are:

| Control      | Action                          |
|:-------------|:--------------------------------|
| Circle Pad   | Move between rooms              |
| Touch screen | Interact with your surroundings |
| D-Pad        | Browse the inventory            |
| A            | Use the selected item           |
| X            | Examine the selected item       |
| B            | Cancel / close                  |
| START        | Quit                            |

The HUD, available interactions and exact behaviour remain part of the
adventure configuration and can be adapted by a project using MOTEUR.

### Use of AI (LLMs)

In the interest of transparency: **large language models (LLMs) have
been used during the development of MOTEUR**, specifically to:

- **generate graphical assets** used by the sample adventure;
- **perform code reviews** of the engine and extensions.

------------------------------------------------------------------------

## Français

### Le projet

**MOTEUR — Modular Open Text-driven Engine for User-defined Rooms** est
un petit moteur de jeu d’aventure pour Nintendo 3DS, **piloté par les
données**.

Son objectif est de permettre de créer des point-and-click en séparant
autant que possible le contenu du jeu du moteur écrit en C. Les pièces,
images, zones interactives, chemins, conditions, interactions, objets
d’inventaire, séquences scriptées, disposition du HUD et comportement de
l’écran titre peuvent être décrits dans de petits fichiers texte rangés
dans `resources/` puis embarqués dans la RomFS.

MOTEUR est volontairement conçu autour de la Nintendo 3DS plutôt que
comme un moteur portable et généraliste. Les deux écrans de la console,
son écran tactile, ses contrôles physiques et ses contraintes
matérielles font partie du design au lieu d’être des particularités à
masquer derrière une couche d’abstraction.

Le projet poursuit plusieurs objectifs :

- garder un moteur petit, compréhensible et facile à modifier ;
- permettre de construire l’essentiel d’une aventure à partir de
  fichiers de ressources déclaratifs plutôt que de code C écrit en dur ;
- fournir des mécanismes réutilisables pour les pièces, zones
  interactives, inventaire, états, messages, audio, timelines et
  déplacements ;
- permettre la localisation grâce à des fichiers `.lang` séparés ;
- permettre d’ajouter en C les comportements propres à un jeu lorsque le
  format de données n’est pas l’outil adapté, notamment grâce aux
  callbacks et aux extensions de mini-jeux ;
- rester suffisamment simple pour que le format de ressources décrive un
  jeu d’aventure sans se transformer en langage de script généraliste.

Une petite aventure d’exemple est incluse afin de montrer les
fonctionnalités du moteur et de servir de point de départ à de nouveaux
projets.

### Développé avec devkitPro

MOTEUR est écrit en **C** avec la chaîne d’outils homebrew open source
pour Nintendo 3DS de [devkitPro](https://devkitpro.org/) :

- **devkitARM** fournit la chaîne de compilation croisée ARM ;
- **libctru** donne accès aux services système de la 3DS, aux contrôles,
  à la RomFS et à l’audio NDSP ;
- **Citro2D / Citro3D** assurent le rendu accéléré par le GPU de la 3DS
  ;
- **Tremor** (`libvorbisidec`) assure le décodage Ogg Vorbis en
  arithmétique entière pour la musique.

La compilation peut produire à la fois un `.3dsx`, lançable depuis le
Homebrew Launcher, et un `.cia`, installable comme un titre avec sa
propre icône et sa bannière dans le menu HOME.

📖 Instructions de compilation complètes :
[docs/BUILD.fr.md](./docs/BUILD.fr.md)

### Fonctionnement de MOTEUR

MOTEUR sépare le code réutilisable du moteur du contenu propre à
l’aventure.

Le code C fournit l’environnement d’exécution : rendu, contrôles,
pièces, inventaire, état du jeu, audio, timelines, localisation,
sauvegarde et interfaces utilisées par les extensions. Le contenu de
l’aventure se trouve principalement dans `resources/`, où des fichiers
texte décrivent les pièces et leurs interactions.

Une pièce peut définir son décor et ses images supplémentaires, ses
zones interactives, ses sorties, son contenu conditionnel et ses
séquences d’actions. Les actions peuvent afficher des messages, modifier
l’état du jeu et l’inventaire, jouer des sons, changer de pièce, lancer
des timelines et utiliser d’autres fonctionnalités du moteur. Les
conditions permettent à une même pièce de réagir à ce que le joueur a
déjà accompli.

Les timelines fournissent des séquences scriptées pilotées par les
données. Elles peuvent servir aux introductions, aux fins et à d’autres
séquences, mais également à des cinématiques en cours de jeu qui rendent
ensuite la main au flux d’actions qui les a lancées.

Le moteur ne cherche volontairement pas à tout exprimer dans ses formats
texte. Les comportements qui bénéficient réellement de code peuvent être
implémentés sous forme d’extensions C, notamment les callbacks
d’inventaire et les mini-jeux. La logique courante d’un jeu d’aventure
reste ainsi déclarative sans forcer les mécaniques complexes dans un
langage de script démesuré.

La documentation détaillée des formats et des interfaces d’extension se
trouve ici : [docs/DEVELOPING.fr.md](./docs/DEVELOPING.fr.md)

### Créer une aventure

Un projet MOTEUR repose sur deux parties complémentaires :

- `resources/` contient les données déclaratives du jeu et les
  ressources embarquées dans la RomFS ;
- les extensions C implémentent les comportements qui nécessitent du
  code natif.

Une définition de pièce typique combine images, zones interactives,
conditions et actions. L’état global du jeu et celui de l’inventaire
permettent ensuite à ces définitions d’évoluer au fil de la progression
du joueur. Les textes sont conservés séparément dans des fichiers
`.lang`, afin de ne pas dupliquer la logique du jeu pour chaque langue.

L’exemple fourni est à la fois une démonstration fonctionnelle et une
référence compacte : il montre comment relier des pièces, ramasser et
utiliser des objets, faire évoluer le monde en fonction de l’état,
interrompre puis reprendre un flux d’actions avec une timeline et
étendre la couche déclarative grâce aux callbacks C.

### Commandes

Les commandes par défaut d’une aventure sont :

| Commande             | Action                       |
|:---------------------|:-----------------------------|
| Stick circulaire     | Se déplacer entre les pièces |
| Écran tactile        | Interagir avec le décor      |
| Croix directionnelle | Parcourir l’inventaire       |
| A                    | Utiliser l’objet sélectionné |
| X                    | Examiner l’objet sélectionné |
| B                    | Annuler / fermer             |
| START                | Quitter                      |

Le HUD, les interactions disponibles et leur comportement précis font
partie de la configuration de l’aventure et peuvent être adaptés par un
projet utilisant MOTEUR.

### Utilisation de l’IA (LLM)

Par souci de transparence : **des grands modèles de langage (LLM) ont
été utilisés pendant le développement de MOTEUR**, plus précisément pour
:

- **générer les ressources graphiques** utilisées par l’aventure
  d’exemple ;
- **réaliser les revues de code** du moteur et des extensions.
