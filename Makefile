V=1
SOURCE_DIR=src
BUILD_DIR=build
include $(N64_INST)/include/n64.mk

all: spellcraft.z64
.PHONY: all

###
# images
###

PNG_RGBA16 := $(shell find assets/ -type f -name '*.RGBA16.png' | sort)

MKSPRITE_FLAGS ?=

SPRITES := $(PNG_RGBA16:assets/%.RGBA16.png=filesystem/%.sprite)

filesystem/%.sprite: assets/%.RGBA16.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) -f RGBA16 --compress -o "$(dir $@)" "$<"

###
# meshes
###

BLENDER_4 := /home/james/Blender/blender-4.0.2-linux-x64/blender

MESH_SOURCES := $(shell find assets/ -type f -name '*.blend' | sort)

MESHES := $(MESH_SOURCES:assets/%.blend=filesystem/%.mesh)

filesystem/%.mesh: assets/%.blend tools/mesh_export/__init__.py
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(@:filesystem/%.mesh=build/assets/%.mesh))
	$(BLENDER_4) $< --background --python tools/mesh_export/__init__.py -- $(@:filesystem/%.mesh=build/assets/%.mesh)
	mkasset -o $(dir $@) -w 256 $(@:filesystem/%.mesh=build/assets/%.mesh)

###
# materials
###

MATERIAL_SOURCES := $(shell find assets/ -type f -name '*.mat.json' | sort)

MATERIAL_SCRIPTS := $(shell find tools/mesh_export/material_writer -type f -name '*.py')

MATERIALS := $(MATERIAL_SOURCES:assets/%.mat.json=filesystem/%.mat)

filesystem/%.mat: assets/%.mat.json $(MATERIAL_SCRIPTS)
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(@:filesystem/%.mat=build/assets/%.mat))
	python tools/mesh_export/material_writer --default assets/materials/default.mat.json $< $(@:filesystem/%.mat=build/assets/%.mat)
	mkasset -o $(dir $@) -w 4 $(@:filesystem/%.mat=build/assets/%.mat)

###
# source code
###

SOURCES := $(shell find src/ -type f -name '*.c' | sort)
SOURCE_OBJS := $(SOURCES:src/%.c=$(BUILD_DIR)/%.o)
OBJS := $(BUILD_DIR)/main.o $(SOURCE_OBJS)

$(BUILD_DIR)/spellcraft.dfs: $(SPRITES) $(MESHES) $(MATERIALS)
$(BUILD_DIR)/spellcraft.elf: $(OBJS)

spellcraft.z64: N64_ROM_TITLE="SpellCraft"
spellcraft.z64: $(BUILD_DIR)/spellcraft.dfs

clean:
	rm -rf $(BUILD_DIR)/* filesystem/ *.z64
.PHONY: clean

clean:
	rm -rf filesystem/ 
.PHONY: clean-resource

-include $(wildcard $(BUILD_DIR)/*.d)