
GCC=gcc

CFLAGS = -g -O0 -Wall

LDFLAGS = -O0

CODEFILES = $(shell find src/ -type f -name '*.c')
CODEOBJECTS = $(patsubst %.c, build/%.o, $(CODEFILES))
DEPS = $(patsubst %.c, build/%.d, $(CODEFILES))

main: asset_builder

-include $(DEPS)

build/src/%.o: src/%.c
	@mkdir -p $(@D)
	$(GCC) -c -o $@ $< $(CFLAGS) $(INCLUDE)

asset_builder: $(CODEOBJECTS)
	$(GCC) -o asset_builder $(CODEOBJECTS) $(LDFLAGS)