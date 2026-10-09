# Compiler MOTEUR pour Nintendo 3DS

Ce document explique comment installer l'environnement de développement,
configurer un jeu utilisant **MOTEUR**, compiler les exécutables
Nintendo 3DS et créer un paquet installable `.cia` avec son icône, sa
bannière et son jingle.

MOTEUR est écrit en **C** et utilise **devkitPro / devkitARM**,
**libctru**, **Citro2D / Citro3D** et **libvorbisidec**. La compilation
est pilotée par GNU Make. **Les paramètres propres à votre jeu se
définissent dans `moteur.mk` : il n'est normalement pas nécessaire de
modifier le `Makefile`.**

## 1. Prérequis

Il faut disposer de :

- **Git** et **GNU Make** ;
- **devkitPro**, avec le groupe `3ds-dev` (devkitARM, libctru, Citro2D,
  Citro3D, `tex3ds`, `smdhtool`, etc.) ;
- **`3ds-libvorbisidec`**, pour la lecture des musiques Ogg Vorbis ;
- **`makerom`**, pour produire un `.cia` ;
- **`bannertool`**, pour créer ou régénérer la bannière du menu HOME ;
- éventuellement **FFmpeg**, pour préparer le jingle WAV de la bannière.

Les outils 3DS sont disponibles sous Linux, macOS et Windows. Consultez
la [procédure officielle de
devkitPro](https://devkitpro.org/wiki/Getting_Started) pour installer
son gestionnaire de paquets (`dkp-pacman` sous Linux/macOS ou
l'environnement MSYS2/devkitPro sous Windows).

### Linux : attention à GLIBC

Certaines versions récentes des outils Linux de devkitPro nécessitent
**GLIBC 2.38 ou plus récente**. Sur une distribution plus ancienne, des
exécutables peuvent refuser de démarrer même si leurs paquets sont
installés.

``` sh
ldd --version
```

N'essayez pas de remplacer manuellement la GLIBC de votre distribution
pour cette raison. Vous pouvez utiliser l'image Docker de devkitPro :

``` sh
docker run --rm -v "$(pwd):/work" -w /work devkitpro/devkitarm:latest make
```

Le répertoire courant est monté dans le conteneur : les fichiers générés
restent sur votre machine. Pour créer un `.cia` dans le conteneur,
`makerom` doit également y être disponible.

### Linux (Debian/Ubuntu) : script d'installation

Le projet fournit `scripts/install-env.sh`, destiné à préparer
l'environnement de compilation et les outils de packaging :

``` sh
sudo bash scripts/install-env.sh
source /etc/profile.d/devkit-env.sh
```

Ce script utilise `apt` et nécessite les privilèges administrateur. Il
ne contourne pas une éventuelle incompatibilité de GLIBC.

### Installation manuelle

Sous Linux et macOS, après installation du gestionnaire de paquets
devkitPro :

``` sh
sudo dkp-pacman -S 3ds-dev 3ds-libvorbisidec
```

Sous Windows, dans le terminal MSYS2/devkitPro :

``` sh
pacman -S 3ds-dev 3ds-libvorbisidec
```

Sur Linux, une installation standard utilise notamment :

``` sh
export DEVKITPRO=/opt/devkitpro
export DEVKITARM="$DEVKITPRO/devkitARM"
export PATH="$DEVKITPRO/tools/bin:$PATH"
```

Ces variables sont normalement configurées par l'installation. Pour
vérifier l'environnement :

``` sh
echo "$DEVKITPRO"
echo "$DEVKITARM"
command -v arm-none-eabi-gcc
command -v tex3ds
command -v smdhtool
```

Le `Makefile` exige que `DEVKITARM` soit défini. `makerom` et
`bannertool` doivent être accessibles dans le `PATH` uniquement pour les
opérations qui les utilisent.

## 2. Récupérer et configurer MOTEUR

Récupérez les sources du projet, puis placez-vous à la racine du dépôt,
là où se trouvent `Makefile` et `moteur.mk`. Toutes les commandes
ci-dessous s'exécutent depuis ce répertoire.

### Le rôle de `moteur.mk`

Le `Makefile` inclut `moteur.mk` au démarrage. **Pour adapter MOTEUR à
votre aventure, modifiez `moteur.mk`, pas le `Makefile`** :

``` makefile
TARGET              := moteur
APP_TITLE           := MOTEUR sample
APP_DESCRIPTION     := Point-and-click adventure
APP_AUTHOR          := Jean-Baptiste Langlois
APP_PRODUCT_CODE    := CTR-H-MOTR
APP_UNIQUE_ID       := 0xFAA71
APP_VERSION         := 1.0.0
```

Les six premières valeurs sont celles de l'exemple fourni ;
`APP_VERSION` est facultatif et vaut `1.0.0` par défaut si vous ne le
définissez pas.

| Variable           | Rôle                                                                                                   |
|--------------------|--------------------------------------------------------------------------------------------------------|
| `TARGET`           | Nom de base des fichiers produits : `$(TARGET).3dsx`, `.elf`, `.smdh` et `.cia`.                       |
| `APP_TITLE`        | Nom affiché dans les métadonnées de l'application.                                                     |
| `APP_DESCRIPTION`  | Description affichée avec l'icône.                                                                     |
| `APP_AUTHOR`       | Nom de l'auteur ou de l'éditeur affiché dans les métadonnées.                                          |
| `APP_PRODUCT_CODE` | Code produit du CIA, par exemple `CTR-H-MOTR` ; choisissez-en un adapté à votre jeu.                   |
| `APP_UNIQUE_ID`    | Identifiant numérique utilisé pour construire le Title ID du CIA. **Il doit être propre à votre jeu.** |
| `APP_VERSION`      | Version `major.minor.micro`, utilisée dans le code et dans les métadonnées du CIA ; défaut : `1.0.0`.  |

**Attention :** `TARGET` détermine les noms de fichiers, mais **ne
remplace pas** `APP_UNIQUE_ID`. Deux jeux peuvent avoir des noms
différents tout en entrant en conflit sur la console s'ils utilisent le
même identifiant de titre.

### Générer un `APP_UNIQUE_ID`

Pour chaque **nouveau jeu**, choisissez un identifiant puis conservez-le
pour toutes ses mises à jour. Ne régénérez pas un ID à chaque
compilation : une nouvelle valeur ferait apparaître le jeu comme un
titre différent sur la console.

Vous pouvez tirer une valeur aléatoire dans la plage
`0xF8000`–`0xFEFFF`, couramment utilisée pour les homebrews afin
d'éviter la plage habituelle des applications commerciales :

``` sh
python3 -c 'import secrets; print(f"0x{secrets.randbelow(0x7000) + 0xF8000:05X}")'
```

Copiez la valeur obtenue dans `moteur.mk` :

``` makefile
APP_UNIQUE_ID := 0xF8ABC
```

**Un tirage aléatoire ne garantit pas l'unicité.** Vérifiez que
l'identifiant n'est pas déjà employé par un autre titre que vous
souhaitez installer, en particulier avant de distribuer votre jeu.
L'identifiant `0xF1F3A` présent dans l'exemple n'est pas une valeur à
réutiliser pour tous les projets MOTEUR.

Pour les plages d'identifiants et le fonctionnement des Title IDs, voir
[3dbrew — Titles](https://www.3dbrew.org/wiki/Titles).

### Version de l'application

La version est définie par `APP_VERSION` dans `moteur.mk`, ou peut être
remplacée ponctuellement en ligne de commande :

``` sh
make clean
make APP_VERSION=1.2.3
make cia APP_VERSION=1.2.3
```

Le `Makefile` décompose cette valeur en trois nombres pour `makerom`. Le
format CIA impose les plages **major : 0–63**, **minor : 0–63** et
**micro : 0–15**. Une version telle que `1.2.16` ne convient donc pas
pour le packaging CIA. Après un changement de version, une recompilation
propre évite de conserver un ancien `VERSION` dans les fichiers objets.

## 3. Compiler le jeu (`.3dsx`)

``` sh
make
```

Avec `TARGET := moteur`, les principaux fichiers produits sont :

| Fichier       | Utilisation                                                                           |
|---------------|---------------------------------------------------------------------------------------|
| `moteur.3dsx` | Exécutable pour le **Homebrew Launcher** et les émulateurs compatibles.               |
| `moteur.smdh` | Métadonnées et icône du programme.                                                    |
| `moteur.elf`  | Exécutable intermédiaire, notamment utile au débogage et nécessaire au packaging CIA. |

Le `Makefile` prépare également les ressources. Il génère un `gfx.t3s`
dans chaque répertoire de `resources/` contenant des PNG (hors
`resources/cia/`), convertit les spritesheets en `gfx.t3x` avec `tex3ds`
et assemble `romfs/` avec les pièces, timelines, langues, sons,
inventaire, HUD et autres données du jeu.

**Il n'est pas nécessaire de construire la RomFS manuellement** : elle
est intégrée au `.3dsx` pendant la compilation.

### Nettoyer et recompiler

``` sh
make clean
make
```

`make clean` supprime les objets et ressources **générés** (`build/`,
`romfs/`, les exécutables et les `gfx.t3s` produits automatiquement),
sans supprimer les ressources sources. Il ne supprime pas
`resources/cia/banner.bnr`.

### Debug et analyse statique

``` sh
make clean
make DEBUG=1
```

Cette commande définit la macro C `DEBUG`. Une cible d'analyse statique
est également disponible :

``` sh
make lint
```

Elle active l'analyseur GCC `-fanalyzer` et ne remplace pas un test sur
console ou émulateur.

## 4. Icône et bannière du menu HOME

Les fichiers de packaging se trouvent dans **`resources/cia/`**. Ils
sont distincts des images des rooms : ils ne sont pas transformés en
spritesheets par `tex3ds`.

| Fichier                    | Format et dimensions                     | Utilisation                                            |
|----------------------------|------------------------------------------|--------------------------------------------------------|
| `resources/cia/icon.png`   | **PNG 48 × 48 pixels**                   | Icône de l'application, intégrée au `.smdh`.           |
| `resources/cia/banner.png` | **PNG 256 × 128 pixels, exactement**     | Image de la bannière du menu HOME.                     |
| `resources/cia/banner.wav` | **WAV PCM 16 bits, stéréo, 3 s maximum** | Court jingle joué lorsque le titre est sélectionné.    |
| `resources/cia/banner.bnr` | Binaire généré                           | Bannière finale (image et son) incluse dans le `.cia`. |
| `resources/cia/app.rsf`    | Texte de configuration                   | Modèle de configuration utilisé par `makerom`.         |

Ces dimensions sont celles **des images sources attendues par les
outils**, pas des limites de taille en octets : préparez une icône de 48
× 48 et une bannière de 256 × 128. Le format SMDH contient en interne
deux représentations de l'icône, en 48 × 48 et 24 × 24 pixels. Le chemin
de l'icône est fixé à `resources/cia/icon.png` dans le `Makefile`
actuel.

### Créer l'icône

Créez un PNG **48 × 48** et enregistrez-le sous :

``` text
resources/cia/icon.png
```

Le `Makefile` transmet automatiquement cette image à `smdhtool` pour
produire `$(TARGET).smdh`. **Il n'est pas nécessaire d'exécuter
`bannertool makesmdh`** : l'icône et les métadonnées sont déjà prises en
charge par la compilation normale.

### Créer la bannière

Créez un PNG **256 × 128** (transparence possible) et placez-le ici :

``` text
resources/cia/banner.png
```

Il s'agit de l'image du **menu HOME**, pas de l'écran titre affiché dans
le jeu. La cible `make banner` utilise cette image avec le WAV décrit
ci-dessous pour générer `resources/cia/banner.bnr`.

### Préparer le jingle WAV du CIA

La bannière peut jouer un son court lorsque le joueur sélectionne
l'application dans le menu HOME. Pour une bonne compatibilité, utilisez
un **WAV PCM signé 16 bits, stéréo, 44 100 Hz, d'une durée maximale de 3
secondes**. Le son est incorporé à `banner.bnr` par `bannertool` ; ce
fichier n'est **pas** la musique de l'aventure lue par MOTEUR.

Par exemple, avec FFmpeg, pour convertir un extrait audio :

``` sh
ffmpeg -i jingle.ogg -t 3 -ar 44100 -ac 2 -c:a pcm_s16le resources/cia/banner.wav
```

Si vous ne voulez aucun son, vous pouvez fournir un court WAV silencieux
:

``` sh
ffmpeg -f lavfi -i anullsrc=r=44100:cl=stereo -t 1 -c:a pcm_s16le resources/cia/banner.wav
```

La commande `make banner` du projet passe explicitement
`-a resources/cia/banner.wav` à `bannertool` : prévoyez donc un fichier
WAV, même silencieux. La limite de **3 secondes** vient du format audio
de la bannière 3DS ; la dépasser peut empêcher sa lecture correcte
([3dbrew — CBMD](https://www.3dbrew.org/wiki/CBMD)).

### Générer `banner.bnr`

Après avoir préparé le PNG et le WAV :

``` sh
make banner
```

Cette cible exécute l'équivalent de :

``` sh
bannertool makebanner \
    -i resources/cia/banner.png \
    -a resources/cia/banner.wav \
    -o resources/cia/banner.bnr
```

**`make cia` ne lance pas `make banner` automatiquement.** La bannière
binaire est volontairement générée séparément pour qu'une compilation
CIA (notamment en CI) ne dépende que de `makerom`. Après modification de
`banner.png` ou `banner.wav`, relancez `make banner` et conservez le
`banner.bnr` obtenu dans les sources du jeu.

## 5. Produire un paquet installable (`.cia`)

Le format `.cia` permet d'installer un jeu MOTEUR dans le menu HOME
d'une Nintendo 3DS disposant d'un environnement homebrew adapté.

La génération nécessite **`makerom`**, disponible notamment dans
[Project_CTR](https://github.com/3DSGuy/Project_CTR). `bannertool` n'est
nécessaire que pour créer ou modifier `banner.bnr`.

Compilez d'abord le projet, puis générez le CIA :

``` sh
make
make cia
```

Avec la configuration d'exemple, le résultat est `moteur.cia` ; pour un
autre jeu, il s'appellera `$(TARGET).cia`.

**Important :** dans le `Makefile` actuel, la cible `cia` **ne dépend
pas de la cible `all`**. Elle utilise les fichiers `.elf`, `.smdh` et la
RomFS déjà générés. Exécutez donc `make` avant `make cia`,
particulièrement après un changement de ressources ou de code.

Pour empaqueter le CIA, `makerom` reçoit notamment :

- l'exécutable `$(TARGET).elf` et son icône `$(TARGET).smdh` ;
- `resources/cia/banner.bnr` et `resources/cia/app.rsf` ;
- les variables `APP_TITLE`, `APP_PRODUCT_CODE` et `APP_UNIQUE_ID`
  définies dans `moteur.mk` ;
- les trois composantes de `APP_VERSION` et le répertoire `romfs/`.

## 6. Tester le jeu

### Avec un émulateur

Le fichier `.3dsx` peut être ouvert dans un émulateur Nintendo 3DS
compatible, tel qu'**Azahar**.

Sous Linux, si Azahar est installé via Flatpak :

``` sh
make
make run
```

La cible `run` utilise `org.azahar_emu.Azahar` et lui donne un accès en
lecture au répertoire du projet. Vous pouvez également lancer
l'émulateur directement :

``` sh
flatpak run --filesystem="$(pwd):ro" org.azahar_emu.Azahar "$(pwd)/moteur.3dsx"
```

Dans cette dernière commande, remplacez `moteur.3dsx` par
`$(TARGET).3dsx` si vous avez changé le nom du projet.

### Sur une Nintendo 3DS

Deux possibilités :

- **`.3dsx`** : copiez `$(TARGET).3dsx` sur la carte SD, par exemple
  dans `/3ds/mon-jeu/`, puis lancez-le depuis le **Homebrew Launcher** ;
- **`.cia`** : copiez `$(TARGET).cia` sur la carte SD, installez-le avec
  un gestionnaire de titres adapté (par exemple **FBI**) et lancez-le
  depuis le menu HOME.

Les deux formats contiennent les ressources RomFS nécessaires au jeu.
Vous n'avez pas à copier séparément `resources/` sur la carte SD.

## 7. Problèmes courants

| Symptôme                                           | Vérification                                                                                   |
|----------------------------------------------------|------------------------------------------------------------------------------------------------|
| `Please set DEVKITARM in your environment`         | Vérifiez `DEVKITPRO` et `DEVKITARM`, puis rechargez l'environnement devkitPro.                 |
| `arm-none-eabi-gcc: command not found`             | Vérifiez l'installation du groupe `3ds-dev` et votre `PATH`.                                   |
| `tex3ds: command not found`                        | Vérifiez les outils devkitPro installés et leur présence dans le `PATH`.                       |
| `GLIBC_2.38 not found` (ou erreur analogue)        | Les outils installés nécessitent une GLIBC plus récente ; envisagez le conteneur devkitPro.    |
| Erreur de liaison concernant `vorbisidec` ou `ogg` | Vérifiez `3ds-libvorbisidec` et ses dépendances.                                               |
| `makerom not found in PATH`                        | Installez `makerom` pour le `.cia`, ou compilez seulement le `.3dsx`.                          |
| `Missing resources/cia/banner.bnr`                 | Préparez `banner.png` et `banner.wav`, puis exécutez `make banner`.                            |
| `bannertool` refuse la bannière                    | Vérifiez le PNG **256 × 128** et le WAV **PCM 16 bits, stéréo, ≤ 3 s**.                        |
| L'icône n'apparaît pas comme prévu                 | Vérifiez `resources/cia/icon.png` (**48 × 48**), puis recompilez pour régénérer le `.smdh`.    |
| Le CIA remplace une autre application              | Vérifiez que votre `APP_UNIQUE_ID` n'entre pas en conflit avec celui d'un titre déjà installé. |
| Le CIA contient une ancienne version du jeu        | Exécutez `make` **avant** `make cia` ; utilisez `make clean` après un changement de version.   |
| Les images ou ressources semblent obsolètes        | Exécutez `make clean`, puis recompilez.                                                        |

## 8. Pour aller plus loin

Pour comprendre les formats de ressources, la définition des pièces, les
interactions, les timelines et les extensions C, consultez
[DEVELOPING.fr.md](./DEVELOPING.fr.md).

Pour les commandes et conventions de jeu par défaut, consultez
[HOWTOPLAY.fr.md](./HOWTOPLAY.fr.md).
