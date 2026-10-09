# Fichier de description du HUD

Ce document décrit le format déclaratif utilisé pour définir le HUD du
jeu.

Une description de HUD définit la disposition et l'apparence de
l'interface affichée sur l'écran supérieur de la Nintendo 3DS :
inventaire, directions de déplacement, objet sélectionné, cible courante
et chronomètre.

## 1. Emplacement et structure générale

La description du HUD est chargée depuis :

``` text
romfs:/hud/hud
```

Ses ressources graphiques sont chargées depuis l'ensemble de ressources
correspondant au HUD.

Le HUD utilise l'espace de coordonnées 400 × 240 de l'écran supérieur de
la Nintendo 3DS.

Un HUD contient généralement un arrière-plan global suivi de plusieurs
blocs :

``` text
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

Le `BACKGROUND` global et le bloc `INVENTORY` sont obligatoires : sans
eux, le chargement du HUD échoue. `DIRECTIONS`, `OBJECT`, `TARGET` et
`TIMER` sont optionnels et sont activés par la présence de leur bloc
correspondant. La section 11 récapitule toutes les directives et
indique celles qui sont obligatoires.

Les lignes vides sont ignorées. Une ligne dont le premier caractère non
blanc est `#` est un commentaire. Les commentaires doivent être placés
sur leur propre ligne ; les commentaires en fin de ligne ne font pas
partie du format.

Les tokens sont séparés par des espaces. Les identifiants tels que les
noms d'images, les clés de localisation et les noms de timelines ne
contiennent donc pas d'espaces et ne sont pas placés entre guillemets.

L'indentation ne sert qu'à améliorer la lisibilité ; la structure des
blocs est déterminée par les directives `END_*`.

## 2. Arrière-plan global

Syntaxe :

``` text
BACKGROUND <image>
```

Exemple :

``` text
BACKGROUND background
```

Définit l'arrière-plan principal du HUD. Il est affiché en `(0, 0)`.

Cette directive est obligatoire.

Ne pas spécifier l'extension du fichier image.

## 3. Inventaire

L'inventaire est décrit par un bloc `INVENTORY` :

``` text
INVENTORY
    ...
END_INVENTORY
```

Ce bloc est obligatoire.

### Arrière-plan

Syntaxe :

``` text
BACKGROUND <image> <x> <y>
```

Exemple :

``` text
BACKGROUND inventory 7 49
```

Définit une image d'arrière-plan optionnelle pour la zone d'inventaire.

`x` et `y` sont des coordonnées dans l'espace 400 × 240 du HUD.

### Titre

Syntaxe :

``` text
TEXT <color> <text> <x> <y> <size>
```

Exemple :

``` text
TEXT BLACK HUD_INVENTORY 115 52 0.5
```

Définit le titre de l'inventaire. Cette directive est obligatoire.

`text` est une clé de localisation. `x` et `y` définissent la position
du texte et `size` définit son échelle d'affichage.

### Position des objets

Syntaxe :

``` text
ITEM_POSITION <x> <y>
```

Exemple :

``` text
ITEM_POSITION 20 75
```

Définit la position du premier objet visible de l'inventaire.

La position des autres objets est calculée à partir de cette origine en
utilisant `ITEM_SIZE`, `SPACING`, `COLUMNS` et `ROWS`.

### Taille des objets

Syntaxe :

``` text
ITEM_SIZE <size>
```

Exemple :

``` text
ITEM_SIZE 32
```

Définit la taille d'affichage des icônes des objets de l'inventaire.

Valeur par défaut :

``` text
32
```

### Espacement

Syntaxe :

``` text
SPACING <x> <y>
```

Exemple :

``` text
SPACING 14 10
```

Définit l'espacement horizontal et vertical ajouté entre les objets de
l'inventaire.

La position d'un objet est calculée ainsi :

``` text
x = ITEM_POSITION.x + column * (ITEM_SIZE + SPACING.x)
y = ITEM_POSITION.y + row    * (ITEM_SIZE + SPACING.y)
```

Valeurs par défaut :

``` text
x = 14
y = 10
```

### Colonnes

Syntaxe :

``` text
COLUMNS <count>
```

Exemple :

``` text
COLUMNS 6
```

Définit le nombre de colonnes de l'inventaire.

Cette valeur détermine également le déplacement vertical lors de la
navigation dans l'inventaire. Elle doit être au moins égale à 1.

Valeur par défaut :

``` text
6
```

### Lignes

Syntaxe :

``` text
ROWS <count>
```

Exemple :

``` text
ROWS 2
```

Définit le nombre de lignes de l'inventaire affichées simultanément.
La valeur doit être au moins égale à 1.

Valeur par défaut :

``` text
2
```

### Marqueur de sélection

Syntaxe :

``` text
SELECTION <image> <x> <y>
```

Exemple :

``` text
SELECTION selected -4 -4
```

Définit l'image utilisée pour mettre en évidence l'objet actuellement
sélectionné dans l'inventaire. Cette directive est obligatoire.

Contrairement à la plupart des coordonnées d'images du HUD, `x` et `y`
sont des décalages relatifs à la position de l'objet sélectionné. Des
valeurs négatives peuvent donc être utilisées pour afficher un cadre de
sélection autour d'un objet.

### Aperçu de l'objet sélectionné

Syntaxe :

``` text
SELECTED_ITEM <x> <y> <size>
```

Exemple :

``` text
SELECTED_ITEM 20 169 0.5
```

Affiche une miniature de l'objet actuellement sélectionné dans
l'inventaire à la position indiquée dans le HUD.

`size` est l'échelle d'affichage de la miniature.

Cette directive est optionnelle. Si elle est omise, aucune miniature
séparée de l'objet sélectionné n'est affichée.

`SELECTION` et `SELECTED_ITEM` ont des rôles différents : `SELECTION`
met en évidence l'objet dans la grille d'inventaire, tandis que
`SELECTED_ITEM` affiche une miniature séparée ailleurs dans le HUD.

### Arrière-plan d'examen

Syntaxe :

``` text
EXAMINE_BACKGROUND <image> <x> <y>
```

Exemple :

``` text
EXAMINE_BACKGROUND examine_background 7 49
```

Définit un arrière-plan optionnel affiché lors de l'examen d'un objet de
l'inventaire.

### Texte d'examen

Syntaxe :

``` text
EXAMINE_TEXT <color> <x> <y> <size>
```

Exemple :

``` text
EXAMINE_TEXT WHITE 20 62 0.55
```

Définit la manière dont la description de l'objet examiné est affichée.

Le texte lui-même provient de la définition de l'objet dans l'inventaire
; cette directive définit uniquement sa couleur, sa position et sa
taille.

## 4. Directions

L'indicateur de directions est décrit par un bloc `DIRECTIONS` :

``` text
DIRECTIONS
    ...
END_DIRECTIONS
```

Le bloc entier est optionnel. S'il est omis, aucun indicateur de
directions n'est affiché.

### Arrière-plan

Syntaxe :

``` text
BACKGROUND <image> <x> <y>
```

Exemple :

``` text
BACKGROUND directions 303 51
```

Définit une image d'arrière-plan optionnelle pour l'indicateur de
directions.

### Images des directions

Syntaxe :

``` text
<direction> <image> <x> <y>
```

Les directions prises en charge sont :

``` text
NORTH
NORTHEAST
EAST
SOUTHEAST
SOUTH
SOUTHWEST
WEST
NORTHWEST
```

Exemple :

``` text
NORTH arrow_n 339 62
NORTHEAST arrow_ne 363 74
```

Chaque direction possède sa propre image et sa propre position absolue
dans le HUD.

L'image d'une direction est affichée lorsque le déplacement dans cette
direction est actuellement disponible depuis la pièce active.

## 5. Objet sélectionné

Le bloc `OBJECT` contrôle la zone du HUD décrivant l'objet actuellement
sélectionné dans l'inventaire :

``` text
OBJECT
    ...
END_OBJECT
```

Le bloc entier est optionnel.

### Arrière-plan

Syntaxe :

``` text
BACKGROUND <image> <x> <y>
```

Exemple :

``` text
BACKGROUND object_background 10 170
```

Définit une image d'arrière-plan optionnelle pour la zone de l'objet.

### Libellé

Syntaxe :

``` text
TEXT <color> <text> <x> <y> <size>
```

Exemple :

``` text
TEXT YELLOW HUD_OBJECT 60 172 0.5
```

Définit le libellé statique localisé du panneau. Cette directive est
obligatoire lorsque le bloc `OBJECT` est présent.

`text` est une clé de localisation.

### Nom de l'objet

Syntaxe :

``` text
ITEM <color> <x> <y> <size>
```

Exemple :

``` text
ITEM WHITE 115 172 0.5
```

Définit la manière dont le nom localisé de l'objet actuellement
sélectionné dans l'inventaire est affiché.

L'aperçu optionnel de l'objet sélectionné est configuré séparément avec
`SELECTED_ITEM` dans le bloc `INVENTORY`.

## 6. Cible

Le bloc `TARGET` contrôle la zone du HUD décrivant la cible
d'interaction courante :

``` text
TARGET
    ...
END_TARGET
```

Le bloc entier est optionnel.

Il prend en charge les mêmes directives que `OBJECT` :

``` text
BACKGROUND <image> <x> <y>
TEXT <color> <text> <x> <y> <size>
ITEM <color> <x> <y> <size>
```

Exemple :

``` text
TARGET
    TEXT YELLOW HUD_TARGET 60 208 0.5
    ITEM WHITE 115 208 0.5
END_TARGET
```

`TEXT` définit le libellé statique localisé ; il est obligatoire lorsque
le bloc `TARGET` est présent. `ITEM` définit la manière dont le nom
localisé de la cible courante est affiché.

## 7. Chronomètre

Le chronomètre est décrit par un bloc `TIMER` :

``` text
TIMER
    ...
END_TIMER
```

Le bloc entier est optionnel. S'il est omis, aucun chronomètre n'est
affiché et aucune timeline d'expiration n'est déclenchée.

### Arrière-plan

Syntaxe :

``` text
BACKGROUND <image> <x> <y>
```

Exemple :

``` text
BACKGROUND timer_background 330 20
```

Définit un arrière-plan optionnel pour le chronomètre.

### Texte

Syntaxe :

``` text
TEXT <color> <x> <y> <size>
```

Exemple :

``` text
TEXT WHITE 335 23 0.4
```

Définit la couleur, la position et la taille du compte à rebours.

Le chronomètre est affiché sous la forme :

``` text
HH:MM:SS
```

### Durée maximale

Syntaxe :

``` text
MAX_DURATION <seconds>
```

Exemple :

``` text
MAX_DURATION 3600
```

Définit la durée initiale du compte à rebours en secondes. Cette
directive est obligatoire lorsque le bloc `TIMER` est présent, et la
valeur doit être supérieure à 0.

### Timeline d'expiration

Syntaxe :

``` text
TIMELINE <timeline>
```

Exemple :

``` text
TIMELINE gameover_timeup
```

Définit la timeline lancée lorsque le chronomètre atteint zéro. Cette
directive est obligatoire lorsque le bloc `TIMER` est présent.

Le chronomètre ne se déclenche qu'une fois par partie. Cette timeline est
normalement un game over qui finit par `END`, ce qui ramène à l'écran
titre. Si elle finit par `RETURN`, le joueur revient dans la pièce et
continue à jouer sans compte à rebours.

## 8. Images

Chaque image référencée par la description du HUD doit exister dans
l'ensemble de ressources graphiques du HUD.

Les images sont référencées par leur nom :

``` text
BACKGROUND inventory 7 49
SELECTION selected -4 -4
NORTH arrow_n 339 62
```

Ne pas spécifier l'extension du fichier image.

Une image inconnue provoque l'échec du chargement du HUD.

## 9. Texte et localisation

Les libellés visibles par l'utilisateur stockés dans la description du
HUD utilisent des clés de localisation.

Par exemple :

``` text
TEXT YELLOW HUD_OBJECT 60 172 0.5
```

`HUD_OBJECT` est résolu dans la langue courante.

Les valeurs correspondantes sont définies dans les fichiers de langue,
par exemple :

``` ini
HUD_OBJECT=Objet
HUD_TARGET=Cible
HUD_INVENTORY=INVENTAIRE
```

Les noms dynamiques des objets, les noms des cibles et les descriptions
d'examen proviennent de leurs définitions respectives dans l'inventaire
ou les pièces.

Les directives de texte utilisent les noms de couleurs pris en charge
par le parseur de couleurs graphiques.

## 10. Exemple complet

``` text
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

## 11. Récapitulatif des directives

| Directive            | Bloc                          | Obligatoire     | Description                                              |
|----------------------|-------------------------------|-----------------|----------------------------------------------------------|
| `BACKGROUND`         | —                             | oui             | Arrière-plan global de l'écran supérieur (section 2).    |
| `INVENTORY`          | —                             | oui             | Bloc de l'inventaire, fermé par `END_INVENTORY` (section 3). |
| `BACKGROUND`         | `INVENTORY`                   | non             | Arrière-plan de l'inventaire.                            |
| `TEXT`               | `INVENTORY`                   | oui             | Titre de l'inventaire.                                   |
| `ITEM_POSITION`      | `INVENTORY`                   | non (`0 0`)     | Position du premier objet de la grille.                  |
| `ITEM_SIZE`          | `INVENTORY`                   | non (`32`)      | Taille d'un objet dans la grille.                        |
| `SPACING`            | `INVENTORY`                   | non (`14 10`)   | Espace horizontal et vertical entre les objets.          |
| `COLUMNS`            | `INVENTORY`                   | non (`6`)       | Nombre de colonnes de la grille, au moins 1.             |
| `ROWS`               | `INVENTORY`                   | non (`2`)       | Nombre de lignes de la grille, au moins 1.               |
| `SELECTION`          | `INVENTORY`                   | oui             | Marqueur affiché autour de l'objet sélectionné.          |
| `SELECTED_ITEM`      | `INVENTORY`                   | non             | Miniature de l'objet sélectionné ailleurs dans le HUD.   |
| `EXAMINE_BACKGROUND` | `INVENTORY`                   | non             | Arrière-plan affiché pendant l'examen d'un objet.        |
| `EXAMINE_TEXT`       | `INVENTORY`                   | non             | Style du texte d'examen.                                 |
| `DIRECTIONS`         | —                             | non             | Bloc des directions, fermé par `END_DIRECTIONS` (section 4). |
| `BACKGROUND`         | `DIRECTIONS`                  | non             | Arrière-plan des directions.                             |
| `NORTH` … `NORTHWEST` | `DIRECTIONS`                 | non             | Image de l'une des huit directions.                      |
| `OBJECT`             | —                             | non             | Bloc de l'objet sélectionné, fermé par `END_OBJECT` (section 5). |
| `TARGET`             | —                             | non             | Bloc de la cible, fermé par `END_TARGET` (section 6).    |
| `BACKGROUND`         | `OBJECT`, `TARGET`            | non             | Arrière-plan du panneau.                                 |
| `TEXT`               | `OBJECT`, `TARGET`            | oui (si bloc)   | Libellé du panneau.                                      |
| `ITEM`               | `OBJECT`, `TARGET`            | non             | Style du nom de l'objet ou de la cible.                  |
| `TIMER`              | —                             | non             | Bloc du chronomètre, fermé par `END_TIMER` (section 7).  |
| `BACKGROUND`         | `TIMER`                       | non             | Arrière-plan du chronomètre.                             |
| `TEXT`               | `TIMER`                       | non             | Style du temps restant.                                  |
| `MAX_DURATION`       | `TIMER`                       | oui (si bloc)   | Durée du compte à rebours en secondes, supérieure à 0.   |
| `TIMELINE`           | `TIMER`                       | oui (si bloc)   | Timeline lancée quand le temps est écoulé.               |

La valeur entre parenthèses est celle utilisée quand la directive est
omise.

Remarques :

- **Blocs optionnels.** `DIRECTIONS`, `OBJECT`, `TARGET` et `TIMER`
  activent leur composant du HUD : omettez le bloc pour le désactiver.
  Dans un bloc, une directive marquée « oui (si bloc) » n'est
  obligatoire que si ce bloc est présent. Il n'existe pas de mot-clé
  `NONE`, `DISABLED` ou `ENABLED`.
- **Directives répétées.** Chaque directive peut apparaître plusieurs
  fois ; c'est la dernière valeur qui compte. Il en va de même pour un
  bloc ouvert deux fois : le second complète ou remplace le premier.
- **Erreurs.** Contrairement à l'écran titre, le HUD est strict : une
  directive inconnue (ou placée dans le mauvais bloc), un argument
  manquant, une image inconnue, un bloc sans son `END_*`, `COLUMNS 0`,
  `ROWS 0` ou une directive obligatoire absente font échouer le
  chargement du HUD, avec un message qui indique le fichier et la
  ligne.
## 12. Erreurs courantes

### Ajouter une extension d'image

Ne pas faire ceci :

``` text
BACKGROUND inventory.png 7 49
```

Utiliser le nom de l'image sans extension :

``` text
BACKGROUND inventory 7 49
```

### Confondre SELECTION et SELECTED_ITEM

`SELECTION` est le marqueur affiché autour de l'objet sélectionné dans
la grille d'inventaire.

`SELECTED_ITEM` est une miniature séparée optionnelle de cet objet,
affichée ailleurs dans le HUD.

### Utiliser du texte littéral au lieu de clés de localisation

Ne pas placer directement les libellés visibles par l'utilisateur dans
une directive `TEXT` :

``` text
TEXT YELLOW Object 60 172 0.5
```

Utiliser une clé de localisation :

``` text
TEXT YELLOW HUD_OBJECT 60 172 0.5
```

### Oublier les directives END\_\*

Chaque bloc doit être fermé par la directive correspondante :

``` text
END_INVENTORY
END_DIRECTIONS
END_OBJECT
END_TARGET
END_TIMER
```

## 13. Style recommandé

Pour améliorer la lisibilité, la description du HUD doit normalement
être organisée dans l'ordre suivant :

``` text
Arrière-plan global
Inventaire
Directions
Objet sélectionné
Cible
Chronomètre
```

Utiliser des commentaires de section et indenter les directives
imbriquées de manière cohérente. Conserver ensemble les directives de
disposition associées et omettre les composants optionnels plutôt que de
déclarer des éléments inutilisés.

Le format du HUD est volontairement un petit format de description
d'interface spécifique au jeu, et non un langage d'interface
généraliste. Si l'interface nécessite un comportement qui ne peut pas
être exprimé proprement avec les directives existantes, préférer l'ajout
d'une petite primitive réutilisable au format.
