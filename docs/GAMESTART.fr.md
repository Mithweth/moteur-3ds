# Fichier de configuration du jeu

Ce document décrit le fichier `game`, qui définit comment démarre une
nouvelle partie (la première salle, la musique de fond et les objets déjà
présents dans l'inventaire), l'emplacement de la sauvegarde, ainsi que
l'apparence de la boîte de message, de la boîte de choix et du menu Start.

## 1. Emplacement et chargement

Le fichier se trouve dans :

```text
resources/game/game
```

La compilation copie tous les fichiers de `resources/game/` dans la
RomFS, et le jeu le lit depuis :

```text
romfs:/game/game
```

Le fichier est lu **une seule fois**, au lancement du jeu, avant
l'initialisation de l'inventaire et du HUD. Si le fichier est absent ou
invalide, le jeu ne démarre pas.

Son contenu est ensuite appliqué **à chaque nouvelle partie** lancée
depuis l'écran titre (« Nouvelle partie ») :

1. l'inventaire, les états de jeu et le HUD sont réinitialisés ;
2. la musique est lancée, si `MUSIC` est renseigné ;
3. les objets listés par `ITEM` sont ajoutés à l'inventaire ;
4. le joueur entre dans la salle indiquée par `ROOM`.

Quand le joueur choisit « Continuer », seules les étapes 1 et 2
s'appliquent : l'inventaire et la salle viennent de la sauvegarde, pas de
`ITEM` ni de `ROOM`.

Les couleurs et les tailles du texte s'appliquent pendant toute la partie.

## 2. Syntaxe générale

Le fichier contient une commande par ligne, suivie de ses arguments :

```text
ROOM hall
MUSIC romfs:/audio/background.ogg
ITEM FLASHLIGHT
```

- La commande et ses arguments sont séparés par des **espaces**. Les
  tabulations ne sont pas acceptées comme séparateur.
- Les espaces et tabulations en début et en fin de ligne sont ignorés.
- Les commandes sont sensibles à la casse : `ROOM` est valide, `Room` ne
  l'est pas.
- Les lignes vides ou commençant par `#` sont ignorés.
- Les mots en trop après les arguments attendus sont ignorés.
- Les chemins et les noms contenant des espaces ne sont **pas** pris en
  charge.
- Une commande inconnue fait échouer le chargement.
- Une ligne ne doit pas dépasser 511 caractères.

### 2.1. Couleurs

Les commandes de couleur attendent quatre entiers séparés par des
espaces (`A` représente l'opacité). Les valeurs possibles vont de 0 à 255.

```text
<R> <V> <B> <A>
```

Exemple : `150 120 55 255` est un doré opaque.

## 3. Commandes

| Commande         | Obligatoire | Répétable | Description                                           |
|------------------|-------------|-----------|-------------------------------------------------------|
| `ROOM`           | oui         | non       | Salle dans laquelle démarre une nouvelle partie.      |
| `MUSIC`          | non         | non       | Musique de fond jouée pendant la partie.              |
| `ITEM`           | non         | oui (16)  | Objet déjà dans l'inventaire au début de la partie.   |
| `MESSAGE`        | non         | —         | Bloc du texte de la boîte de message (voir 3.4).      |
| `CHOICE`         | non         | —         | Bloc de la boîte de choix et du menu Start (voir 3.5). |
| `FRAME_COLORS`   | oui         | non       | Bloc des couleurs du cadre (voir 3.6).                |
| `SAVE`           | non         | —         | Fichier de sauvegarde ; active la sauvegarde (voir 3.7). |
| `CANNOT_USE_MESSAGE` | non     | non       | Message quand un objet ne peut pas être utilisé (voir 3.8). |

`ROOM`, `MUSIC` et `CANNOT_USE_MESSAGE` ne peuvent apparaître qu'une seule fois : une seconde
occurrence fait échouer le chargement. Pour `SAVE` et les commandes des
blocs `MESSAGE` et `CHOICE`, si une commande est répétée, c'est la
dernière valeur qui compte ; ces deux blocs peuvent apparaître plusieurs
fois.

Les nombres décimaux (tailles, largeurs) s'écrivent avec un point (`0.6`,
pas `0,6`). Pour les tailles de texte, `1.0` correspond à la taille
normale de la police.

### 3.1. ROOM

```text
ROOM hall
```

Nom de la salle de départ, c'est-à-dire le nom de son répertoire dans
`resources/rooms/` (chargée depuis `romfs:/rooms/<nom>`).

Cette commande est obligatoire : sans elle, le chargement échoue avec
`<fichier>: Missing ROOM`.

La salle n'est chargée qu'au démarrage d'une partie, pas à la lecture du
fichier. Une faute de frappe dans le nom de la salle n'est donc signalée
qu'à ce moment-là, par `Cannot enter room: <nom>`, et le jeu revient à
l'écran titre.

### 3.2. MUSIC

```text
MUSIC background
MUSIC romfs:/audio/background.ogg
```

Un fichier Ogg Vorbis (mono ou stéréo), indiqué de l'une de ces deux
façons (une seule : `MUSIC` ne peut apparaître qu'une fois) :

- un nom relatif, **sans extension**, résolu depuis `romfs:/game` avec
  `.ogg` ajouté : `MUSIC background` joue `romfs:/game/background.ogg` ;
- un chemin absolu commençant par `romfs:/`, utilisé exactement tel
  qu'il est écrit : aucune extension n'est ajoutée, il faut donc l'écrire
  en entier, `.ogg` compris (`MUSIC romfs:/audio/background.ogg`).

La musique est lancée au début d'une partie, puis relancée quand le
joueur quitte un mini-jeu ou une timeline qui finit par `RETURN`.

Sans cette commande, le jeu est silencieux : aucune musique n'est jouée,
ni au début de la partie ni après un mini-jeu.

### 3.3. ITEM

```text
ITEM FLASHLIGHT
ITEM SCREWDRIVER
```

Un identifiant d'objet par ligne, tel que déclaré par les directives
`ITEM` de `resources/inventory/inventory`. Les objets sont ajoutés à
l'inventaire dans l'ordre du fichier.

- 16 objets au maximum : un 17e `ITEM` fait échouer le chargement avec
  `too many ITEM (max 16)`.
- Un identifiant inconnu n'empêche pas la partie de démarrer : le jeu
  affiche `Unknown inventory item: <id>` et passe à l'objet suivant.
- Un objet listé deux fois n'est ajouté qu'une seule fois.
- Sans `ITEM`, l'inventaire démarre vide.

### 3.4. MESSAGE ... END_MESSAGE

```text
MESSAGE
    TEXT_COLOR 255 255 255 255
    TEXT_SIZE 0.5
END_MESSAGE
```

Texte de la boîte de message (messages des salles, des objets et des
extensions).

| Commande           | Valeur          | Défaut            | Description                |
|--------------------|-----------------|-------------------|----------------------------|
| `TEXT_COLOR`       | couleur (2.1)   | `255 255 255 255` | couleur du texte           |
| `TEXT_SIZE`        | nombre décimal  | `0.6`             | échelle du texte           |

Le texte est découpé en lignes selon la largeur de la boîte à cette
taille. La boîte grandit avec le nombre de lignes : avec une grande
taille, gardez les messages courts.

### 3.5. CHOICE ... END_CHOICE

```text
CHOICE
    FRAME_WIDTH 260
    PROMPT_TEXT_COLOR 255 255 255 255
    PROMPT_TEXT_SIZE 0.5
    BUTTON_TEXT_COLOR 255 255 255 255
    BUTTON_TEXT_SIZE 0.5
    BUTTON_COLOR 105 82 40 255
    BUTTON_PADDING 5
END_CHOICE
```

Apparence de la boîte affichée par l'action de salle `CHOICE` (voir
`CHOICES.fr.md`) et du menu Start, qui utilise la même boîte. La boîte
est centrée sur l'écran du bas : la question en haut, puis un bouton par
option.

| Commande            | Valeur          | Défaut            | Description                                          |
|---------------------|-----------------|-------------------|------------------------------------------------------|
| `FRAME_WIDTH`       | pixels          | `276`             | largeur de la boîte (l'écran fait 320 pixels de large) |
| `PROMPT_TEXT_COLOR` | couleur (2.1)   | `255 255 255 255` | couleur de la question                               |
| `PROMPT_TEXT_SIZE`  | nombre décimal  | `0.6`             | échelle de la question, découpée selon la largeur de la boîte |
| `BUTTON_TEXT_COLOR` | couleur (2.1)   | `255 255 255 255` | couleur des libellés des boutons                     |
| `BUTTON_TEXT_SIZE`  | nombre décimal  | `0.6`             | échelle des libellés des boutons (non découpés)      |
| `BUTTON_COLOR`      | couleur (2.1)   | `105 82 40 255`   | rectangle derrière le bouton sélectionné             |
| `BUTTON_PADDING`    | pixels          | `5`               | marge autour d'un libellé, qui espace aussi les boutons |

La hauteur de la boîte dépend de la question et du nombre de boutons :
avec de grandes tailles, un choix comportant beaucoup d'options peut ne
pas tenir dans les 240 pixels de l'écran.

### 3.6. FRAME_COLORS ... END_FRAME_COLORS

```text
FRAME_COLORS
    SHADOW 0 0 0 150
    OUTER_BORDER 150 120 55 255
    OUTER_BACKGROUND 18 24 34 235
    INNER_BORDER 105 82 40 255
    INNER_BACKGROUND 22 28 40 245
END_FRAME_COLORS
```

Couleurs du cadre de la boîte de message, de la boîte de choix et du menu
Start. Le cadre est dessiné en cinq couches, de l'extérieur vers
l'intérieur :

| Commande           | Couche                                                   |
|--------------------|----------------------------------------------------------|
| `SHADOW`           | ombre portée, décalée de 3 pixels vers le bas à droite   |
| `OUTER_BORDER`     | bordure extérieure (2 pixels)                            |
| `OUTER_BACKGROUND` | fond entre les deux bordures (3 pixels)                  |
| `INNER_BORDER`     | bordure intérieure (1 pixel)                             |
| `INNER_BACKGROUND` | fond sur lequel le texte est affiché                     |

- Entre `FRAME_COLORS` et `END_FRAME_COLORS`, seules ces cinq commandes
  sont acceptées. Toute autre commande fait échouer le chargement
- Une couche absente du bloc reste **transparente** (`0 0 0 0`) : elle
  n'est pas dessinée.
- Le bloc est obligatoire et doit être fermé par `END_FRAME_COLORS`

### 3.7. SAVE

```text
SAVE sdmc:/moteur.save
```

Chemin complet du fichier de sauvegarde. Le jeu n'a qu'un seul
emplacement de sauvegarde.

- Le fichier doit être sur la carte SD : le chemin commence donc par
  `sdmc:/`. La RomFS est en lecture seule : avec un chemin `romfs:/`,
  toutes les sauvegardes échouent.
- Le répertoire doit déjà exister : le jeu crée le fichier, pas les
  répertoires qui y mènent. Un fichier à la racine de la carte évite le
  problème.
- Le chemin fait au plus 251 caractères, car le jeu écrit aussi un
  fichier temporaire nommé `<chemin>.tmp` (voir plus bas).

Sans cette commande, la sauvegarde est désactivée : le menu Start n'a pas
de bouton « Sauvegarder » et l'écran titre n'affiche jamais `CONTINUE`.

Avec elle :

- « Sauvegarder » dans le menu Start enregistre le temps écoulé, la salle
  courante, l'inventaire, les états de jeu vrais et les données des
  extensions. Ce bouton n'est proposé que lorsque le joueur explore une
  salle, pas pendant un message, un choix, un mini-jeu ou une timeline.
- La sauvegarde est d'abord écrite dans `<chemin>.tmp`, puis renommée :
  une sauvegarde qui échoue ou qui est interrompue conserve la
  précédente.
- `CONTINUE` sur l'écran titre la charge (voir la documentation de
  l'écran titre) ; « Nouvelle partie » la supprime.

### 3.8. CANNOT_USE_MESSAGE

```text
CANNOT_USE_MESSAGE GAME_CANNOT_USE_MESSAGE
```

Clé de traduction du message affiché quand le joueur utilise un objet
(A) sur une cible qui n'a pas de bloc `USE` correspondant, et que
l'objet n'a pas de `USE_CALLBACK`.

- Sans cette commande, rien n'est affiché : l'appui sur A ne fait
  simplement rien. C'est à chaque jeu de décider s'il veut ce retour.
- Le message n'est affiché que si un hotspot est ciblé. Sans cible, il
  ne se passe rien.
- La valeur est une clé de traduction, à définir dans chaque fichier
  `.lang` (voir la documentation des traductions).

## 4. Exemple complet

```text
# Configuration du jeu

# Première salle
ROOM hall

# Musique de fond
MUSIC romfs:/audio/background.ogg

# Inventaire de départ
ITEM FLASHLIGHT
ITEM MAGNIFYING_GLASS

# Emplacement de sauvegarde sur la carte SD
SAVE sdmc:/moteur.save

# Retour quand un objet est utilisé au mauvais endroit
CANNOT_USE_MESSAGE GAME_CANNOT_USE_MESSAGE

# Boîte de message
MESSAGE
    TEXT_COLOR 255 255 255 255
    TEXT_SIZE 0.5
END_MESSAGE

# Boîte de choix et menu Start
CHOICE
    FRAME_WIDTH 260
    PROMPT_TEXT_COLOR 255 255 255 255
    PROMPT_TEXT_SIZE 0.5
    BUTTON_TEXT_COLOR 255 255 255 255
    BUTTON_TEXT_SIZE 0.5
    BUTTON_COLOR 105 82 40 255
    BUTTON_PADDING 5
END_CHOICE

FRAME_COLORS
    SHADOW 0 0 0 150
    OUTER_BORDER 150 120 55 255
    OUTER_BACKGROUND 18 24 34 235
    INNER_BORDER 105 82 40 255
    INNER_BACKGROUND 22 28 40 245
END_FRAME_COLORS
```

## 5. Erreurs fréquentes

### Oublier `END_FRAME_COLORS`, `END_MESSAGE` ou `END_CHOICE`

Tant qu'un bloc n'est pas fermé, seules ses propres commandes sont
acceptées : une ligne `ROOM` ou `MUSIC` placée après fait échouer le
chargement avec `unknown command`. Si le bloc est le dernier du fichier,
le chargement échoue avec `missing END_FRAME_COLORS`,
`missing END_MESSAGE` ou `missing END_CHOICE`.

### Utiliser les anciennes commandes de style hors d'un bloc

```text
TEXT_COLOR 255 255 255 255
TEXT_SIZE 0.6
BUTTON_COLOR 105 82 40 255
```

Ces commandes ne sont plus acceptées au niveau du fichier et font échouer
le chargement avec `unknown command`. Déplacez `TEXT_COLOR` et
`TEXT_SIZE` dans le bloc `MESSAGE`, et `BUTTON_COLOR` dans le bloc
`CHOICE`, où `BUTTON_TEXT_COLOR` et `BUTTON_TEXT_SIZE` règlent les
boutons du menu Start.

### Donner un nom de couleur

```text
MESSAGE
    TEXT_COLOR WHITE
END_MESSAGE
```

Contrairement au fichier de l'écran titre, ce fichier n'accepte pas les
noms de couleurs : donnez les quatre valeurs `R V B A`.

### Oublier l'opacité

```text
MESSAGE
    TEXT_COLOR 255 255 255
END_MESSAGE
```

L'opacité est obligatoire : sans elle, le chargement échoue avec
`invalid TEXT_COLOR`. Utilisez `255` pour une couleur opaque.

### Mettre l'extension, ou oublier romfs:/, dans le chemin de la musique

```text
MUSIC background.ogg
MUSIC audio/background.ogg
```

Sans le préfixe `romfs:/`, le nom est relatif à `romfs:/game` et `.ogg`
y est ajouté : ces lignes cherchent `romfs:/game/background.ogg.ogg` et
`romfs:/game/audio/background.ogg.ogg`. Écrivez `MUSIC background` pour
`romfs:/game/background.ogg`, ou le chemin complet
`MUSIC romfs:/audio/background.ogg` pour un fichier situé ailleurs.

### Mal orthographier le préfixe de la carte SD

```text
SAVE smdc:/moteur.save
```

Le préfixe est `sdmc:`. Avec tout autre préfixe, le fichier ne peut pas
être créé : le fichier de configuration se charge quand même, mais chaque
sauvegarde échoue avec le message d'erreur de sauvegarde, et `CONTINUE`
n'apparaît jamais sur l'écran titre.

### Utiliser le nom de l'objet au lieu de son identifiant

`ITEM` attend l'identifiant déclaré après `ITEM` dans
`resources/inventory/inventory` (par exemple `FLASHLIGHT`), ni la clé de
traduction ni le nom de l'image.
