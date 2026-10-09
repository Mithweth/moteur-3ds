# Fichiers de description des pièces

Ce document décrit le format déclaratif utilisé pour définir les pièces
du jeu.

Une description de pièce définit le contenu et le comportement d'une
pièce : images affichées, hotspots, conditions, interactions,
utilisation des objets de l'inventaire et sorties.

## 1. Emplacement et structure générale

Une pièce nommée `livingroom` est chargée depuis :

``` text
romfs:/rooms/livingroom/room
```

Ses ressources graphiques sont chargées depuis l'ensemble de ressources
correspondant à la pièce.

Une pièce contient généralement trois sections :

``` text
# Images

IMAGE bg 0 0 0
END_IMAGE

# Hotspots

HOTSPOT EXAMPLE_OBJECT 10 20 40 30
    MESSAGE EXAMPLE_OBJECT_EXAMINE
END_HOTSPOT

# Sorties

PATH SOUTH
    ACTION
        WAIT_SFX door_open
        ROOM corridor
    END_ACTION
END_PATH
```

Les lignes vides sont ignorées. Une ligne dont le premier caractère non
blanc est `#` est un commentaire. Les commentaires doivent être placés
sur leur propre ligne ; les commentaires en fin de ligne ne font pas
partie du format.

Les tokens sont séparés par des espaces. Les identifiants tels que les
noms d'états, d'objets, les IDs de messages, les noms de pièces, de sons
et d'images ne contiennent donc pas d'espaces et ne sont pas placés
entre guillemets.

L'indentation ne sert qu'à améliorer la lisibilité ; la structure des
blocs est déterminée par les directives `END_*`.

## 2. Conditions

Les conditions sont introduites par `WHEN` :

``` text
WHEN STATE_IS <state> <true|false>
WHEN INVENTORY_HAS <item> <true|false>
```

Exemples :

``` text
WHEN STATE_IS underground_dug true
WHEN INVENTORY_HAS SHOVEL false
```

Plusieurs directives `WHEN` dans un même bloc sont combinées par un
**ET** logique : toutes les conditions doivent être satisfaites.

Une condition s'applique au bloc dans lequel elle apparaît. Elle peut
donc contrôler un `IMAGE`, un `HOTSPOT`, un `ACTION`, un `USE` ou un
`PATH`.

Les identifiants d'états et d'objets doivent correspondre à des
identifiants connus respectivement par les systèmes d'état du jeu et
d'inventaire.

## 3. Images

Syntaxe :

``` text
IMAGE <image> <x> <y> <z>
    [WHEN ...]
END_IMAGE
```

Exemple :

``` text
IMAGE hole_dug 261 174 0.3
    WHEN STATE_IS underground_dug true
    WHEN STATE_IS underground_card_taken false
END_IMAGE
```

Ne pas spécifier l'extension du fichier image. Les noms d'images peuvent
contenir des lettres, des chiffres et des underscores (`_`) ; les tirets
(`-`) ne doivent pas être utilisés.

`x` (de 0 à 320) et `y` (de 0 à 240) sont les coordonnées d'affichage.
`z` (de -1.0 à 1.0) contrôle la profondeur d'affichage.

L'image n'est affichée que lorsque toutes ses conditions sont vraies.
Sans `WHEN`, l'image est toujours affichée.

Les images conditionnelles sont indépendantes. Si les conditions de deux
blocs `IMAGE` sont vraies, les deux images sont affichées. Lorsqu'une
seule variante doit être visible, leurs conditions doivent être
explicitement mutuellement exclusives :

``` text
IMAGE full ...
    WHEN STATE_IS card_taken false
END_IMAGE

IMAGE empty ...
    WHEN STATE_IS card_taken true
END_IMAGE
```

## 4. Hotspots

Syntaxe :

``` text
HOTSPOT <id> <x> <y> <width> <height>
    [WHEN ...]
    [MESSAGE <message_id>]
    [MESSAGE_IMAGE <image>]
    [ACTION ... END_ACTION]
    [USE ... END_USE]
END_HOTSPOT
```

Exemple :

``` text
HOTSPOT UNDERGROUND_MAGNETIC_CARD 275 182 24 14
    WHEN STATE_IS underground_card_taken false
    WHEN STATE_IS underground_dug true
    ACTION
        SET underground_card_taken
        INVENTORY_ADD MAGNETIC_CARD
    END_ACTION
END_HOTSPOT
```

Le rectangle est défini par `x`, `y`, `width` et `height`.

Les directives `WHEN` au niveau du hotspot déterminent si celui-ci
existe du point de vue du joueur. Un hotspot inactif est ignoré lors de
la détection du point sélectionné.

### L'ordre des hotspots est important

Les hotspots sont testés dans leur ordre de déclaration. Le premier
hotspot actif dont le rectangle contient le point sélectionné est
retenu.

Ce comportement est utilisé intentionnellement pour les hotspots qui se
chevauchent. Un grand hotspot générique doit donc normalement être
déclaré **après** les hotspots plus petits et plus spécifiques qu'il
recouvre.

Par exemple :

``` text
HOTSPOT UNDERGROUND_X_FORM 141 190 17 16
    ...
END_HOTSPOT

HOTSPOT UNDERGROUND_GROUND 45 171 275 68
    ...
END_HOTSPOT
```

Placer le grand hotspot du sol en premier rendrait le petit hotspot en
forme de X inaccessible.

## 5. Messages d'examen

Un `MESSAGE` placé directement dans un `HOTSPOT`, en dehors d'un
`ACTION` ou d'un `USE`, définit le message affiché lorsque l'objet est
examiné :

``` text
HOTSPOT LIVINGROOM_FIREPLACE 163 72 26 17
    MESSAGE LIVINGROOM_FIREPLACE_EXAMINE
END_HOTSPOT
```

Ceci est différent d'un `MESSAGE` utilisé comme action :

``` text
ACTION
    MESSAGE CELLAR_DISABLE_ALARM_BOX
END_ACTION
```

Dans ce cas, le message est affiché lorsque le bloc d'action est
exécuté.

Un `MESSAGE_IMAGE` placé directement dans un `HOTSPOT` fonctionne de la
même manière, mais affiche une image des ressources de la pièce, centrée
au-dessus de la pièce, au lieu d'un texte :

``` text
HOTSPOT STUDY_PAINTING 120 40 60 45
    MESSAGE_IMAGE painting_closeup
END_HOTSPOT
```

Si un hotspot définit à la fois `MESSAGE` et `MESSAGE_IMAGE`, `MESSAGE`
est prioritaire et l'image n'est jamais affichée.

Un hotspot a au plus un `MESSAGE` et un `MESSAGE_IMAGE` en dehors de ses
blocs `ACTION` et `USE` : un second fait échouer le chargement de la
salle avec `duplicate MESSAGE in HOTSPOT <id>` (ou
`duplicate MESSAGE_IMAGE`).

## 6. Blocs ACTION

Un bloc `ACTION` décrit ce qui se produit lorsque le joueur effectue
l'action normale sur un hotspot :

``` text
HOTSPOT UNDERGROUND_SHOVEL 18 89 37 100
    WHEN INVENTORY_HAS SHOVEL false
    ACTION
        INVENTORY_ADD SHOVEL
    END_ACTION
END_HOTSPOT
```

Un `ACTION` peut lui-même comporter des conditions :

``` text
ACTION
    WHEN STATE_IS cellar_alarm_box_unscrewed true
    SET cellar_alarm_box_opened
END_ACTION
```

### Plusieurs blocs ACTION sont séquentiels

Les blocs `ACTION` ne sont **pas** des alternatives de type
`if / else if`.

Tous les blocs d'action dont les conditions correspondent sont évalués
dans leur ordre de déclaration. Leurs conditions sont réévaluées lorsque
chaque bloc est atteint ; un bloc précédent peut donc modifier l'état du
jeu et affecter un bloc suivant.

Ce comportement est intentionnel et utile pour les transitions d'état :

``` text
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

Si `diningroom_lasers_disabled` est un état de type `TOGGLE`, le premier
bloc le modifie. Les blocs suivants examinent alors sa **nouvelle**
valeur.

Ce comportement est différent de celui des blocs `USE` : les blocs
`ACTION` correspondants ne s'arrêtent pas après la première
correspondance.

## 7. Blocs USE

`USE` décrit l'utilisation d'un objet de l'inventaire sur un hotspot.

Syntaxe :

``` text
USE <item>
    [WHEN ...]
    <actions>
END_USE
```

Exemple :

``` text
USE SCREWDRIVER
    WHEN STATE_IS cellar_alarm_box_unscrewed false
    SET cellar_alarm_box_unscrewed
    MESSAGE CELLAR_OPEN_ALARM_BOX
END_USE
```

Un bloc `USE` contient directement les actions. Il n'y a **pas de bloc
`ACTION` imbriqué** dans un `USE`.

Incorrect :

``` text
USE SCREWDRIVER
    ACTION
        SET cellar_alarm_box_unscrewed
    END_ACTION
END_USE
```

Correct :

``` text
USE SCREWDRIVER
    SET cellar_alarm_box_unscrewed
END_USE
```

Cette distinction est importante car `ACTION` appartient à la grammaire
du hotspot ou du chemin englobant, et non à `USE`.

### Plusieurs blocs USE

Plusieurs blocs `USE` peuvent faire référence au même objet et utiliser
des conditions pour sélectionner le comportement approprié :

``` text
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

Contrairement aux blocs `ACTION` des hotspots, les blocs `USE` sont des
alternatives : le premier bloc correspondant à l'objet et à toutes ses
conditions est exécuté, puis le traitement de l'utilisation de l'objet
s'arrête.

### USE générique

`USE *` correspond à n'importe quel objet de l'inventaire :

``` text
HOTSPOT STUDY_DARK 0 0 320 240
    WHEN STATE_IS study_lights_on false
    MESSAGE STUDY_MESSAGE_NO_LIGHT
    USE *
        MESSAGE STUDY_USE_NO_LIGHT
    END_USE
END_HOTSPOT
```

Comme les blocs `USE` sont testés dans leur ordre de déclaration, un
bloc générique doit être placé après les gestionnaires d'objets plus
spécifiques lorsque les deux sont présents.

## 8. Chemins et sorties

Un `PATH` déclare une direction de déplacement disponible.

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

Exemple simple :

``` text
PATH EAST
    ACTION
        ROOM secondunderground
    END_ACTION
END_PATH
```

Un chemin peut comporter des conditions déterminant si la direction est
disponible :

``` text
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

Les conditions au niveau du `PATH` déterminent si le joueur peut
utiliser la sortie.

Les conditions placées dans les blocs `ACTION` d'un chemin déterminent
ce qui se produit après l'utilisation du chemin.

Les blocs `ACTION` d'un `PATH` suivent la même sémantique séquentielle
que les blocs `ACTION` d'un hotspot.

## 9. Actions disponibles

Les actions sont valides dans les blocs `ACTION` et `USE`.

  -----------------------------------------------------------------------
  Directive                           Effet
  ----------------------------------- -----------------------------------
  `SET <state>`                       Met à jour l'état selon son type
                                      déclaré. Pour un `TOGGLE`, inverse
                                      sa valeur actuelle.

  `INVENTORY_ADD <item>`              Ajoute un objet à l'inventaire.

  `INVENTORY_REMOVE <item>`           Retire un objet de l'inventaire.

  `MESSAGE <message_id>`              Affiche un message localisé du jeu.

  `MESSAGE_IMAGE <image>`             Affiche une image des ressources de
                                      la pièce, centrée au-dessus de la
                                      pièce.

  `SFX <name>`                        Démarre un effet sonore et poursuit
                                      immédiatement l'exécution.

  `WAIT_SFX <name>`                   Démarre un effet sonore et suspend
                                      l'exécution jusqu'à sa fin.

  `ROOM <room>`                       Passe à une autre pièce. **Termine
                                      le flux d'actions courant et doit
                                      être la dernière action de son
                                      bloc.**

  `TIMELINE <name>`                   Démarre une timeline et suspend
                                      l'exécution jusqu'à sa fin. Après
                                      `RETURN`, la suite du bloc
                                      s'exécute ; après `END`, elle est
                                      abandonnée.

  `MINIGAME <name>`                   Démarre un mini-jeu. **Termine le
                                      flux d'actions courant et doit être
                                      la dernière action de son bloc.**
  -----------------------------------------------------------------------

Les effets sonores peuvent être indiqués soit par un nom relatif au répertoire
de la pièce, soit par un chemin absolu `romfs:/` :

```text
SFX closet_open
WAIT_SFX metal_ladder
SFX romfs:/audio/title_choice.raw
```

Un nom relatif s'écrit **sans extension** : il est résolu depuis le
répertoire de la pièce et reçoit automatiquement l'extension `.raw`. Dans
la pièce `hall`, `SFX closet_open` joue `romfs:/rooms/hall/closet_open.raw`.
Un chemin absolu, qui commence par `romfs:/`, est utilisé exactement
tel qu'il est écrit : aucune extension n'est ajoutée, il faut donc
l'écrire en entier, `.raw` compris (`SFX romfs:/audio/title_choice.raw`).
Il permet de partager un son entre plusieurs pièces.

### SET ne signifie pas nécessairement « mettre à true »

L'effet de `SET` dépend du type de l'état.

Pour un état déclaré comme `TOGGLE`, `SET` **inverse sa valeur
actuelle** :

-   `false` devient `true` ;
-   `true` devient `false`.

Par exemple :

``` text
ACTION
    SET livingroom_piano_opened
END_ACTION
```

ouvre un piano fermé si `livingroom_piano_opened` vaut actuellement
`false`, et le ferme si l'état vaut actuellement `true`.

Ne pas interpréter `SET foo` comme l'équivalent de `foo = true` sans
vérifier la définition de l'état.

## 10. Effets sonores et flux d'actions

`SFX` démarre un effet sonore et poursuit immédiatement avec l'action
suivante :

``` text
SFX closet_open
SET closet_opened
```

`WAIT_SFX` démarre un effet sonore et attend qu'il se termine avant de
poursuivre avec l'action suivante **dans le même bloc** :

``` text
ACTION
    WAIT_SFX metal_ladder
    ROOM cellar
END_ACTION
```

Ne pas compter sur l'exécution de blocs `ACTION` ultérieurs après un
`WAIT_SFX`. Toute action devant suivre le son doit être placée après
`WAIT_SFX` dans le même bloc.

### Actions après une timeline

`TIMELINE` suspend le bloc de la même façon que `WAIT_SFX`, jusqu'à la
fin de la timeline :

``` text
ACTION
    SET cellar_door_opened
    TIMELINE cellar_discovery
    ROOM cellar
END_ACTION
```

- Si la timeline finit par `RETURN`, le joueur revient dans la pièce et
  la suite du bloc s'exécute : ici, il est envoyé dans `cellar`.
- Si elle finit par `END`, le jeu revient à l'écran titre et la suite du
  bloc est abandonnée.

Passer la timeline avec **B** atteint quand même son `RETURN` ou son
`END` final : le résultat est le même. Comme pour `WAIT_SFX`, les blocs
`ACTION` suivants ne sont pas exécutés après la timeline. Les actions
placées **avant** `TIMELINE` prennent effet tout de suite, mais le
joueur ne voit leur résultat qu'à son retour dans la pièce.

### Actions terminales

`ROOM` et `MINIGAME` terminent le flux d'actions courant.
Ils doivent donc toujours être la **dernière action de leur bloc**.

Correct :

``` text
ACTION
    WAIT_SFX door_open
    ROOM corridor
END_ACTION
```

Incorrect :

``` text
ACTION
    ROOM corridor
    MESSAGE UNREACHABLE_MESSAGE
END_ACTION
```

Le `MESSAGE` ne sera jamais exécuté car `ROOM` termine le flux
d'actions.

Ne pas placer `WAIT_SFX`, `ROOM`, `TIMELINE` ou `MINIGAME` après un
`MESSAGE` dans le même bloc. L'action suivante remplace immédiatement
l'état du message, qui ne sera donc pas affiché.

## 11. Exemple complet

``` text
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

# Hotspot générique volontairement déclaré après les hotspots plus spécifiques.
HOTSPOT UNDERGROUND_GROUND 45 171 275 68
    USE SHOVEL
        MESSAGE UNDERGROUND_DIG_ANYWHERE
    END_USE
END_HOTSPOT

# ---------------------------------------------------------------------------
# Sorties
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

## 12. Résumé du modèle d'exécution

Lorsqu'une pièce est active :

1.  Les images dont les conditions correspondent sont affichées.
2.  La détection des hotspots parcourt les hotspots actifs dans leur
    ordre de déclaration et sélectionne le premier rectangle
    correspondant.
3.  L'examen d'un hotspot utilise son `MESSAGE` direct, s'il existe,
    sinon son `MESSAGE_IMAGE` direct, s'il existe.
4.  Les actions normales d'un hotspot parcourent tous les blocs `ACTION`
    dans leur ordre de déclaration. Chaque bloc correspondant est
    exécuté, sauf si le flux d'actions est suspendu par `WAIT_SFX` ou
    `TIMELINE`, ou terminé par `ROOM` ou `MINIGAME`.
5.  L'utilisation d'un objet parcourt les blocs `USE` dans leur ordre de
    déclaration et exécute le premier bloc dont l'objet et les
    conditions correspondent.
6.  Un chemin n'existe que lorsque ses conditions au niveau du `PATH`
    correspondent ; son utilisation évalue ses blocs `ACTION` dans leur
    ordre de déclaration.

Les fichiers de pièce sont supposés être correctement écrits. Les
erreurs de syntaxe et d'exécution sont signalées via la sortie de debug
habituelle.

## 13. Erreurs courantes

### Imbriquer ACTION dans USE

Ne pas faire ceci :

``` text
USE KEY_ONE
    ACTION
        MESSAGE SOMETHING
    END_ACTION
END_USE
```

Les actions doivent être placées directement dans le bloc `USE`.

### Traiter les blocs ACTION comme des else-if

Plusieurs blocs `ACTION` correspondants peuvent être exécutés. Les
changements d'état effectués dans un bloc peuvent affecter les
conditions des blocs suivants.

### Oublier la priorité des hotspots

Un grand hotspot déclaré avant un hotspot plus petit qu'il recouvre peut
rendre ce dernier inaccessible.

### Oublier que les images conditionnelles sont indépendantes

Les images sont évaluées indépendamment. Ajouter des conditions
complémentaires lorsqu'une seule variante doit être visible.

### Supposer que SET force un booléen à true

Vérifier la définition de l'état. Sur un état `TOGGLE`, `SET` inverse la
valeur courante : `false` devient `true` et `true` devient `false`.

### Séparer WAIT_SFX et sa suite dans différents blocs ACTION

Les actions devant être exécutées après un `WAIT_SFX` doivent se trouver
dans le même bloc :

``` text
ACTION
    WAIT_SFX door_open
    ROOM hall
END_ACTION
```

### Placer des actions après ROOM ou MINIGAME

`ROOM` et `MINIGAME` terminent le flux d'actions courant. Rien ne doit
les suivre dans le même bloc.

### Compter sur l'exécution des actions après TIMELINE

Les actions placées après `TIMELINE` ne s'exécutent que si la timeline
finit par `RETURN`. Après un game over qui finit par `END`, elles sont
abandonnées.

### Donner deux messages d'examen à un hotspot

```text
HOTSPOT HALL_CLOSET 40 60 30 50
    MESSAGE HALL_CLOSET_EXAMINE
    MESSAGE HALL_CLOSET_EXAMINE_AGAIN
END_HOTSPOT
```

Le chargement de la salle échoue avec
`duplicate MESSAGE in HOTSPOT HALL_CLOSET`. Un hotspot n'a qu'un seul
message d'examen ; pour afficher des messages différents selon l'état du
jeu, utilisez des blocs `ACTION` conditionnels. Cela arrive typiquement
quand `scripts/add_events.sh` est lancé deux fois sur le même fichier.

## 14. Style recommandé

Pour améliorer la lisibilité, les fichiers de pièce doivent normalement
être organisés dans l'ordre suivant :

``` text
Images
Hotspots
Sorties
```

Utiliser des commentaires de section et indenter les directives
imbriquées de manière cohérente. Placer les hotspots spécifiques avant
les hotspots génériques qui les recouvrent. Conserver ensemble les
actions `WAIT_SFX` et les actions de transition associées.

Le format des pièces est volontairement un petit DSL spécifique au jeu,
et non un langage de script généraliste. Si une pièce nécessite un
comportement qui ne peut pas être exprimé proprement avec les primitives
existantes, préférer l'ajout d'une petite primitive réutilisable au
format.
