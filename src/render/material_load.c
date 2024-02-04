#include "material_load.h"

#include <libdragon.h>

// MATR
#define EXPECTED_HEADER 0x4D415452

#define COMMAND_EOF         0x00
#define COMMAND_COMBINE     0x01
#define COMMAND_ENV         0x02
#define COMMAND_LIGHTING    0x03

void material_load(struct Material* into, const char* path) {
    FILE* materialFile = asset_fopen(path, NULL);

    int header;
    fread(&header, 1, 4, materialFile);
    assert(header == EXPECTED_HEADER);

    bool hasMore = true;

    materialInit(into);

    glNewList(into->list, GL_COMPILE);

    while (hasMore) {
        uint8_t nextCommand;
        fread(&nextCommand, 1, 1, materialFile);

        switch (nextCommand) {
            case COMMAND_EOF:
                hasMore = false;
                break;
            case COMMAND_COMBINE:
                {
                    rdpq_combiner_t combineMode;
                    fread(&combineMode, sizeof(rdpq_combiner_t), 1, materialFile);
                    rdpq_mode_combiner(combineMode);
                }
                break;
            case COMMAND_ENV:
                {
                    color_t color;
                    fread(&color, sizeof(color_t), 1, materialFile);
                    rdpq_set_env_color(color);
                }
                break;
            case COMMAND_LIGHTING:
                {
                    uint8_t enabled;
                    fread(&enabled, 1, 1, materialFile);
                    if (enabled) {
                        glEnable(GL_LIGHTING);
                    } else {
                        glDisable(GL_LIGHTING);
                    }
                }
                break;
        }
    }

    glEndList();

    fclose(materialFile);
}