V=1
SOURCE_DIR=src
BUILD_DIR=build
include $(N64_INST)/include/n64.mk
include $(T3D_INST)/t3d.mk

MK_ASSET=$(N64_INST)/bin/mkasset

N64_C_AND_CXX_FLAGS += -Og

all: spellcraft.z64
.PHONY: all

tests: spellcraft_test.z64
.PHONY: tests

###
# fonts
###

FONT_SOURCES := $(shell find assets/ -type f -name '*.ttf' | sort)

FONTS := $(FONT_SOURCES:assets/%.ttf=filesystem/%.font64)

MKFONT_FLAGS ?=

filesystem/%.font64: assets/%.ttf
	@mkdir -p $(dir $@)
	@echo "    [FONT] $@"
	@$(N64_MKFONT) $(MKFONT_FLAGS) -o $(dir $@) "$<"

###
# images
###

PNG_RGBA16 := $(shell find assets/ -type f -name '*.png' | sort)

MKSPRITE_FLAGS ?=

SPRITES := $(PNG_RGBA16:assets/%.png=filesystem/%.sprite)

filesystem/%.sprite: assets/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	$(N64_MKSPRITE) -f ${shell jq -r .format ${<:%.png=%.json} || echo AUTO} --compress -o "$(dir $@)" "$<"

###
# t3d_meshes
###

EXPORT_SOURCE := $(shell find tools/mesh_export/ -type f -name '*.py' | sort)
# BLENDER_4 := /home/james/Blender/blender-4.0.2-linux-x64/blender

MESH_SOURCES := $(shell find assets/meshes -type f -name '*.blend' | sort)

TMESHES := $(MESH_SOURCES:assets/meshes/%.blend=filesystem/meshes/%.tmesh)

filesystem/meshes/%.tmesh filesystem/meshes/%.anim: assets/meshes/%.blend $(EXPORT_SOURCE)
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(@:filesystem/meshes/%.tmesh=build/assets/meshes/%.tmesh))
	$(BLENDER_4) $< --background --python-exit-code 1 --python tools/mesh_export/t3d_mesh.py -- $(@:filesystem/meshes/%.tmesh=build/assets/meshes/%.tmesh)
	$(MK_ASSET) -o $(dir $@) -w 256 $(@:filesystem/meshes/%.tmesh=build/assets/meshes/%.tmesh)
	-cp $(@:filesystem/meshes/%.tmesh=build/assets/meshes/%.anim) $(@:%.tmesh=%.anim)

###
# materials
###

MATERIAL_SOURCES := $(shell find assets/ -type f -name '*.mat.json' | sort)

MATERIALS := $(MATERIAL_SOURCES:assets/%.mat.json=filesystem/%.mat)

filesystem/%.mat: assets/%.mat.json $(EXPORT_SOURCE)
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(@:filesystem/%.mat=build/assets/%.mat))
	python3 tools/mesh_export/material.py --default assets/materials/default.mat.json $< $(@:filesystem/%.mat=build/assets/%.mat)
	$(MK_ASSET) -o $(dir $@) -w 4 $(@:filesystem/%.mat=build/assets/%.mat)

###
# material_builder
###

assets/materials/materials.blend: tools/mesh_export/material_generator.py $(MATERIAL_SOURCES)
	$(BLENDER_4) --background --python-exit-code 1 --python tools/mesh_export/material_generator.py -- $@ $(MATERIAL_SOURCES)

###
# cutscenes
###

SCRIPTS := $(shell find assets/scripts -type f -name '*.script' | sort)

SCRIPTS_COMPILED := $(SCRIPTS:assets/scripts/%=filesystem/scripts/%)

build/assets/scripts/globals.json build/assets/scripts/globals.dat: tools/mesh_export/globals.py assets/scripts/globals.script
	@mkdir -p $(dir $@)
	python3 tools/mesh_export/globals.py build/assets/scripts/globals assets/scripts/globals.script

filesystem/scripts/globals.dat: build/assets/scripts/globals.dat
	$(MK_ASSET) -o $(dir $@) -w 4 $<

filesystem/%.script: assets/%.script build/assets/scripts/globals.json $(EXPORT_SOURCE)
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(@:filesystem/%=build/assets/%))
	python3 tools/mesh_export/cutscene.py -g build/assets/scripts/globals.json $< $(@:filesystem/%=build/assets/%)
	$(MK_ASSET) -o $(dir $@) -w 4 $(@:filesystem/%=build/assets/%)

###
# worlds
###

WORLD_SOURCES := $(shell find assets/worlds -type f -name '*.blend' | sort)

WORLDS := $(WORLD_SOURCES:assets/worlds/%.blend=filesystem/worlds/%.world)

filesystem/worlds/%.world: assets/worlds/%.blend build/assets/scripts/globals.json $(EXPORT_SOURCE)
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(@:filesystem/worlds/%.world=build/assets/worlds/%.world))
	$(BLENDER_4) $< --background --python-exit-code 1 --python tools/mesh_export/world.py -- $(@:filesystem/worlds/%.world=build/assets/worlds/%.world)
	$(MK_ASSET) -o $(dir $@) -w 256 $(@:filesystem/worlds/%.world=build/assets/worlds/%.world)

###
# tests
###

build/test_result.txt: $(EXPORT_SOURCE)
	@mkdir -p $(dir $@)
	python3 tools/mesh_export/cutscene_test.py
	echo "success" > $@

###
# source code
###

SOURCES := $(shell find src/ ! -name '*_test.c' ! -name 'main.c' -type f -name '*.c' | sort)
SOURCE_OBJS := $(SOURCES:src/%.c=$(BUILD_DIR)/%.o)
OBJS := $(BUILD_DIR)/main.o $(SOURCE_OBJS)

TEST_SOURCES := $(shell find src/ -type f -name '*_test.c' | sort)
TEST_SOURCE_OBJS := $(TEST_SOURCES:src/%.c=$(BUILD_DIR)/%.o)
TEST_OBJS := $(SOURCE_OBJS) $(TEST_SOURCE_OBJS)

filesystem/: $(SPRITES) $(TMESHES) $(MATERIALS) $(WORLDS) $(FONTS) $(SCRIPTS_COMPILED) filesystem/scripts/globals.dat

$(BUILD_DIR)/spellcraft.dfs: filesystem/ $(SPRITES) $(TMESHES) $(MATERIALS) $(WORLDS) $(FONTS) $(SCRIPTS_COMPILED) filesystem/scripts/globals.dat
$(BUILD_DIR)/spellcraft.elf: $(OBJS)
$(BUILD_DIR)/spellcraft_test.elf: $(TEST_OBJS)

spellcraft.z64: N64_ROM_TITLE="SpellCraft"
spellcraft.z64: $(BUILD_DIR)/spellcraft.dfs

spellcraft_test.z64: N64_ROM_TITLE="SpellCraft Test"
spellcraft_test.z64: $(BUILD_DIR)/spellcraft.dfs

clean:
	rm -rf $(BUILD_DIR)/* filesystem/ *.z64
.PHONY: clean

clean-fs:
	rm -rf filesystem/ $(BUILD_DIR)/spellcraft.dfs
.PHONY: clean-resource

check-pairings:
	node tools/pairing_checker.js $(SOURCES) src/main.c
.PHONY: check-pairings

-include $(wildcard $(BUILD_DIR)/*.d)