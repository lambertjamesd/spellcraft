V=1
SOURCE_DIR=src
BUILD_DIR=build
include $(N64_INST)/include/n64.mk

all: spellcraft.z64
.PHONY: all

###
# images
###

PNG_RGBA16 := $(shell find assets/ -type f -name '*.png' | sort)

MKSPRITE_FLAGS ?=

SPRITES := $(PNG_RGBA16:assets/%.png=filesystem/%.sprite)

filesystem/%.sprite: assets/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@" ${shell jq -r .format ${<:%.png=%.json} | sed 's/null/AUTO/g'}
	@$(N64_MKSPRITE) -f ${shell jq -r .format ${<:%.png=%.json} | sed 's/null/AUTO/g'} --compress -o "$(dir $@)" "$<"

###
# meshes
###

EXPORT_SOURCE := $(shell find tools/mesh_export/ -type f -name '*.py' | sort)
BLENDER_4 := /home/james/Blender/blender-4.0.2-linux-x64/blender

MESH_SOURCES := $(shell find assets/meshes -type f -name '*.blend' | sort)

MESHES := $(MESH_SOURCES:assets/meshes/%.blend=filesystem/meshes/%.mesh)

filesystem/meshes/%.mesh: assets/meshes/%.blend $(EXPORT_SOURCE)
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(@:filesystem/meshes/%.mesh=build/assets/meshes/%.mesh))
	$(BLENDER_4) $< --background --python-exit-code 1 --python tools/mesh_export/mesh.py -- $(@:filesystem/meshes/%.mesh=build/assets/meshes/%.mesh)
	mkasset -o $(dir $@) -w 256 $(@:filesystem/meshes/%.mesh=build/assets/meshes/%.mesh)

###
# materials
###

MATERIAL_SOURCES := $(shell find assets/ -type f -name '*.mat.json' | sort)

MATERIALS := $(MATERIAL_SOURCES:assets/%.mat.json=filesystem/%.mat)

filesystem/%.mat: assets/%.mat.json $(EXPORT_SOURCE)
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(@:filesystem/%.mat=build/assets/%.mat))
	python tools/mesh_export/material.py --default assets/materials/default.mat.json $< $(@:filesystem/%.mat=build/assets/%.mat)
	mkasset -o $(dir $@) -w 4 $(@:filesystem/%.mat=build/assets/%.mat)

###
# material_builder
###

assets/materials/materials.blend: tools/mesh_export/material_generator.py $(MATERIAL_SOURCES)
	$(BLENDER_4) --background --python-exit-code 1 --python tools/mesh_export/material_generator.py -- $@ $(MATERIAL_SOURCES)

###
# worlds
###

WORLD_SOURCES := $(shell find assets/worlds -type f -name '*.blend' | sort)

WORLDS := $(WORLD_SOURCES:assets/worlds/%.blend=filesystem/worlds/%.world)

filesystem/worlds/%.world: assets/worlds/%.blend $(EXPORT_SOURCE)
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(@:filesystem/worlds/%.world=build/assets/worlds/%.world))
	$(BLENDER_4) $< --background --python-exit-code 1 --python tools/mesh_export/world.py -- $(@:filesystem/worlds/%.world=build/assets/worlds/%.world)
	mkasset -o $(dir $@) -w 256 $(@:filesystem/worlds/%.world=build/assets/worlds/%.world)

###
# source code
###

SOURCES := $(shell find src/ -type f -name '*.c' | sort)
SOURCE_OBJS := $(SOURCES:src/%.c=$(BUILD_DIR)/%.o)
OBJS := $(BUILD_DIR)/main.o $(SOURCE_OBJS)

filesystem/: $(SPRITES) $(MESHES) $(MATERIALS) $(WORLDS)

$(BUILD_DIR)/spellcraft.dfs: filesystem/ $(SPRITES) $(MESHES) $(MATERIALS) $(WORLDS)
$(BUILD_DIR)/spellcraft.elf: $(OBJS)

spellcraft.z64: N64_ROM_TITLE="SpellCraft"
spellcraft.z64: $(BUILD_DIR)/spellcraft.dfs

clean:
	rm -rf $(BUILD_DIR)/* filesystem/ *.z64
.PHONY: clean

clean-fs:
	rm -rf filesystem/ $(BUILD_DIR)/spellcraft.dfs
.PHONY: clean-resource

-include $(wildcard $(BUILD_DIR)/*.d)