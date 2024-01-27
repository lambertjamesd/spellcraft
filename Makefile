V=1
SOURCE_DIR=src
BUILD_DIR=build
include $(N64_INST)/include/n64.mk

all: spellcraft.z64
.PHONY: all

SOURCES = $(shell find src/ -type f -name '*.c' | sort)

SOURCE_OBJS = $(SOURCES:src/%.c=$(BUILD_DIR)/%.o)

OBJS = $(BUILD_DIR)/main.o $(SOURCE_OBJS)

spellcraft.z64: N64_ROM_TITLE="SpellCraft"

$(BUILD_DIR)/spellcraft.elf: $(OBJS)

clean:
	rm -f $(BUILD_DIR)/* *.z64
.PHONY: clean

-include $(wildcard $(BUILD_DIR)/*.d)