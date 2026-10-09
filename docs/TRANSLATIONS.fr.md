# Traductions

Ce document décrit le système de traduction utilisé par le moteur :
emplacement et format des fichiers de langue, paires clé/valeur, retours
à la ligne et champs spéciaux `LANG_NAME` et `ORDER`.

## 1. Fichiers de langue

Les traductions se trouvent dans :

``` text
resources/lang/
```

Lors de la compilation, ce répertoire est copié dans le RomFS du jeu et
devient :

``` text
romfs:/lang/
```

Par convention, un fichier porte l’extension `.lang`, par exemple :

``` text
resources/lang/en.lang
resources/lang/fr.lang
resources/lang/ja.lang
```

Le moteur parcourt les fichiers présents dans `romfs:/lang/` pour
construire la liste des langues disponibles. Il ne se base pas sur le
nom du fichier pour déterminer la langue ni son ordre d’affichage : ces
informations sont contenues dans le fichier lui-même.

L’extension n’est actuellement pas vérifiée par le moteur. Il est donc
préférable de ne placer dans `resources/lang/` que des fichiers de
langue.

Le moteur prend en charge au plus 16 langues. Les fichiers en trop sont
ignorés, avec `Too many languages` dans le log ; lesquels le sont dépend
de l'ordre dans lequel les fichiers sont listés, qui n'est pas défini.

## 2. Format général

Un fichier de langue est un simple fichier texte composé de paires :

``` text
CLE=Valeur
```

Par exemple :

``` ini
TITLE_GAME=Nouvelle partie
TITLE_CONTROLS=Commandes
HUD_OBJECT=Objet
HALL_CLOSET=Un vieux buffet en bois
```

La partie située avant le premier `=` est la **clé**. La partie située
après est la **valeur traduite**.

Le premier `=` rencontré sur la ligne sert de séparateur. Une valeur
peut donc elle-même contenir le caractère `=` :

``` ini
EXAMPLE=2 + 2 = 4
```

Le moteur recherche les traductions par leur clé. Ces clés sont
utilisées par les rooms, les timelines, le HUD, l’inventaire, l’écran
titre et, ponctuellement, par le code C.

Par exemple, si une room contient :

``` text
MESSAGE HALL_MESSAGE
```

le moteur cherche la clé `HALL_MESSAGE` dans la langue actuellement
chargée :

``` ini
HALL_MESSAGE=Il y a un message sous le tapis
```

Les clés doivent rester **identiques dans toutes les langues**. Seule
leur valeur change.

## 3. Espaces et mise en forme

Les espaces autour du `=` ne sont pas supprimés automatiquement.

Il faut donc écrire :

``` ini
TITLE_GAME=Nouvelle partie
```

et non :

``` ini
TITLE_GAME = Nouvelle partie
```

Dans le second cas, la clé serait `TITLE_GAME`, avec un espace final, et
ne correspondrait donc pas à `TITLE_GAME`.

De la même manière, un espace placé immédiatement après `=` fait partie
de la traduction.

Les lignes vides sont ignorées. Les lignes commençant par `#` sont des
commentaires :

``` ini
# Main menu
TITLE_GAME=Nouvelle partie
TITLE_CONTROLS=Commandes
```

Il n’existe pas de syntaxe de section : toutes les traductions
appartiennent au même espace de clés.

## 4. Retours à la ligne

Une traduction occupe une seule ligne physique dans le fichier. Pour
insérer un retour à la ligne dans le texte affiché, utiliser la séquence
`\n` :

``` ini
STUDY_NO_LIGHTBULB=Inutile ! Il n'y a pas\nd'ampoule...
```

Le moteur remplace chaque `\n` par un véritable retour à la ligne lors
du chargement.

Plusieurs retours à la ligne peuvent être utilisés :

``` ini
TITLE_INTRO_SCENE_1=Janvier 1985\n\nLe début de ma carrière
```

Il ne faut donc pas couper directement une valeur sur plusieurs lignes
dans le fichier :

``` text
# Incorrect
MESSAGE=Première ligne
Deuxième ligne
```

La seconde ligne ne possède pas de `=` et ne fait pas partie de
`MESSAGE`.

## 5. `LANG_NAME`

Chaque fichier doit définir la clé spéciale :

``` ini
LANG_NAME=Français
```

`LANG_NAME` est le nom de la langue tel qu’il apparaît dans le menu du
jeu.

Il doit être écrit **dans la langue qu’il désigne**, et non traduit
depuis la langue actuellement sélectionnée. Par exemple :

``` ini
# en.lang
LANG_NAME=English

# fr.lang
LANG_NAME=Français

# ja.lang
LANG_NAME=日本語
```

Lorsque le joueur change de langue, le nouveau fichier est chargé
immédiatement et le menu utilise le `LANG_NAME` de cette langue.

`LANG_NAME` est également une traduction normale : le moteur de
traduction la charge comme n’importe quelle autre paire clé/valeur. Son
rôle particulier vient du fait que l’écran titre l’utilise comme libellé
de l’entrée permettant de changer de langue. Avec un seul fichier de
langue, cette entrée est masquée (voir [TITLE.fr.md](./TITLE.fr.md)).

## 6. `ORDER`

Chaque fichier doit également définir :

``` ini
ORDER=10
```

`ORDER` détermine l’ordre dans lequel les langues sont parcourues.

Les fichiers sont triés par valeur numérique croissante. Par exemple :

``` text
en.lang    ORDER=10
fr.lang    ORDER=20
ja.lang    ORDER=30
```

donne l’ordre :

``` text
English -> Français -> 日本語 -> English -> ...
```

La langue ayant la plus petite valeur de `ORDER` devient la langue
initiale au démarrage du jeu.

Il est recommandé d’utiliser des valeurs distinctes et espacées, par
exemple `10`, `20`, `30`, afin de pouvoir insérer facilement une
nouvelle langue plus tard :

``` text
en.lang    ORDER=10
de.lang    ORDER=15
fr.lang    ORDER=20
ja.lang    ORDER=30
```

Si `ORDER` est absent, sa valeur est considérée comme `0`. Un fichier
sans `ORDER` risque donc de devenir la première langue. Pour cette
raison, chaque fichier de langue doit définir explicitement ce champ.

Comme `LANG_NAME`, `ORDER` est aussi chargé dans la table des
traductions lorsque le fichier complet est lu, mais sa fonction
particulière est de permettre au moteur de trier les langues
disponibles.

## 7. Exemple minimal

Un fichier de langue minimal peut ressembler à ceci :

``` ini
LANG_NAME=Français
ORDER=20

TITLE_GAME=Nouvelle partie
TITLE_CONTINUE=Continuer
TITLE_CONTROLS=Commandes

HUD_OBJECT=Objet
HUD_TARGET=Cible
HUD_INVENTORY=INVENTAIRE

GAME_CANNOT_USE_MESSAGE=Comment suis-je censé utiliser ça ?
```

La version anglaise conserverait exactement les mêmes clés :

``` ini
LANG_NAME=English
ORDER=10

TITLE_GAME=New Game
TITLE_CONTINUE=Continue
TITLE_CONTROLS=Controls

HUD_OBJECT=Object
HUD_TARGET=Target
HUD_INVENTORY=INVENTORY

GAME_CANNOT_USE_MESSAGE=How am I supposed to use that?
```

L’ordre des clés dans le fichier n’a pas d’importance pour leur
utilisation. `ORDER` peut techniquement apparaître n’importe où, mais il
est préférable de placer `LANG_NAME` et `ORDER` au début du fichier afin
de rendre son rôle immédiatement visible.

## 8. Traductions manquantes

Lorsqu’une clé demandée par le jeu n’existe pas dans la langue courante,
le moteur affiche **la clé elle-même**.

Par exemple, si le jeu demande :

``` text
HALL_CLOSET
```

mais que `HALL_CLOSET` manque dans le fichier chargé, l’écran affichera
:

``` text
HALL_CLOSET
```

Ce comportement permet de repérer facilement une traduction oubliée sans
provoquer d’erreur ni interrompre le jeu.

Il n’existe pas de fallback automatique vers une autre langue : chaque
fichier doit donc contenir toutes les clés nécessaires au jeu.

## 9. Ajouter une langue

Pour ajouter une traduction, il suffit de créer un nouveau fichier dans
`resources/lang/`, de lui attribuer un `LANG_NAME` et un `ORDER`, puis
d’y reprendre les clés existantes avec leurs nouvelles valeurs.

Par exemple :

``` ini
LANG_NAME=Deutsch
ORDER=15
TITLE_GAME=Neues Spiel
TITLE_CONTINUE=Fortsetzen
TITLE_CONTROLS=Steuerung
...
```

Aucune modification du code C ou de la configuration de l’écran titre
n’est nécessaire : le moteur découvre automatiquement les fichiers
présents dans le répertoire des langues.

Après l’ajout ou la modification d’un fichier, reconstruire le jeu
normalement :

``` sh
make
```

Le fichier sera copié dans le RomFS lors de la compilation.

## 10. Limites actuelles

Le système de traduction est volontairement simple. Les limites définies
actuellement par le moteur sont :

- **16 langues** au maximum ;
- **512 traductions** chargées pour une langue ;
- **1024 octets** pour la lecture d’une ligne du fichier.

Il n’existe pas de pluriels, de substitutions de variables ou de règles
grammaticales intégrées. Lorsqu’un texte doit être construit
dynamiquement, le code ou l’extension concernée assemble les traductions
nécessaires.

Le format est donc essentiellement un dictionnaire texte :

``` text
clé -> texte affiché
```

ce qui permet d’ajouter ou de modifier une traduction sans toucher au
fonctionnement des rooms, des timelines ou du moteur.
