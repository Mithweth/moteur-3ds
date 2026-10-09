# Jouer à un jeu MOTEUR

Ce document présente les **conventions de jeu communes à MOTEUR**.

MOTEUR fournit les mécanismes nécessaires à une aventure — pièces,
interactions, inventaire, objets, messages, séquences et mini-jeux —
mais chaque jeu reste libre d'organiser son interface et ses commandes
comme il le souhaite. Il n'existe donc pas de mode d'emploi universel
pour les déplacements, la sélection des cibles, la sauvegarde ou les
mini-jeux.

Pour l'installation et la compilation, consultez le [README du
projet](../README.md) et [BUILD.fr.md](./BUILD.fr.md).

## 1. Les commandes communes

Trois boutons conservent une fonction commune dans les jeux utilisant
MOTEUR :

| Commande | Action                                |
|:---------|:--------------------------------------|
| **A**    | Utiliser l'objet sélectionné          |
| **X**    | Examiner l'objet sélectionné          |
| **B**    | Annuler / fermer / revenir en arrière |

Les autres commandes dépendent du jeu. Les déplacements, la navigation
dans l'inventaire, les interactions avec le décor ou l'ouverture d'un
éventuel menu peuvent donc être différents d'une aventure à l'autre.

## 2. L'inventaire

Un jeu MOTEUR peut proposer un inventaire contenant les objets récupérés
pendant l'aventure.

Lorsqu'un objet est sélectionné :

- **X** permet de l'examiner, lorsqu'il possède une action d'examen ;
- **A** permet de l'utiliser.

Selon l'objet et le jeu, son utilisation peut agir directement ou
s'appliquer à une **cible** sélectionnée dans la pièce.

Un objet peut disparaître de l'inventaire après utilisation s'il a été
placé, donné, consommé ou utilisé par l'aventure.

## 3. Examiner un objet

Appuyez sur **X** pour examiner l'objet actuellement sélectionné.

L'examen peut afficher un texte, une image ou déclencher un comportement
propre au jeu. Tous les objets ne proposent pas nécessairement une vue
ou une action particulière.

**B** permet de fermer l'examen ou de revenir au jeu.

## 4. Utiliser un objet

Appuyez sur **A** pour utiliser l'objet actuellement sélectionné.

MOTEUR permet deux types d'utilisation :

- un objet peut avoir une action qui lui est propre et être utilisé
  directement ;
- un objet peut être utilisé sur une **cible** de la pièce.

La manière de sélectionner cette cible dépend de l'interface choisie par
le jeu. Lorsqu'une combinaison objet/cible n'est pas prévue, le jeu peut
simplement la refuser ou afficher un message.

## 5. Messages et séquences

Les aventures peuvent afficher des messages, des images et des séquences
scénarisées.

**B** sert généralement à fermer, annuler ou quitter l'écran courant
lorsque cette opération est possible.

Le comportement exact d'une séquence — possibilité de l'accélérer, de la
passer ou de revenir au jeu — est défini par l'aventure.

## 6. Mini-jeux

MOTEUR permet à un jeu d'intégrer ses propres mini-jeux écrits en C.

Leurs commandes sont donc entièrement spécifiques au jeu. Lorsqu'un
mini-jeu permet d'être quitté, **B** conserve sa fonction habituelle de
retour ou d'annulation.

## 7. Déplacements, inventaire et interface

Le **stick circulaire** permet de se déplacer entre les pièces. Le jeu
ne permet un déplacement que dans une direction disponible depuis la
pièce courante.

La **croix directionnelle** permet de parcourir les objets de
l'inventaire et de choisir l'objet actuellement sélectionné. **A**
l'utilise et **X** l'examine.

L'aventure reste libre d'afficher ces informations comme elle le
souhaite et peut ajouter ses propres menus, indicateurs ou commandes. La
sauvegarde, par exemple, est optionnelle et sa présentation dépend du
jeu.

## 8. Sauvegarde

Lorsqu'une aventure active la sauvegarde, MOTEUR utilise **un seul
emplacement de sauvegarde**.

La partie sauvegardée peut être reprise avec **Continuer** depuis
l'écran titre. Sauvegarder à nouveau remplace la sauvegarde précédente.

**Commencer une nouvelle partie supprime la sauvegarde existante.** Il
n'est donc pas possible de conserver plusieurs parties ou de revenir à
une sauvegarde antérieure après avoir choisi de recommencer.

## 9. En résumé

| Commande                 | Fonction commune             |
|:-------------------------|:-----------------------------|
| **Stick circulaire**     | Se déplacer entre les pièces |
| **Croix directionnelle** | Parcourir l'inventaire       |
| **A**                    | Utiliser l'objet sélectionné |
| **X**                    | Examiner l'objet sélectionné |
| **B**                    | Annuler / fermer / revenir   |

Les commandes supplémentaires et leur rôle dépendent de l'aventure.
