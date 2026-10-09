#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
DEBUG ?= 0
include $(DEVKITARM)/3ds_rules
-include moteur.mk

#---------------------------------------------------------------------------------
# Project configuration
#---------------------------------------------------------------------------------
BUILD				:=	build
SOURCES 			:=  source extensions
DATA				:=	data
INCLUDES			:=	include source extensions

ASSETS				:= resources
GRAPHICS			:= $(ASSETS)/gfx
TIMELINES			:= $(ASSETS)/timelines
INVENTORY			:= $(ASSETS)/inventory
HUD					:= $(ASSETS)/hud
MINIGAMES			:= $(ASSETS)/minigames
ROOMS				:= $(ASSETS)/rooms
ROMFS				:= romfs
CIA					:= $(ASSETS)/cia

# HOME Menu / CIA metadata
ICON				:= $(CIA)/icon.png

VERSION_PARTS 		:= $(subst ., ,$(APP_VERSION))
APP_VERSION_MAJOR 	:= $(word 1,$(VERSION_PARTS))
APP_VERSION_MINOR 	:= $(word 2,$(VERSION_PARTS))
APP_VERSION_MICRO 	:= $(word 3,$(VERSION_PARTS))

CIA_RSF				:= $(CIA)/app.rsf
CIA_BANNER 			:= $(CIA)/banner.bnr
CIA_BANNER_PNG 		:= $(CIA)/banner.png
CIA_BANNER_WAV 		:= $(CIA)/banner.wav

# These directories are copied as-is from resources/ to romfs/.
RAW_ASSET_DIRS		:=	audio lang states

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH				:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS 				:=	-g -Wall -O2 -mword-relocations -ffunction-sections $(ARCH)

ifeq ($(DEBUG),1)
	CFLAGS 			+= -DDEBUG
endif
CFLAGS 				+=	$(INCLUDE) -D__3DS__ -DVERSION=\"$(APP_VERSION)\"

CXXFLAGS			:= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS				:=	-g $(ARCH)
LDFLAGS				=	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS				:= -lcitro2d -lcitro3d -lctru -lm -lvorbisidec -logg

#---------------------------------------------------------------------------------
# Libraries
#---------------------------------------------------------------------------------
LIBDIRS 			:= $(CTRULIB) $(DEVKITPRO)/portlibs/3ds

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add
# additional rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT		:=	$(CURDIR)/$(TARGET)
export TOPDIR		:=	$(CURDIR)
export DEPSDIR		:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# Source files
#---------------------------------------------------------------------------------
CFILES				:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES			:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES				:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PICAFILES			:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.v.pica)))
SHLISTFILES			:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.shlist)))
BINFILES			:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# Graphics
#---------------------------------------------------------------------------------

# generate-gfx creates one gfx.t3s in every directory containing PNG files.
# This list is evaluated by the second make invocation, after generation.
GFXFILES			:=	$(shell if [ -d "$(ASSETS)" ]; then find "$(ASSETS)" -type f -name 'gfx.t3s' ! -path "$(CIA)/*"; fi)

# Everything except MEMGFX goes to RomFS, preserving the directory hierarchy.
# resources/gfx/hall/gfx.t3s -> romfs/gfx/hall/gfx.t3x + gfx.h
ROMGFX				:=	$(GFXFILES)

ROM_T3XFILES		:=	$(patsubst $(ASSETS)/%/gfx.t3s,$(ROMFS)/%/gfx.t3x,$(ROMGFX))
ROM_HFILES			:=	$(patsubst $(ASSETS)/%/gfx.t3s,$(ROMFS)/%/gfx.h,$(ROMGFX))

GAME_FILES			:=  $(patsubst $(ASSETS)/%,$(ROMFS)/%,$(wildcard $(ASSETS)/game/*))

#---------------------------------------------------------------------------------
# Timelines
#---------------------------------------------------------------------------------

TIMELINE_SRC_FILES 	:= $(wildcard $(TIMELINES)/*/timeline)
TIMELINE_FILES 		:= $(patsubst $(TIMELINES)/%/timeline,$(ROMFS)/timelines/%/timeline,$(TIMELINE_SRC_FILES))
TIMELINE_AUDIO_FILES := $(patsubst $(ASSETS)/%,$(ROMFS)/%,$(wildcard $(TIMELINES)/*/*.raw $(TIMELINES)/*/*.ogg))

#---------------------------------------------------------------------------------
# Inventory
#---------------------------------------------------------------------------------

INVENTORY_FILES 	:= $(ROMFS)/inventory/inventory

#---------------------------------------------------------------------------------
# HUD
#---------------------------------------------------------------------------------

HUD_FILES := $(ROMFS)/hud/hud

#---------------------------------------------------------------------------------
# Minigames
#---------------------------------------------------------------------------------

MINIGAME_RAW_FILES 			:= $(shell if [ -d "$(MINIGAMES)" ]; then find "$(MINIGAMES)" -type f -name '*.raw'; fi)
ROMFS_MINIGAME_RAW_FILES 	:= $(patsubst $(ASSETS)/%,$(ROMFS)/%,$(MINIGAME_RAW_FILES))

#---------------------------------------------------------------------------------
# Rooms
#---------------------------------------------------------------------------------

ROOM_SRC_FILES				:= $(wildcard $(ROOMS)/*/room)
ROMFS_ROOM_FILES			:= $(patsubst $(ASSETS)/%,$(ROMFS)/%,$(ROOM_SRC_FILES))

ROOM_RAW_FILES 				:= $(shell if [ -d "$(ROOMS)" ]; then find "$(ROOMS)" -type f -name '*.raw'; fi)
ROMFS_ROOM_RAW_FILES 		:= $(patsubst $(ASSETS)/%,$(ROMFS)/%,$(ROOM_RAW_FILES))

#---------------------------------------------------------------------------------
# Assets copied directly to RomFS
#---------------------------------------------------------------------------------

RAW_ASSET_FILES 			:= $(foreach dir,$(RAW_ASSET_DIRS),$(shell if [ -d "$(ASSETS)/$(dir)" ]; then find "$(ASSETS)/$(dir)" -type f; fi))
ROMFS_RAW_FILES 			:= $(patsubst $(ASSETS)/%,$(ROMFS)/%,$(RAW_ASSET_FILES))

#---------------------------------------------------------------------------------
# Search paths
#---------------------------------------------------------------------------------
export VPATH 				:= $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) $(foreach dir,$(DATA),$(CURDIR)/$(dir))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

#---------------------------------------------------------------------------------
# Object files
#---------------------------------------------------------------------------------
export OFILES_SOURCES 		:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

# IMPORTANT:
# Only MEM_T3XFILES is converted with bin2o and linked into the executable.
# RomFS graphics are NOT included here.
export OFILES_BIN			:=	$(addsuffix .o,$(BINFILES)) \
								$(PICAFILES:.v.pica=.shbin.o) \
								$(SHLISTFILES:.shlist=.shbin.o) \
								$(addsuffix .o,$(MEM_T3XFILES))

export OFILES 				:= $(OFILES_BIN) $(OFILES_SOURCES)

#---------------------------------------------------------------------------------
# Generated headers
#---------------------------------------------------------------------------------
export HFILES 				:= 	$(PICAFILES:.v.pica=_shbin.h) \
								$(SHLISTFILES:.shlist=_shbin.h) \
								$(addsuffix .h,$(subst .,_,$(BINFILES))) \
								$(MEM_HFILES)

#---------------------------------------------------------------------------------
# Include / library paths
#---------------------------------------------------------------------------------
export INCLUDE 				:= 	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
				  				$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
				  				-I$(CURDIR)/$(BUILD)

export LIBPATHS				:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export _3DSXDEPS			:=	$(if $(NO_SMDH),,$(OUTPUT).smdh)

#---------------------------------------------------------------------------------
# Icon
#---------------------------------------------------------------------------------
ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.png)
	ifneq (,$(findstring $(TARGET).png,$(icons)))
		export APP_ICON 	:= $(TOPDIR)/$(TARGET).png
	else
		ifneq (,$(findstring icon.png,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.png
		endif
	endif
else
	export APP_ICON 		:= $(TOPDIR)/$(ICON)
endif

#---------------------------------------------------------------------------------
# SMDH
#---------------------------------------------------------------------------------
ifeq ($(strip $(NO_SMDH)),)
	export _3DSXFLAGS		+= --smdh=$(CURDIR)/$(TARGET).smdh
endif

#---------------------------------------------------------------------------------
# RomFS
#---------------------------------------------------------------------------------
ifneq ($(ROMFS),)
	export _3DSXFLAGS		+= --romfs=$(CURDIR)/$(ROMFS)
endif

.PHONY: all generate-gfx build-project banner cia clean lint

#---------------------------------------------------------------------------------
# Main outer target
#---------------------------------------------------------------------------------
# gfx.t3s files must exist before the graphics lists are evaluated, hence the
# second make invocation.
all: generate-gfx
	@$(MAKE) --no-print-directory build-project

#---------------------------------------------------------------------------------
# Generate one gfx.t3s per directory containing PNG files
#---------------------------------------------------------------------------------
generate-gfx:
	@find "$(ASSETS)" -type f -name '*.png' ! -path "$(CIA)/*" -exec dirname {} \; | sort -u | \
	while IFS= read -r d; do \
		tmp="$$d/gfx.t3s.tmp"; \
		{ \
			echo "--atlas -f rgba8888 -z auto"; \
			for f in "$$d"/*.png; do basename "$$f"; done | sort; \
		} > "$$tmp"; \
		if [ ! -f "$$d/gfx.t3s" ] || ! cmp -s "$$tmp" "$$d/gfx.t3s"; then \
			mv "$$tmp" "$$d/gfx.t3s"; \
			echo "generated $$d/gfx.t3s"; \
		else \
			rm -f "$$tmp"; \
		fi; \
	done

#---------------------------------------------------------------------------------
# Build assets, then run the usual inner Makefile from build/
#---------------------------------------------------------------------------------
build-project: $(BUILD) \
	 $(ROM_T3XFILES) \
	 $(GAME_FILES) \
	 $(ROM_HFILES) \
	 $(TIMELINE_FILES) \
	 $(TIMELINE_AUDIO_FILES) \
	 $(INVENTORY_FILES) \
	 $(HUD_FILES) \
	 $(ROMFS_MINIGAME_RAW_FILES) \
	 $(ROMFS_ROOM_FILES) \
	 $(ROMFS_ROOM_RAW_FILES) \
	 $(ROMFS_RAW_FILES)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
# CIA
#---------------------------------------------------------------------------------
# Generate the banner locally once, then commit resources/cia/banner.bnr.
# The CIA target deliberately does NOT depend on this target, so CI only needs
# makerom, not bannertool.
banner:
	@bannertool makebanner -i $(CIA_BANNER_PNG) -a $(CIA_BANNER_WAV) -o $(CIA_BANNER)
	@echo built ... $(CIA_BANNER)

# Build the normal project first (ELF + SMDH + RomFS), then package it as CIA.
cia:
	@test -f "$(CIA_BANNER)" || { \
		echo "Missing $(CIA_BANNER). Run 'make banner' locally and commit it."; \
		exit 1; \
	}
	@command -v makerom >/dev/null 2>&1 || { \
		echo "makerom not found in PATH"; \
		exit 1; \
	}
	@makerom -f cia \
		-o $(TARGET).cia \
		-target t \
		-exefslogo \
		-elf $(TARGET).elf \
		-icon $(TARGET).smdh \
		-banner $(CIA_BANNER) \
		-rsf $(CIA_RSF) \
		-major $(APP_VERSION_MAJOR) \
		-minor $(APP_VERSION_MINOR) \
		-micro $(APP_VERSION_MICRO) \
		-DAPP_TITLE="$(APP_TITLE)" \
		-DAPP_PRODUCT_CODE="$(APP_PRODUCT_CODE)" \
		-DAPP_UNIQUE_ID="$(APP_UNIQUE_ID)" \
		-DAPP_ROMFS="$(CURDIR)/$(ROMFS)" \
		-DAPP_CATEGORY="Application" \
		-DAPP_USE_ON_SD="true" \
		-DAPP_ENCRYPTED="false" \
		-DAPP_MEMORY_TYPE="Application" \
		-DAPP_SYSTEM_MODE="64MB" \
		-DAPP_SYSTEM_MODE_EXT="Legacy" \
		-DAPP_CPU_SPEED="268MHz" \
		-DAPP_ENABLE_L2_CACHE="false" \
		-DAPP_VERSION_MAJOR="$(APP_VERSION_MAJOR)"
	@echo built ... $(TARGET).cia

#---------------------------------------------------------------------------------
# Directories
#---------------------------------------------------------------------------------
$(BUILD):
	@mkdir -p $@

#---------------------------------------------------------------------------------
# Clean
#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -rf $(BUILD) $(ROMFS)
	@rm -f $(TARGET).3dsx $(OUTPUT).smdh $(TARGET).elf $(TARGET).cia PRN
	@find "$(ASSETS)" -type f \( -name 'gfx.t3s' -o -name 'gfx.t3s.tmp' \) -delete 2>/dev/null || true

#---------------------------------------------------------------------------------
# Lint
#---------------------------------------------------------------------------------

lint:
	@$(MAKE) CFLAGS="$(CFLAGS) -fanalyzer"


#---------------------------------------------------------------------------------
# Run in an emulator
#---------------------------------------------------------------------------------

run: $(TARGET).3dsx
	@flatpak info org.azahar_emu.Azahar &>/dev/null || { \
		echo "Azahar flatpak install is missing"; \
		exit 1; \
	}
	@flatpak run --filesystem="$(CURDIR):ro" org.azahar_emu.Azahar "$(TARGET).3dsx"

#---------------------------------------------------------------------------------
# Assets copied directly to RomFS
#---------------------------------------------------------------------------------
$(ROMFS)/%: $(ASSETS)/%
	@mkdir -p $(dir $@)
	@cp $< $@

#---------------------------------------------------------------------------------
# Runtime graphics
# resources/<path>/gfx.t3s -> romfs/<path>/gfx.t3x + gfx.h
#---------------------------------------------------------------------------------
$(ROMFS)/%/gfx.t3x $(ROMFS)/%/gfx.h &: $(ASSETS)/%/gfx.t3s
	@echo $*/gfx.t3s
	@mkdir -p $(ROMFS)/$*
	@tex3ds -i $< \
		-H $(ROMFS)/$*/gfx.h \
		-d $(DEPSDIR)/gfx_$(subst /,_,$*).d \
		-o $(ROMFS)/$*/gfx.t3x

#---------------------------------------------------------------------------------
# Timelines
#---------------------------------------------------------------------------------
$(ROMFS)/timelines/%/timeline: $(TIMELINES)/%/timeline
	@mkdir -p $(ROMFS)/timelines/$*
	@cp $< $@

#---------------------------------------------------------------------------------
# Inventory
#---------------------------------------------------------------------------------
$(ROMFS)/inventory/inventory: $(INVENTORY)/inventory
	@mkdir -p $(ROMFS)/inventory
	@cp $< $@

#---------------------------------------------------------------------------------
# HUD
#---------------------------------------------------------------------------------
$(ROMFS)/hud/hud: $(HUD)/hud
	@mkdir -p $(ROMFS)/hud
	@cp $< $@

#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
# Inner make
#---------------------------------------------------------------------------------

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).3dsx	:	$(OUTPUT).elf $(_3DSXDEPS)

$(OFILES_SOURCES) : $(HFILES)

$(OUTPUT).elf	:	$(OFILES)

#---------------------------------------------------------------------------------
# Binary data
#---------------------------------------------------------------------------------
%.bin.o %_bin.h : %.bin
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------
# Only .t3x files present in BUILD are converted with bin2o.
#---------------------------------------------------------------------------------
.PRECIOUS	:	%.t3x %.shbin

%.t3x.o %_t3x.h : %.t3x
	$(SILENTMSG) $(notdir $<)
	$(bin2o)

#---------------------------------------------------------------------------------
# Shaders
#---------------------------------------------------------------------------------
%.shbin.o %_shbin.h : %.shbin
	$(SILENTMSG) $(notdir $<)
	$(bin2o)

-include $(DEPSDIR)/*.d

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
