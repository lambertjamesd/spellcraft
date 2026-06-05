V=1
SOURCE_DIR=src
BUILD_DIR=build
N64_CFLAGS += -O3 -DNDEBUG
include $(N64_INST)/include/n64.mk
N64_ROM_TITLE = "Spellcraft"
N64_ROM_REGION = E
N64_ROM_SAVETYPE = sram256k
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

filesystem/fonts/HeavyEquipment.font64: MKFONT_FLAGS=--charset assets/fonts/alpha-cap.txt --monochrome -s 36

###
# images
###

PNG_RGBA16 := $(shell find assets/ -type f -name '*.png' | sort)

MKSPRITE_FLAGS ?=

SPRITES := $(PNG_RGBA16:assets/%.png=filesystem/%.sprite)

.SECONDEXPANSION:
filesystem/%.sprite: assets/%.png $$(wildcard assets/%.json)
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	$(N64_MKSPRITE) -f ${shell jq -r .format ${<:%.png=%.json} || echo AUTO} --compress -o "$(dir $@)" "$<"


###
# script dependencies
###
	
EXPORT_SOURCE := $(shell find tools/mesh_export/ -type f -name '*.py' | sort)

###
# t3d_meshes
###

# BLENDER_5 := /home/james/Blender/blender-4.5.8-linux-x64/blender

MESH_SOURCES := $(shell find assets/meshes -type f -name '*.blend' | sort)

TMESHES := $(MESH_SOURCES:assets/meshes/%.blend=filesystem/meshes/%.tmesh)

filesystem/meshes/%.tmesh filesystem/meshes/%.anim: assets/meshes/%.blend tools/mesh_export/t3d_mesh.d.template
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(@:filesystem/meshes/%.tmesh=build/assets/meshes/%.tmesh))
	$(BLENDER_5) $< --background --factory-startup --python-exit-code 1 --python tools/mesh_export/t3d_mesh.py -- $(@:filesystem/meshes/%.tmesh=build/assets/meshes/%.tmesh)
	$(MK_ASSET) -o $(dir $@) -w 256 $(@:filesystem/meshes/%.tmesh=build/assets/meshes/%.tmesh)
	-cp $(@:filesystem/meshes/%.tmesh=build/assets/meshes/%.anim) $(@:%.tmesh=%.anim)

assets/game_objects.json: tools/mesh_export/rebuild_prefabs.py $(EXPORT_SOURCE)
	$(BLENDER_5) --background --factory-startup --python-exit-code 1 --python tools/mesh_export/rebuild_prefabs.py -- assets/game_objects.json

###
# materials
###

MATERIAL_SOURCES := $(shell find assets/ -type f -name '*.mat.json' | sort)
MATERIAL_BLENDER_SOURCES := $(shell find assets/ -type f -name '*.mat.blend' | sort)

MATERIALS := $(MATERIAL_SOURCES:assets/%.mat.json=filesystem/%.mat) $(MATERIAL_BLENDER_SOURCES:assets/%.mat.blend=filesystem/%.mat)

filesystem/%.mat: assets/%.mat.json tools/mesh_export/material.d.template
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(@:filesystem/%.mat=build/assets/%.mat))
	python3 tools/mesh_export/material.py --default assets/materials/default.mat.json $< $(@:filesystem/%.mat=build/assets/%.mat)
	$(MK_ASSET) -o $(dir $@) -w 4 $(@:filesystem/%.mat=build/assets/%.mat)

filesystem/%.mat: assets/%.mat.blend tools/mesh_export/material_fast64.d.template
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(@:filesystem/%.mat=build/assets/%.mat))
	$(BLENDER_5) $< --background --factory-startup --python-exit-code 1 --python tools/mesh_export/material_fast64.py -- $@

list_materials: $(MATERIALS)
	echo $(MATERIALS)

###
# sound effects
###

SOUND_EFFECT_SOURCES := $(shell find assets/ -type f -name '*.wav' | sort)

SOUND_EFFECTS := $(SOUND_EFFECT_SOURCES:assets/%.wav=filesystem/%.wav64)

filesystem/%.wav64: assets/%.wav $$(wildcard assets/%.txt)
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	$(N64_AUDIOCONV) ${shell cat ${<:%.wav=%.txt} || echo --wav-compress 1 --wav-resample 22050} -o $@ $<
	
###
# music
###

MUSIC_SOURCES := $(shell find assets/ -type f -name '*.mp3' | sort)

MUSIC := $(MUSIC_SOURCES:assets/%.mp3=filesystem/%.wav64)

filesystem/%.wav64: assets/%.mp3 $$(wildcard assets/%.txt)
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	$(N64_AUDIOCONV) ${shell cat ${<:%.mp3=%.txt} || echo --wav-compress 1 --wav-resample 22050} -o $@ $<

###
# material_builder
###

assets/materials/materials.blend: tools/mesh_export/material_generator.py $(MATERIAL_SOURCES)
	$(BLENDER_5) --background --factory-startup --python-exit-code 1 --python tools/mesh_export/material_generator.py -- $@ $(MATERIAL_SOURCES)

###
# cutscenes
###

SCRIPTS := $(shell find assets -type f -name '*.script' | sort)

SCRIPTS_COMPILED := $(SCRIPTS:assets/%=filesystem/%)

build/cutscene/function_defs.json: src/cutscene/cutscene_step_fn.c src/cutscene/expression_fn.c
	@mkdir -p build/cutscene/
	python3 tools/mesh_export/generate_function_defs.py

build/assets/scripts/globals.json build/assets/scripts/globals.dat src/player/inventory_mapping.c: tools/mesh_export/globals.py assets/scripts/globals.script
	@mkdir -p build/assets/scripts/
	python3 tools/mesh_export/globals.py build/assets/scripts/globals src/player/inventory_mapping.c assets/scripts/globals.script

filesystem/scripts/globals.dat: build/assets/scripts/globals.dat
	$(MK_ASSET) -o $(dir $@) -w 4 $<

filesystem/scripts/%.script: assets/scripts/%.script build/assets/scripts/globals.json build/cutscene/function_defs.json tools/mesh_export/cutscene.d.template
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(@:filesystem/%=build/assets/%))
	python3 tools/mesh_export/cutscene.py -g build/assets/scripts/globals.json $< $(@:filesystem/%=build/assets/%)
	$(MK_ASSET) -o $(dir $@) -w 4 $(@:filesystem/%=build/assets/%)
	
filesystem/scenes/%.script: assets/scenes/%.script build/assets/scenes/%.json assets/scenes/%.blend build/assets/scripts/globals.json tools/mesh_export/cutscene.d.template
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(@:filesystem/%=build/assets/%))
	python3 tools/mesh_export/cutscene.py -g build/assets/scripts/globals.json -c $(@:filesystem/%.script=build/assets/%.json) $< $(@:filesystem/%=build/assets/%)
	$(MK_ASSET) -o $(dir $@) -w 4 $(@:filesystem/%=build/assets/%)

###
# scenes
###

SCENE_SOURCES := $(shell find assets/scenes -type f -name '*.blend' | sort)

SCENES := $(SCENE_SOURCES:assets/scenes/%.blend=filesystem/scenes/%.scene)

.SECONDEXPANSION:
filesystem/scenes/%.scene: assets/scenes/%.blend $$(wildcard assets/scenes/%.script) build/assets/scripts/globals.json build/cutscene/function_defs.json tools/mesh_export/scene.d.template
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(@:filesystem/scenes/%.scene=build/assets/scenes/%.scene))
	echo $@ $<
	$(BLENDER_5) $< --background --factory-startup --python-exit-code 1 --python tools/mesh_export/scene.py -- $(@:filesystem/scenes/%.scene=build/assets/scenes/%.scene) $(@:%.scene=%.overworld)
	$(MK_ASSET) -o $(dir $@) -w 256 $(@:filesystem/scenes/%.scene=build/assets/scenes/%.scene)
	-cp $(@:filesystem/scenes/%.scene=build/assets/scenes/%.sanim) $(@:%.scene=%.sanim)

filesystem/scenes/%.overworld build/assets/scenes/%.json: filesystem/scenes/%.scene;

build/assets/scenes/%_exits.txt: assets/scenes/%.blend tools/mesh_export/find_exits.py
	@mkdir -p $(dir $@)
	$(BLENDER_5) $< --background --factory-startup --python-exit-code 1 --python tools/mesh_export/find_exits.py -- $@

all_exits: $(SCENE_SOURCES:assets/scenes/%.blend=build/assets/scenes/%_exits.txt)
.PHONY: all_exits


###
# tests
###

build/test_result.txt: $(EXPORT_SOURCE)
	@mkdir -p $(dir $@)
	python3 tools/mesh_export/cutscene_test.py
	echo "success" > $@

build/blender_results.txt: $(EXPORT_SOURCE)
	@mkdir -p $(dir $@)
	$(BLENDER_5) --background --factory-startup --python-exit-code 1 --python tools/mesh_export/tests.py
	echo "success" > $@

###
# source code
###

SOURCES := $(shell find src/ ! -name '*_test.c' ! -name 'main.c' ! -name 'inventory_mapping.c' -type f -name '*.c' | sort) src/player/inventory_mapping.c
SOURCE_OBJS := $(SOURCES:src/%.c=$(BUILD_DIR)/%.o)
UCODES := $(shell find src/ -type f -name '*.S' | sort)
UCODE_OBJS := $(UCODES:src/%.S=$(BUILD_DIR)/%.o)
OBJS := $(BUILD_DIR)/main.o $(SOURCE_OBJS) $(UCODE_OBJS)

TEST_SOURCES := $(shell find src/ -type f -name '*_test.c' | sort)
TEST_SOURCE_OBJS := $(TEST_SOURCES:src/%.c=$(BUILD_DIR)/%.o)
TEST_OBJS := $(SOURCE_OBJS) $(TEST_SOURCE_OBJS)

filesystem/: $(SPRITES) $(TMESHES) $(MATERIALS) $(SCENES) $(REPAIRS) $(FONTS) $(SCRIPTS_COMPILED) $(SOUND_EFFECTS) $(MUSIC) filesystem/scripts/globals.dat

$(BUILD_DIR)/spellcraft.dfs: filesystem/ $(SPRITES) $(TMESHES) $(MATERIALS) $(SCENES) $(REPAIRS) $(FONTS) $(SCRIPTS_COMPILED) $(SOUND_EFFECTS) $(MUSIC) filesystem/scripts/globals.dat
$(BUILD_DIR)/spellcraft.elf: $(OBJS)
$(BUILD_DIR)/spellcraft_test.elf: $(TEST_OBJS)

build/%.asm: build/%.o
	mips-linux-gnu-objdump -S --disassemble $< > $@

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

clean-src:
	find build -name "*.o" -type f -delete
	find build -name "*.d" -type f -delete

check-pairings:
	node tools/pairing_checker.js $(SOURCES) src/main.c
.PHONY: check-pairings

tools/mesh_export.zip: tools/mesh_export/__init__.py
	cd tools; zip -r mesh_export.zip mesh_export/

build/strings.txt: tools/mesh_export/export_text.py $(SCRIPTS)
	$(BLENDER_5) --background --factory-startup --python-exit-code 1 --python tools/mesh_export/export_text.py -- $@ $(SCRIPTS)

DEPENDENCY_FILES := $(shell find build/ -name '*.d' 2>/dev/null)

-include $(DEPENDENCY_FILES)
