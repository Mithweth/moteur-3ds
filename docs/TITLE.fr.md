# Fichier de description de l'écran titre

Ce document décrit le format déclaratif utilisé pour définir l'écran
titre du jeu.

Une description de l'écran titre définit les arrière-plans des écrans
supérieur et inférieur, la disposition et l'ordre du menu, les effets
sonores et la musique de l'écran titre, la page des contrôles et la page des crédits.
Le comportement du menu lui-même reste implémenté par le jeu.

## 1. Emplacement et structure générale

La description de l'écran titre est chargée depuis :

``` text
romfs:/game/title
```

Ses ressources graphiques sont chargées depuis l'ensemble de ressources
situé dans :

``` text
romfs:/game
```

L'écran supérieur utilise un espace de coordonnées de 400 × 240. Le
menu, les contrôles et les crédits sont affichés sur l'écran inférieur
de 320 × 240.

Une description d'écran titre contient généralement la structure
suivante :

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

`MENU` définit le menu principal et son ordre. `CONTROLS` et `CREDITS`
définissent les deux sous-pages optionnelles ouvertes par les entrées
correspondantes du menu.

Les lignes vides sont ignorées. Une ligne dont le premier caractère non
blanc est `#` est un commentaire. Les commentaires doivent être placés
sur leur propre ligne ; les commentaires en fin de ligne ne font pas
partie du format.

Les tokens sont séparés par des espaces. Les identifiants tels que les
noms d'images, les clés de localisation et les noms de sons ne
contiennent donc pas d'espaces et ne sont pas placés entre guillemets.
L'exception est le nom de la personne dans une directive `CREDIT` : tout
ce qui suit la clé du rôle fait partie du nom et peut contenir des
espaces.

L'indentation ne sert qu'à améliorer la lisibilité ; la structure des
blocs est déterminée par les directives `END_*`.

`BACKGROUND_TOP`, `BACKGROUND_BOTTOM`, `SFX_SELECT`, `SFX_CHOICE` et
`MUSIC` se placent en dehors de tout bloc, avant ou après eux. Dans un
bloc `MENU`, `CONTROLS` ou `CREDITS`, ce sont des directives inconnues
qui font échouer le chargement.

## 2. Arrière-plans

Syntaxe :

``` text
BACKGROUND_TOP <image>
BACKGROUND_BOTTOM <image>
```

Exemple :

``` text
BACKGROUND_TOP background_top
BACKGROUND_BOTTOM background_bottom
```

`BACKGROUND_TOP` définit l'image affichée en `(0, 0)` sur l'écran
supérieur.

`BACKGROUND_BOTTOM` définit l'image affichée en `(0, 0)` derrière le
menu principal sur l'écran inférieur. Elle n'est pas affichée sur les
pages des contrôles ou des crédits, qui ont leur propre directive
`BACKGROUND` (voir les sections 5 et 6).

Les deux directives sont optionnelles. Si un arrière-plan est omis,
aucune image n'est affichée pour cet écran et le fond reste noir.

Les noms d'images font référence aux images de l'ensemble de ressources
graphiques de `romfs:/game`. Ne pas spécifier l'extension du fichier
image.

## 3. Effets sonores et musique

Syntaxe :

``` text
SFX_SELECT <sound>
SFX_CHOICE <sound>
```

Exemple :

``` text
SFX_SELECT title_select
SFX_CHOICE title_choice
```

`SFX_SELECT` est joué lorsque l'entrée sélectionnée dans le menu change.

`SFX_CHOICE` est joué lorsqu'une entrée du menu est activée ainsi que
lorsque la page des contrôles ou des crédits est fermée avec A ou B.

Les deux directives sont optionnelles.

Un nom de son relatif s'écrit sans extension : il est résolu depuis
`romfs:/game` et reçoit automatiquement l'extension `.raw` :

``` text
SFX_SELECT title_select
```

est résolu en :

``` text
romfs:/game/title_select.raw
```

Un chemin absolu `romfs:/` est aussi accepté. Il est utilisé exactement
tel qu'il est écrit : aucune extension n'est ajoutée, il faut donc
l'écrire en entier, `.raw` compris :

``` text
SFX_CHOICE romfs:/audio/menu_choice.raw
```

### Musique

Syntaxe :

``` text
MUSIC <music>
```

Exemple :

``` text
MUSIC title
MUSIC romfs:/audio/title.ogg
```

Musique de fond jouée en boucle tant que l'écran titre est affiché. Le
fichier doit être au format Ogg Vorbis, mono ou stéréo.

Cette directive est optionnelle. Si elle apparaît plusieurs fois, la
dernière est utilisée.

Un nom relatif s'écrit sans extension : il est résolu depuis
`romfs:/game` et reçoit automatiquement l'extension `.ogg` :
`MUSIC title` est résolu en `romfs:/game/title.ogg`. Un chemin absolu
`romfs:/` est utilisé exactement tel qu'il est écrit : aucune extension
n'est ajoutée, il faut donc l'écrire en entier, `.ogg` compris
(`MUSIC romfs:/audio/title.ogg`).

La musique démarre une fois la description entièrement chargée sans
erreur. Elle s'arrête quand on quitte l'écran titre : intro, nouvelle
partie ou partie chargée. La musique du jeu (`MUSIC` dans
`romfs:/game/game`, voir [GAMESTART.fr.md](./GAMESTART.fr.md)) prend
ensuite le relais au démarrage de la partie. Un fichier absent
n'empêche pas l'écran titre de se charger : il affiche seulement
`File not found: <chemin>` et l'écran reste silencieux.

## 4. Menu

Le menu principal est décrit par un bloc `MENU` :

``` text
MENU
    ...
END_MENU
```

Le menu doit contenir une directive `ORDER` avec au moins une entrée
valide.

### Ordre

Syntaxe :

``` text
ORDER <entry>,<entry>,...
```

Exemple :

``` text
ORDER LANG,INTRO,GAME,CONTROLS,CREDITS
```

Les entrées prises en charge sont :

  Entrée       Clé de localisation affichée   Action
  ------------ ------------------------------ ------------------------------
  `LANG`       `LANG_NAME`                    Passer à la langue suivante
  `INTRO`      `TITLE_INTRO`                  Lancer l'introduction
  `CONTINUE`   `TITLE_CONTINUE`               Charger la partie sauvegardée
  `GAME`       `TITLE_GAME`                   Lancer une nouvelle partie
  `CONTROLS`   `TITLE_CONTROLS`               Ouvrir la page des contrôles
  `CREDITS`    `TITLE_CREDITS`                Ouvrir la page des crédits

Les entrées sont affichées exactement dans l'ordre donné par `ORDER`.
Une entrée qui n'est pas indiquée n'est pas affichée.

Trois entrées sont aussi masquées quand elles n'ont rien à faire :

- `LANG` quand `romfs:/lang` ne contient qu'un seul fichier de langue
  (voir [TRANSLATIONS.fr.md](./TRANSLATIONS.fr.md)) ;
- `INTRO` sans directive `INTRO` (voir [Introduction](#introduction)) ;
- `CONTINUE` quand il n'y a pas de sauvegarde, ce qui est vérifié à
  chaque ouverture de l'écran titre. La sauvegarde nécessite la commande
  `SAVE` du fichier de configuration du jeu (`romfs:/game/game`) ; sans
  elle, `CONTINUE` n'apparaît jamais.

`GAME` supprime la sauvegarde existante avant de lancer la nouvelle
partie : `CONTINUE` disparaît donc au prochain affichage de l'écran
titre, jusqu'à ce que le joueur sauvegarde à nouveau. Si la sauvegarde
ne peut pas être lue, `CONTINUE` lance une nouvelle partie à la place et
en informe le joueur.

Ne pas ajouter d'espaces autour des virgules :

``` text
ORDER INTRO,GAME,CONTROLS,CREDITS,LANG
```

### Sélection par défaut

Syntaxe :

``` text
DEFAULT <entry>
```

Exemple :

``` text
DEFAULT GAME
```

Définit l'entrée du menu sélectionnée lors de l'ouverture de l'écran
titre.

`DEFAULT` peut être placé avant ou après `ORDER` ; la sélection est
résolue après la lecture complète de la description.

L'entrée sélectionnée doit également être présente dans `ORDER`. Si
`DEFAULT` est omis ou désigne une entrée qui n'est pas affichée (absente
de `ORDER`, ou masquée comme décrit plus haut), la première entrée
affichée est sélectionnée.

### Introduction

Syntaxe :

``` text
INTRO <timeline>
```

Exemple :

``` text
INTRO intro
```

Désigne la timeline jouée par l'entrée `INTRO` du menu. `<timeline>` est
le nom d'un dossier de `resources/timelines/` (`romfs:/timelines/` à
l'exécution), sans ce préfixe : `INTRO romfs:/timelines/intro` ne
fonctionne pas. Le nom fait au plus 255 caractères.

Sans directive `INTRO`, l'entrée `INTRO` est retirée du menu, même si
elle figure dans `ORDER`.

### Position

Syntaxe :

``` text
POSITION <x> <y>
```

Exemple :

``` text
POSITION 160 70
```

`x` est le centre horizontal de toutes les entrées du menu. `y` est la
position verticale de la première entrée.

Valeur par défaut :

``` text
160 70
```

### Espacement

Syntaxe :

``` text
SPACING <spacing>
```

Exemple :

``` text
SPACING 30
```

Définit la distance verticale entre deux entrées consécutives du menu.

Valeur par défaut :

``` text
30
```

### Taille du texte

Syntaxe :

``` text
TEXT_SIZE <size>
```

Exemple :

``` text
TEXT_SIZE 0.65
```

Définit l'échelle d'affichage des libellés du menu.

Valeur par défaut :

``` text
0.65
```

### Couleurs

Syntaxe :

``` text
COLOR <color>
SELECTED_COLOR <color>
```

Exemple :

``` text
COLOR GRAY
SELECTED_COLOR LIGHTGRAY
```

`COLOR` définit la couleur des entrées normales du menu.
`SELECTED_COLOR` définit la couleur de l'entrée sélectionnée.

Les noms de couleurs sont résolus par le parseur de couleurs graphiques
utilisé par le jeu.

Valeurs par défaut :

``` text
COLOR GRAY
SELECTED_COLOR LIGHTGRAY
```

### Version

La version du jeu n'est pas configurable dans la description de l'écran
titre. Elle est toujours affichée par le jeu dans le coin inférieur
droit du menu.

## 5. Page des contrôles

La page des contrôles est décrite par un bloc `CONTROLS` :

``` text
CONTROLS
    ...
END_CONTROLS
```

Elle prend en charge les directives `BACKGROUND`, `IMAGE` et `TEXT`. La
page est ouverte par l'entrée `CONTROLS` du menu et fermée avec A ou B.

### Arrière-plan

Syntaxe :

``` text
BACKGROUND <image>
```

Exemple :

``` text
BACKGROUND controls_background
```

Définit l'image affichée en `(0, 0)` sur l'écran inférieur quand la page
des contrôles est ouverte, derrière ses images et ses textes.

Cette directive est optionnelle. Si elle est omise, la page n'a pas
d'arrière-plan. Si elle apparaît plusieurs fois, la dernière est
utilisée.

### Image

Syntaxe :

``` text
IMAGE <image> <x> <y>
```

Exemple :

``` text
IMAGE analogpad 10 0
```

Affiche une image de l'ensemble de ressources graphiques de
`romfs:/game` à la position indiquée sur l'écran inférieur.

Ne pas spécifier l'extension du fichier image.

Une page de contrôles peut contenir jusqu'à 8 directives `IMAGE`.

### Texte

Syntaxe :

``` text
TEXT <localization_key> <x> <y> <size> [alignment]
```

Exemple :

``` text
TEXT TITLE_CONTROLS_MOVE 36 15 0.55 CENTER
```

Affiche le texte localisé associé à `localization_key` à la position et
à l'échelle indiquées.

L'argument optionnel `alignment` définit à quoi correspond `x` :

  Alignement   `x` est
  ------------ ------------------------------
  `LEFT`       le bord gauche du texte
  `CENTER`     le centre horizontal du texte
  `RIGHT`      le bord droit du texte

Si `alignment` est omis, le texte est aligné à gauche. Un alignement
inconnu empêche le chargement de l'écran titre, avec le message
`<fichier>:<ligne>: incorrect argument: <alignement>`. `y` est toujours
le haut du texte.

Un texte localisé peut contenir des retours à la ligne `\n`.

La couleur du texte est fixée par l'implémentation de l'écran titre et
ne fait pas partie du format de description.

Une page de contrôles peut contenir jusqu'à 8 directives `TEXT`.

## 6. Page des crédits

La page des crédits est décrite par un bloc `CREDITS` :

``` text
CREDITS
    ...
END_CREDITS
```

Elle prend en charge les directives `BACKGROUND`, `CREDIT` et `IMAGE`.
La page est ouverte par l'entrée `CREDITS` du menu et fermée avec A ou
B.

### Arrière-plan

Syntaxe :

``` text
BACKGROUND <image>
```

Exemple :

``` text
BACKGROUND credits_background
```

Définit l'image affichée en `(0, 0)` sur l'écran inférieur quand la page
des crédits est ouverte, derrière ses lignes de crédits et ses images.

Cette directive est optionnelle. Si elle est omise, la page n'a pas
d'arrière-plan. Si elle apparaît plusieurs fois, la dernière est
utilisée.

### Ligne de crédit

Syntaxe :

``` text
CREDIT <role_key> <name>
```

Exemple :

``` text
CREDIT TITLE_CREDITS_PROGRAMMER Jane Doe
```

`role_key` est une clé de localisation. `name` est un texte littéral non
traduit qui s'étend jusqu'à la fin de la ligne et peut donc contenir des
espaces :

``` text
CREDIT TITLE_CREDITS_GRAPHICS Alex Martin & Sam Lee
```

La disposition des crédits est fixée par l'implémentation de l'écran
titre : les rôles sont affichés à gauche, les noms à droite et les
lignes successives sont espacées verticalement automatiquement.

Une page de crédits peut contenir jusqu'à 11 directives `CREDIT`.

### Image

Syntaxe :

``` text
IMAGE <image> <x> <y>
```

Exemple :

``` text
IMAGE publisher_logo 85 160
```

Affiche une image de l'ensemble de ressources graphiques de
`romfs:/game` à la position indiquée sur l'écran inférieur.

Une page de crédits peut contenir jusqu'à 8 directives `IMAGE`.

## 7. Images

Chaque image référencée par la description de l'écran titre doit exister
dans l'ensemble de ressources graphiques chargé depuis `romfs:/game`.

Les images sont référencées par leur nom :

``` text
BACKGROUND_TOP background_top
BACKGROUND_BOTTOM background_bottom
BACKGROUND controls_background
IMAGE publisher_logo 85 160
```

Ne pas spécifier l'extension du fichier image.

## 8. Texte et localisation

Les libellés du menu sont associés à des clés de localisation par
l'implémentation de l'écran titre. La description contrôle quelles
entrées sont affichées et dans quel ordre ; elle ne redéfinit pas leurs
libellés.

La correspondance est :

``` text
LANG     -> LANG_NAME
INTRO    -> TITLE_INTRO
CONTINUE -> TITLE_CONTINUE
GAME     -> TITLE_GAME
CONTROLS -> TITLE_CONTROLS
CREDITS  -> TITLE_CREDITS
```

Les directives `TEXT` du bloc `CONTROLS` et les identifiants de rôles
des directives `CREDIT` sont également des clés de localisation.

Les noms des personnes dans les crédits ne sont volontairement pas
localisés.

Les clés de localisation sont résolues au moment de l'affichage. Changer
de langue avec l'entrée `LANG` met donc immédiatement à jour les textes
visibles de l'écran titre.

## 9. Exemple complet

L'exemple suivant illustre toutes les directives prises en charge :

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

Les noms de ressources et de clés de localisation utilisés dans cet
exemple illustrent le format ; ils ne constituent pas une syntaxe
supplémentaire.

## 10. Récapitulatif des directives

| Directive           | Bloc       | Obligatoire | Répétable            | Description                                              |
|---------------------|------------|-------------|----------------------|----------------------------------------------------------|
| `BACKGROUND_TOP`    | —          | non         | —                    | Arrière-plan de l'écran supérieur (section 2).           |
| `BACKGROUND_BOTTOM` | —          | non         | —                    | Arrière-plan de l'écran inférieur, derrière le menu (section 2). |
| `SFX_SELECT`        | —          | non         | —                    | Son joué quand l'entrée sélectionnée change (section 3). |
| `SFX_CHOICE`        | —          | non         | —                    | Son joué quand une entrée est activée (section 3).       |
| `MUSIC`             | —          | non         | —                    | Musique de l'écran titre (section 3).                    |
| `MENU`              | —          | oui         | —                    | Bloc du menu principal, fermé par `END_MENU` (section 4). |
| `ORDER`             | `MENU`     | oui         | oui (6 entrées max)  | Entrées du menu et leur ordre.                           |
| `DEFAULT`           | `MENU`     | non         | —                    | Entrée sélectionnée à l'ouverture de l'écran.            |
| `INTRO`             | `MENU`     | non         | —                    | Timeline jouée par l'entrée `INTRO`.                     |
| `POSITION`          | `MENU`     | non         | —                    | Position de la première entrée.                          |
| `SPACING`           | `MENU`     | non         | —                    | Distance verticale entre deux entrées.                   |
| `TEXT_SIZE`         | `MENU`     | non         | —                    | Taille du texte des entrées.                             |
| `COLOR`             | `MENU`     | non         | —                    | Couleur des entrées.                                     |
| `SELECTED_COLOR`    | `MENU`     | non         | —                    | Couleur de l'entrée sélectionnée.                        |
| `CONTROLS`          | —          | non         | —                    | Bloc de la page des contrôles, fermé par `END_CONTROLS` (section 5). |
| `BACKGROUND`        | `CONTROLS` | non         | —                    | Arrière-plan de la page des contrôles.                   |
| `IMAGE`             | `CONTROLS` | non         | oui (8)              | Image affichée sur la page des contrôles.                |
| `TEXT`              | `CONTROLS` | non         | oui (8)              | Texte localisé de la page des contrôles.                 |
| `CREDITS`           | —          | non         | —                    | Bloc de la page des crédits, fermé par `END_CREDITS` (section 6). |
| `BACKGROUND`        | `CREDITS`  | non         | —                    | Arrière-plan de la page des crédits.                     |
| `CREDIT`            | `CREDITS`  | non         | oui (11)             | Une ligne rôle / nom.                                    |
| `IMAGE`             | `CREDITS`  | non         | oui (8)              | Image affichée sur la page des crédits.                  |

Remarques :

- **Obligatoire.** L'écran titre ne se charge pas si son menu est vide :
  un bloc `MENU` avec un `ORDER` qui liste au moins une entrée affichée
  est donc obligatoire. Tout le reste peut être omis : il n'existe pas
  de mot-clé `NONE`, `DISABLED` ou `ENABLED`. L'argument `alignment` de
  `TEXT` est lui aussi optionnel.
- **Directives répétées.** Quand une directive marquée `—` apparaît
  plusieurs fois, c'est la dernière valeur qui compte. Les lignes
  `ORDER` successives ajoutent leurs entrées au menu. Au-delà de la
  limite indiquée entre parenthèses, les lignes en trop sont ignorées
  avec un message dans les logs ; le chargement n'échoue pas.
- **Argument manquant.** Une directive sans ses arguments fait échouer
  le chargement de l'écran titre.
- **Directive inconnue.** Une directive inconnue, ou placée hors de son
  bloc, fait échouer le chargement de l'écran titre, avec
  `<fichier>:<ligne>: unknown command: <directive>` dans les logs.
- **Sous-pages.** Un bloc `CONTROLS` ou `CREDITS` n'est utile que si
  l'entrée correspondante figure dans `ORDER`.

## 11. Erreurs courantes

### Ajouter une extension d'image

Ne pas faire ceci :

``` text
BACKGROUND_TOP background_top.png
```

Utiliser le nom de l'image dans l'ensemble de ressources :

``` text
BACKGROUND_TOP background_top
```

### Ajouter des espaces dans ORDER

Ne pas écrire :

``` text
ORDER LANG, INTRO, GAME
```

`ORDER` est un seul token séparé par des virgules. Écrire :

``` text
ORDER LANG,INTRO,GAME
```

### Utiliser une clé de localisation dans ORDER

Ne pas écrire :

``` text
ORDER TITLE_INTRO,TITLE_GAME,TITLE_CREDITS
```

`ORDER` utilise les identifiants du menu de l'écran titre :

``` text
ORDER INTRO,GAME,CREDITS
```

### Localiser les noms des crédits

Le premier argument de `CREDIT` est localisé ; le reste de la ligne est
un nom littéral :

``` text
CREDIT TITLE_CREDITS_PROGRAMMER Jane Doe
```

### Mal orthographier l'alignement d'un TEXT

Ne pas écrire :

``` text
TEXT TITLE_CONTROLS_MOVE 36 15 0.55 CENTRE
```

Les alignements s'écrivent en anglais et sont sensibles à la casse. Un
alignement inconnu empêche le chargement de l'écran titre. Utiliser
`LEFT`, `CENTER` ou `RIGHT`.

### Essayer de configurer la version

La position, la taille et la couleur de la version sont volontairement
codées en dur et ne font pas partie du fichier de description de l'écran
titre.

## 12. Style recommandé

Pour améliorer la lisibilité, la description de l'écran titre doit
normalement être organisée dans l'ordre suivant :

``` text
Arrière-plans
Effets sonores et musique
Menu
Contrôles
Crédits
```

Utiliser des commentaires de section et indenter les directives
imbriquées de manière cohérente. Conserver le comportement du menu dans
le code du jeu et utiliser la description uniquement pour le contenu,
l'ordre et la disposition de l'écran titre.

Le format de l'écran titre est volontairement un petit format de
description d'interface spécifique au jeu, et non un langage de script
de menu généraliste. Si l'écran titre nécessite un comportement qui ne
peut pas être exprimé proprement avec les directives existantes,
préférer l'ajout d'une petite primitive réutilisable au format.
