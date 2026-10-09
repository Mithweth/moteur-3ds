# Building MOTEUR for Nintendo 3DS

This document explains how to install the development environment,
configure a game using **MOTEUR**, build Nintendo 3DS executables, and
create an installable `.cia` package with its icon, banner, and jingle.

MOTEUR is written in **C** and uses **devkitPro / devkitARM**,
**libctru**, **Citro2D / Citro3D**, and **libvorbisidec**. The build is
driven by GNU Make. **Game-specific settings are defined in `moteur.mk`;
you should normally not need to modify the `Makefile`.**

## 1. Requirements

You will need:

- **Git** and **GNU Make**;
- **devkitPro**, with the `3ds-dev` package group (devkitARM, libctru,
  Citro2D, Citro3D, `tex3ds`, `smdhtool`, etc.);
- **`3ds-libvorbisidec`**, for Ogg Vorbis music playback;
- **`makerom`**, to produce a `.cia`;
- **`bannertool`**, to create or regenerate the HOME Menu banner;
- optionally **FFmpeg**, to prepare the banner WAV jingle.

The 3DS development tools are available on Linux, macOS, and Windows.
See the [official devkitPro installation
guide](https://devkitpro.org/wiki/Getting_Started) to install its
package manager (`dkp-pacman` on Linux/macOS, or the MSYS2/devkitPro
environment on Windows).

### Linux: GLIBC requirements

Some recent versions of the devkitPro Linux tools require **GLIBC 2.38
or newer**. On an older distribution, executables may refuse to start
even when their packages are correctly installed.

``` sh
ldd --version
```

Do not try to replace your distribution's GLIBC manually for this
reason. You can use the devkitPro Docker image instead:

``` sh
docker run --rm -v "$(pwd):/work" -w /work devkitpro/devkitarm:latest make
```

The current directory is mounted into the container, so generated files
remain on your machine. To create a `.cia` inside the container,
`makerom` must also be available there.

### Linux (Debian/Ubuntu): installation script

The project provides `scripts/install-env.sh` to prepare the build
environment and packaging tools:

``` sh
sudo bash scripts/install-env.sh
source /etc/profile.d/devkit-env.sh
```

This script uses `apt` and requires administrative privileges. It does
not work around a possible GLIBC incompatibility.

### Manual installation

On Linux and macOS, after installing the devkitPro package manager:

``` sh
sudo dkp-pacman -S 3ds-dev 3ds-libvorbisidec
```

On Windows, from the MSYS2/devkitPro terminal:

``` sh
pacman -S 3ds-dev 3ds-libvorbisidec
```

On Linux, a standard installation typically uses:

``` sh
export DEVKITPRO=/opt/devkitpro
export DEVKITARM="$DEVKITPRO/devkitARM"
export PATH="$DEVKITPRO/tools/bin:$PATH"
```

These variables are normally configured by the installation. To check
your environment:

``` sh
echo "$DEVKITPRO"
echo "$DEVKITARM"
command -v arm-none-eabi-gcc
command -v tex3ds
command -v smdhtool
```

The `Makefile` requires `DEVKITARM` to be defined. `makerom` and
`bannertool` only need to be available in the `PATH` for the operations
that use them.

## 2. Getting and configuring MOTEUR

Get the project sources, then move to the repository root, where
`Makefile` and `moteur.mk` are located. All commands below are run from
this directory.

### The role of `moteur.mk`

The `Makefile` includes `moteur.mk` when it starts. **To adapt MOTEUR to
your adventure, edit `moteur.mk`, not the `Makefile`:**

``` makefile
TARGET              := moteur
APP_TITLE           := MOTEUR sample
APP_DESCRIPTION     := Point-and-click adventure
APP_AUTHOR          := Jean-Baptiste Langlois
APP_PRODUCT_CODE    := CTR-H-MOTR
APP_UNIQUE_ID       := 0xFAA71
APP_VERSION         := 1.0.0
```

The first six values are those used by the supplied sample;
`APP_VERSION` is optional and defaults to `1.0.0` if it is not defined.

| Variable           | Purpose                                                                                |
|:-------------------|:---------------------------------------------------------------------------------------|
| `TARGET`           | Base name of the generated files: `$(TARGET).3dsx`, `.elf`, `.smdh`, and `.cia`.       |
| `APP_TITLE`        | Name displayed in the application metadata.                                            |
| `APP_DESCRIPTION`  | Description displayed with the icon.                                                   |
| `APP_AUTHOR`       | Author or publisher name displayed in the metadata.                                    |
| `APP_PRODUCT_CODE` | CIA product code, for example `CTR-H-MOTR`; choose one appropriate for your game.      |
| `APP_UNIQUE_ID`    | Numeric identifier used to build the CIA Title ID. **It must be unique to your game.** |
| `APP_VERSION`      | `major.minor.micro` version used in the code and CIA metadata; default: `1.0.0`.       |

**Important:** `TARGET` determines file names, but it does **not**
replace `APP_UNIQUE_ID`. Two games may have different names and still
conflict on the console if they use the same title identifier.

### Generating an `APP_UNIQUE_ID`

For each **new game**, choose an identifier and keep it for all future
updates. Do not generate a new ID for every build: changing it would
make the console treat the game as a different title.

You can generate a random value in the `0xF8000`–`0xFEFFF` range,
commonly used by homebrew applications to avoid the usual commercial
application range:

``` sh
python3 -c 'import secrets; print(f"0x{secrets.randbelow(0x7000) + 0xF8000:05X}")'
```

Copy the generated value into `moteur.mk`:

``` makefile
APP_UNIQUE_ID := 0xF8ABC
```

**A random value does not guarantee uniqueness.** Check that the
identifier is not already used by another title you intend to install,
especially before distributing your game.

The MOTEUR sample itself uses **`0xFAA71`**. Do not reuse this ID for
another MOTEUR-based game; generate a different one for your project.

For identifier ranges and Title ID details, see [3dbrew —
Titles](https://www.3dbrew.org/wiki/Titles).

### Application version

The version is defined by `APP_VERSION` in `moteur.mk`, or it can be
overridden temporarily on the command line:

``` sh
make clean
make APP_VERSION=1.2.3
make cia APP_VERSION=1.2.3
```

The `Makefile` splits this value into three numbers for `makerom`. The
CIA format limits them to **major: 0–63**, **minor: 0–63**, and **micro:
0–15**. A version such as `1.2.16` is therefore not valid for CIA
packaging. After changing the version, a clean rebuild avoids keeping an
old `VERSION` value in object files.

## 3. Building the game (`.3dsx`)

``` sh
make
```

With `TARGET := moteur`, the main generated files are:

| File          | Purpose                                                                       |
|:--------------|:------------------------------------------------------------------------------|
| `moteur.3dsx` | Executable for the **Homebrew Launcher** and compatible emulators.            |
| `moteur.smdh` | Application metadata and icon.                                                |
| `moteur.elf`  | Intermediate executable, useful for debugging and required for CIA packaging. |

The `Makefile` also prepares the resources. It generates a `gfx.t3s`
file in each `resources/` directory containing PNG files (except
`resources/cia/`), converts spritesheets to `gfx.t3x` with `tex3ds`, and
assembles `romfs/` with rooms, timelines, languages, sounds, inventory,
HUD, and other game data.

**You do not need to build the RomFS manually:** it is embedded in the
`.3dsx` during compilation.

### Cleaning and rebuilding

``` sh
make clean
make
```

`make clean` removes **generated** objects and resources (`build/`,
`romfs/`, executables, and automatically generated `gfx.t3s` files)
without deleting source resources. It does not remove
`resources/cia/banner.bnr`.

### Debugging and static analysis

``` sh
make clean
make DEBUG=1
```

This defines the C macro `DEBUG`. A static-analysis target is also
available:

``` sh
make lint
```

It enables GCC's `-fanalyzer`; it does not replace testing on a console
or emulator.

## 4. HOME Menu icon and banner

Packaging files are stored in **`resources/cia/`**. They are separate
from room images and are not converted into spritesheets by `tex3ds`.

| File                       | Format and dimensions                         | Purpose                                                          |
|:---------------------------|:----------------------------------------------|:-----------------------------------------------------------------|
| `resources/cia/icon.png`   | **PNG, 48 × 48 pixels**                       | Application icon embedded in the `.smdh`.                        |
| `resources/cia/banner.png` | **PNG, exactly 256 × 128 pixels**             | HOME Menu banner image.                                          |
| `resources/cia/banner.wav` | **16-bit PCM WAV, stereo, 3 seconds maximum** | Short jingle played when the title is selected.                  |
| `resources/cia/banner.bnr` | Generated binary                              | Final banner containing image and sound, included in the `.cia`. |
| `resources/cia/app.rsf`    | Configuration text                            | Configuration template used by `makerom`.                        |

These dimensions are the **source image dimensions expected by the
tools**, not byte-size limits: prepare a 48 × 48 icon and a 256 × 128
banner. Internally, the SMDH format contains two icon representations,
at 48 × 48 and 24 × 24 pixels. The current `Makefile` expects the icon
at `resources/cia/icon.png`.

### Creating the icon

Create a **48 × 48** PNG and save it as:

``` text
resources/cia/icon.png
```

The `Makefile` automatically passes this image to `smdhtool` to generate
`$(TARGET).smdh`. **You do not need to run `bannertool makesmdh`**: the
icon and metadata are already handled by the normal build.

### Creating the banner

Create a **256 × 128** PNG (transparency is allowed) and save it as:

``` text
resources/cia/banner.png
```

This is the **HOME Menu** banner image, not the title screen displayed
inside the game. The `make banner` target uses this image together with
the WAV described below to generate `resources/cia/banner.bnr`.

### Preparing the CIA banner WAV

The banner can play a short sound when the player selects the
application in the HOME Menu. For good compatibility, use a **signed
16-bit PCM WAV, stereo, 44,100 Hz, no longer than 3 seconds**. The sound
is embedded in `banner.bnr` by `bannertool`; this file is **not**
adventure music played by MOTEUR.

For example, with FFmpeg:

``` sh
ffmpeg -i jingle.ogg -t 3 -ar 44100 -ac 2 -c:a pcm_s16le resources/cia/banner.wav
```

If you do not want any sound, you can provide a short silent WAV:

``` sh
ffmpeg -f lavfi -i anullsrc=r=44100:cl=stereo -t 1 -c:a pcm_s16le resources/cia/banner.wav
```

The project's `make banner` command explicitly passes
`-a resources/cia/banner.wav` to `bannertool`, so provide a WAV file
even if it is silent. The **3-second** limit comes from the 3DS banner
audio format; exceeding it may prevent correct playback ([3dbrew —
CBMD](https://www.3dbrew.org/wiki/CBMD)).

### Generating `banner.bnr`

After preparing the PNG and WAV:

``` sh
make banner
```

This target runs the equivalent of:

``` sh
bannertool makebanner \
    -i resources/cia/banner.png \
    -a resources/cia/banner.wav \
    -o resources/cia/banner.bnr
```

**`make cia` does not run `make banner` automatically.** The binary
banner is deliberately generated separately so that CIA builds,
especially in CI, only depend on `makerom`. After changing `banner.png`
or `banner.wav`, run `make banner` again and keep the resulting
`banner.bnr` with the game's sources.

## 5. Building an installable package (`.cia`)

The `.cia` format allows a MOTEUR game to be installed in the HOME Menu
of a Nintendo 3DS with a suitable homebrew environment.

Generating it requires **`makerom`**, available notably from
[Project_CTR](https://github.com/3DSGuy/Project_CTR). `bannertool` is
only required to create or modify `banner.bnr`.

Build the project first, then generate the CIA:

``` sh
make
make cia
```

With the sample configuration, the result is `moteur.cia`; for another
game it will be named `$(TARGET).cia`.

**Important:** in the current `Makefile`, the `cia` target does **not**
depend on the `all` target. It uses the already generated `.elf`,
`.smdh`, and RomFS files. Run `make` before `make cia`, especially after
changing resources or code.

For CIA packaging, `makerom` receives, among other things:

- the `$(TARGET).elf` executable and its `$(TARGET).smdh` icon;
- `resources/cia/banner.bnr` and `resources/cia/app.rsf`;
- the `APP_TITLE`, `APP_PRODUCT_CODE`, and `APP_UNIQUE_ID` variables
  defined in `moteur.mk`;
- the three components of `APP_VERSION` and the `romfs/` directory.

## 6. Testing the game

### With an emulator

The `.3dsx` file can be opened in a compatible Nintendo 3DS emulator
such as **Azahar**.

On Linux, if Azahar is installed through Flatpak:

``` sh
make
make run
```

The `run` target uses `org.azahar_emu.Azahar` and grants it read access
to the project directory. You can also launch the emulator directly:

``` sh
flatpak run --filesystem="$(pwd):ro" org.azahar_emu.Azahar "$(pwd)/moteur.3dsx"
```

In the latter command, replace `moteur.3dsx` with `$(TARGET).3dsx` if
you changed the project name.

### On a Nintendo 3DS

There are two options:

- **`.3dsx`**: copy `$(TARGET).3dsx` to the SD card, for example into
  `/3ds/my-game/`, then launch it from the **Homebrew Launcher**;
- **`.cia`**: copy `$(TARGET).cia` to the SD card, install it with a
  suitable title manager such as **FBI**, then launch it from the HOME
  Menu.

Both formats contain the RomFS resources required by the game. You do
not need to copy `resources/` separately to the SD card.

## 7. Troubleshooting

| Symptom                                       | Check                                                                                 |
|:----------------------------------------------|:--------------------------------------------------------------------------------------|
| `Please set DEVKITARM in your environment`    | Check `DEVKITPRO` and `DEVKITARM`, then reload the devkitPro environment.             |
| `arm-none-eabi-gcc: command not found`        | Check that the `3ds-dev` package group is installed and available in your `PATH`.     |
| `tex3ds: command not found`                   | Check the installed devkitPro tools and their presence in the `PATH`.                 |
| `GLIBC_2.38 not found` or similar             | The installed tools require a newer GLIBC; consider using the devkitPro container.    |
| Linker error involving `vorbisidec` or `ogg`  | Check `3ds-libvorbisidec` and its dependencies.                                       |
| `makerom not found in PATH`                   | Install `makerom` to build the `.cia`, or build only the `.3dsx`.                     |
| `Missing resources/cia/banner.bnr`            | Prepare `banner.png` and `banner.wav`, then run `make banner`.                        |
| `bannertool` rejects the banner               | Check that the PNG is **256 × 128** and the WAV is **16-bit PCM, stereo, ≤ 3 s**.     |
| The icon does not appear as expected          | Check `resources/cia/icon.png` (**48 × 48**), then rebuild to regenerate the `.smdh`. |
| The CIA replaces another application          | Check that your `APP_UNIQUE_ID` does not conflict with another installed title.       |
| The CIA contains an older version of the game | Run `make` **before** `make cia`; use `make clean` after changing the version.        |
| Images or resources appear stale              | Run `make clean`, then rebuild.                                                       |

## 8. Further reading

For the resource formats, room definitions, interactions, timelines, and
C extensions, see [DEVELOPING.en.md](./DEVELOPING.en.md).

For the default game controls and conventions, see
[HOWTOPLAY.en.md](./HOWTOPLAY.en.md).
