# Fichiers de choix

Ce document décrit le format des fichiers ouverts par l'action de pièce
`CHOICE`.

Un choix affiche une question sur l'écran du bas avec une liste de
boutons. Le joueur en choisit un, et les actions de cette option
s'exécutent. Les choix peuvent s'imbriquer : ils servent aussi bien à de
petits menus (« Que faire ? ») qu'à simuler une conversation, où chaque
réponse mène à de nouvelles questions.

## 1. Emplacement et chargement

Un fichier de choix se place dans le répertoire de la pièce qui
l'utilise, à côté de son script `room`, et on le désigne par son nom de
fichier :

``` text
resources/rooms/sample1/room       HOTSPOT ... ACTION ... CHOICE dustbin
resources/rooms/sample1/dustbin    le fichier de choix
```

La compilation copie dans la romfs tous les fichiers **sans extension**
d'un répertoire de pièce : le fichier ci-dessus est donc lu depuis
`romfs:/rooms/sample1/dustbin`. Ne donnez pas d'extension à un fichier
de choix : `dustbin.txt` ne serait pas copié.

Le fichier est lu quand l'action `CHOICE` s'exécute, pas au chargement
de la pièce, et libéré quand le joueur quitte le choix le plus extérieur
ou la pièce. Un fichier illisible ou contenant une erreur est signalé
dans les logs, et l'action `CHOICE` est ignorée : le flux d'actions
continue avec l'action suivante.

## 2. Syntaxe générale

Les règles sont celles des fichiers de pièce (voir `ROOMS.fr.md`) :

- les éléments sont séparés par des **espaces** ;
- les espaces et tabulations en début et en fin de ligne sont ignorés :
  l'indentation sert seulement à la lisibilité ;
- les lignes vides et celles qui commencent par `#` sont ignorées ;
- les commandes sont sensibles à la casse ;
- une ligne ne doit pas dépasser 511 caractères.

## 3. Structure

Un fichier contient un ou plusieurs blocs `CHOICE` :

``` text
CHOICE MAIN
    PROMPT DUSTBIN_WHAT_TO_DO

    OPTION EMPTY_BIN
        MESSAGE FLEMME
    END_OPTION

    OPTION LEAVE
        EXIT_CHOICE
    END_OPTION
END_CHOICE
```

| Directive                    | Où                 | Description                                               |
|------------------------------|--------------------|-----------------------------------------------------------|
| `CHOICE <nom>`               | niveau du fichier  | Ouvre un bloc. `<nom>` est unique dans le fichier.        |
| `PROMPT <clé>`               | dans un `CHOICE`   | Clé de traduction de la question. Obligatoire, une seule fois. |
| `OPTION <clé>`               | dans un `CHOICE`   | Ouvre une option ; `<clé>` est la clé de traduction de son bouton, unique dans le bloc. |
| `WHEN <type> <nom> <valeur>` | dans une `OPTION`  | Condition d'affichage de l'option (voir 4).               |
| *action*                     | dans une `OPTION`  | Exécutée quand le joueur choisit l'option (voir 5).       |
| `END_OPTION`                 | dans une `OPTION`  | Ferme l'option.                                           |
| `END_CHOICE`                 | dans un `CHOICE`   | Ferme le bloc. Il faut un `PROMPT` et au moins une `OPTION`. |

Le bloc nommé **`MAIN`** est obligatoire : c'est lui qu'affiche l'action
`CHOICE` de la pièce. Les autres blocs ne sont accessibles que par une
action `CHOICE` placée dans une option (voir 6).

Les options s'affichent dans l'ordre du fichier.

Limites :

| Quoi                                   | Maximum |
|----------------------------------------|---------|
| blocs `CHOICE` par fichier             | 16      |
| options par bloc                       | 16      |
| conditions `WHEN` par option           | 8       |
| actions par option                     | 16      |
| choix ouverts en même temps (imbrication) | 8    |

Le nombre de boutons qui tiennent à l'écran dépend aussi de la taille du
texte réglée dans le fichier de jeu (voir section 8).

## 4. Conditions

`WHEN` fonctionne comme dans les fichiers de pièce, avec les mêmes
types :

``` text
OPTION THROW_CAN
    WHEN INVENTORY_HAS EMPTY_BEER true
    MESSAGE NO_PLACE
END_OPTION
```

Une option n'est affichée que si toutes ses conditions sont vraies. Une
option masquée n'a pas de bouton : la croix directionnelle et l'écran
tactile la sautent.

Les conditions sont réévaluées à chaque frame. Si une action change un
état ou l'inventaire, des options apparaissent ou disparaissent pendant
que le choix est affiché ; si l'option sélectionnée disparaît, la
sélection passe à la première option encore affichée.

Un choix qui n'a plus aucune option n'est pas affiché : il est quitté
aussitôt, comme si `EXIT_CHOICE` avait été exécuté (voir 6).

## 5. Actions

Une option accepte toutes les actions de pièce (`SET`, `INVENTORY_ADD`,
`INVENTORY_REMOVE`, `MESSAGE`, `MESSAGE_IMAGE`, `SFX`, `WAIT_SFX`,
`ROOM`, `TIMELINE`, `MINIGAME`), avec le même sens, décrites dans
[ROOMS.fr.md, section 9](./ROOMS.fr.md#9-actions-disponibles), plus deux
actions propres aux choix :

| Directive       | Effet                                                                    |
|-----------------|--------------------------------------------------------------------------|
| `CHOICE <nom>`  | Ouvre le bloc `<nom>` **du même fichier** par-dessus le choix courant.   |
| `EXIT_CHOICE`   | Quitte le choix courant (voir 6). Les actions qui le suivent dans l'option ne s'exécutent jamais. |

`MESSAGE_IMAGE` utilise les images de la pièce à laquelle appartient le
fichier. Un nom de mini-jeu ou d'image mal orthographié fait échouer le
chargement, comme dans un fichier de pièce.

Une option peut n'avoir aucune action : la choisir ne fait alors rien.

## 6. Flux d'actions

### Ouverture

L'action de pièce `CHOICE <fichier>` suspend le bloc de la pièce, comme
`MESSAGE` : le fichier est chargé et son bloc `MAIN` est affiché, avec la
première option affichée sélectionnée. Les blocs `ACTION` suivants du
hotspot ou du chemin ne sont pas exécutés.

### Choisir une option

Les actions de l'option s'exécutent. Une fois terminées, si l'option n'a
pas quitté le choix, **le même choix est réaffiché**, avec la même option
sélectionnée. Le joueur peut donc essayer plusieurs réponses, ce dont une
conversation a besoin.

Pendant qu'une action de l'option est en cours (un message, un son, une
timeline, un mini-jeu), le choix est masqué ; il revient une fois le flux
de l'option terminé ou suspendu par un choix imbriqué.

### Choix imbriqués

`CHOICE <nom>` dans une option ouvre un autre bloc du fichier
**par-dessus** le choix courant. Quand le joueur le quitte, le choix
parent est réaffiché et les actions qui suivaient `CHOICE <nom>` dans
l'option s'exécutent :

``` text
CHOICE MAIN
    PROMPT DUSTBIN_WHAT_TO_DO

    OPTION EMPTY_BIN
        CHOICE WHERE
        # Affiché après avoir quitté WHERE.
        MESSAGE BIN_STILL_FULL
    END_OPTION

    OPTION LEAVE
        EXIT_CHOICE
    END_OPTION
END_CHOICE

CHOICE WHERE
    PROMPT DUSTBIN_WHERE_TO_THROW

    # WHERE est réaffiché après le message.
    OPTION PAVEMENT
        MESSAGE WAY_TOO_FAR
    END_OPTION

    # Retour à MAIN.
    OPTION GIVE_UP
        EXIT_CHOICE
    END_OPTION
END_CHOICE
```

Un bloc peut se rouvrir lui-même ou rouvrir un de ses parents : chaque
`CHOICE` ajoute un niveau, jusqu'à 8. Au-delà, l'action `CHOICE` est
ignorée.

### Quitter un choix

`EXIT_CHOICE` et le bouton **B** ne quittent que le niveau courant :

- dans un choix imbriqué, le choix parent est réaffiché, avec sa
  première option affichée sélectionnée, et la suite de l'option qui
  avait ouvert le choix imbriqué s'exécute ;
- dans `MAIN`, le choix est fermé et le fichier libéré, puis la suite du
  bloc de la pièce qui contenait `CHOICE <fichier>` s'exécute.

Pour fermer tous les niveaux d'un coup, placez aussi `EXIT_CHOICE` après
le `CHOICE` imbriqué dans l'option parente.

Un choix sans aucun `EXIT_CHOICE` accessible peut toujours être quitté
avec B.

### Autres sorties

- `ROOM` change de pièce et ferme tous les choix.
- `TIMELINE` : après `RETURN`, le choix revient et la suite de l'option
  s'exécute ; après `END`, le jeu revient à l'écran titre.
- `MINIGAME` : le choix revient une fois le mini-jeu terminé.

## 7. Commandes

| Commande        | Effet                                                 |
|-----------------|-------------------------------------------------------|
| **↑ / ↓** (croix directionnelle ou stick circulaire) | déplace la sélection entre les options affichées |
| **A**           | choisit l'option sélectionnée                         |
| toucher         | choisit l'option touchée                              |
| **B**           | quitte le choix courant (comme `EXIT_CHOICE`)         |
| **START**       | ouvre le menu START par-dessus le choix               |

Tant qu'un choix est affiché, la pièce ne reçoit aucune commande : le
joueur ne peut ni se déplacer, ni toucher un hotspot, ni utiliser un
objet. Le compte à rebours du HUD, s'il existe, continue de tourner ;
quand le temps est écoulé, sa timeline démarre.

Le menu START est dessiné avec la même boîte. Le fermer ramène le choix
avec sa sélection. « Sauvegarder » n'est pas proposé tant qu'un choix est
ouvert, car la sauvegarde ne pourrait pas l'enregistrer : le joueur doit
d'abord quitter le choix.

## 8. Apparence

La boîte est dessinée avec les couleurs de cadre du fichier de jeu, et
ses autres réglages viennent du bloc `CHOICE` de ce fichier
(`FRAME_WIDTH`, couleurs et tailles du texte, couleur du bouton
sélectionné, marge) : voir `GAMESTART.fr.md`. Ils s'appliquent à tous les
choix et au menu START.

La question est découpée en lignes selon la largeur de la boîte ; les
libellés des boutons ne le sont pas : gardez-les courts.

## 9. Erreurs de chargement

Chaque erreur arrête le chargement avec `<fichier>:<ligne>: <message>`
dans les logs, et l'action `CHOICE` est ignorée.

| Message                         | Cause                                                              |
|---------------------------------|--------------------------------------------------------------------|
| `Cannot open choice file`       | pas de fichier de ce nom dans le répertoire de la pièce (nom mal orthographié, extension) |
| `invalid CHOICE declaration`    | nom manquant, plus de 16 blocs, ou `CHOICE` dans un bloc mais hors d'une `OPTION` |
| `duplicate CHOICE`              | deux blocs portent le même nom                                     |
| `invalid PROMPT`                | hors d'un bloc, dans une option, sans clé, ou répété               |
| `invalid OPTION`                | hors d'un bloc, dans une autre option, sans clé, ou plus de 16     |
| `duplicate OPTION`              | deux options ont la même clé dans un bloc                          |
| `END_OPTION without OPTION`     | `END_OPTION` sans option ouverte                                   |
| `invalid END_CHOICE`            | option non fermée, ou bloc sans `PROMPT` ou sans `OPTION`          |
| `invalid WHEN`                  | hors d'une option, argument manquant, ou plus de 8 conditions      |
| `unknown WHEN condition`        | type autre que `STATE_IS` ou `INVENTORY_HAS`                       |
| `action outside OPTION`         | une action directement dans un bloc                                |
| `invalid action`                | action inconnue, ou action sans argument                           |
| `unknown MESSAGE_IMAGE image`   | image absente des images de la pièce                               |
| `invalid MINIGAME`              | mini-jeu non enregistré dans `extensions/callbacks.c`              |
| `too many ACTION`               | plus de 16 actions dans une option                                 |
| `unclosed CHOICE or OPTION`     | `END_OPTION` ou `END_CHOICE` manquant en fin de fichier            |
| `missing CHOICE MAIN`           | aucun bloc nommé `MAIN`                                            |
| `invalid nested CHOICE`         | `CHOICE <nom>` dans une option désigne un bloc qui n'existe pas    |

## 10. Exemple complet

La pièce `sample1` déclare le hotspot :

``` text
HOTSPOT DUSTBIN 212 160 43 59
    WHEN STATE_IS raytracing_enabled false
    ACTION
        CHOICE dustbin
    END_ACTION
END_HOTSPOT
```

et `resources/rooms/sample1/dustbin` contient :

``` text
CHOICE MAIN
    PROMPT DUSTBIN_WHAT_TO_DO

    OPTION EMPTY_BIN
        CHOICE WHERE
    END_OPTION

    OPTION THROW_CAN
        WHEN INVENTORY_HAS EMPTY_BEER true
        MESSAGE NO_PLACE
    END_OPTION

    OPTION LEAVE
        EXIT_CHOICE
    END_OPTION
END_CHOICE

CHOICE WHERE
    PROMPT DUSTBIN_WHERE_TO_THROW

    OPTION NEIGHBOR_BIN
        MESSAGE TOO_FAR
    END_OPTION

    OPTION PAVEMENT
        MESSAGE WAY_TOO_FAR
    END_OPTION

    OPTION GIVE_UP
        EXIT_CHOICE
    END_OPTION
END_CHOICE
```

Chaque clé de `PROMPT`, `OPTION` et `MESSAGE` est définie dans chaque
fichier `.lang`.

## 11. Erreurs courantes

### Donner un nom de fichier à un CHOICE imbriqué

Dans un fichier de choix, `CHOICE <nom>` désigne un **bloc du même
fichier**, pas un autre fichier : `CHOICE autre_fichier` échoue avec
`invalid nested CHOICE`.

### Placer des actions après EXIT_CHOICE

``` text
OPTION LEAVE
    EXIT_CHOICE
    MESSAGE GOODBYE
END_OPTION
```

`GOODBYE` n'est jamais affiché. Placez les actions avant `EXIT_CHOICE`,
ou après le `CHOICE` qui a ouvert le choix (dans l'option parente ou
dans le bloc de la pièce).

### S'attendre à ce que le choix se ferme après une option

Une option qui n'exécute pas `EXIT_CHOICE` ramène le même choix. C'est
le comportement voulu pour les conversations ; ajoutez `EXIT_CHOICE` à la
fin de l'option pour fermer le choix.

### Masquer toutes les options

Si les conditions `WHEN` masquent toutes les options, le choix n'est
jamais affiché et le bloc de la pièce continue aussitôt. Gardez au moins
une option sans condition (en général celle qui permet de quitter).
