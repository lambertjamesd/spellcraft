#include "mesh2d.h"

#include "../resource/material_cache.h"
#include "../menu/rsp_menu.h"

#define EXPECTED_HEADER 0x4D534832

#define MESH2D_CMD_MATERIAL 0
#define MESH2D_CMD_ATTR     1
#define MESH2D_CMD_MOVE_TO  2
#define MESH2D_CMD_LINE_TO  3

void mesh2d_load(mesh2d_t* mesh, FILE* file) {
    int header;

    fread(&header, 1, 4, file);
    assert(header == EXPECTED_HEADER);

    mesh->material = material_cache_load_from_file(file);

    // load material transitions

    uint16_t transition_count;
    fread(&transition_count, sizeof(uint16_t), 1, file);
    mesh->material_transition_count = transition_count;

    if (transition_count) {
        mesh->transition_materials = malloc(sizeof(struct material) * transition_count);

        for (int i = 0; i < transition_count; i += 1) {
            material_load(&mesh->transition_materials[i], file);
#if DEBUG_MATERIALS
            material_debug(&mesh->transition_materials[i], "transition");
#endif
        }
    } else {
        mesh->transition_materials = NULL;
    }

    uint16_t command_count;
    fread(&command_count, 2, 1, file);

    rspq_block_begin();

    for (int i = 0; i < command_count; i += 1) {
        uint8_t command;
        fread(&command, 1, 1, file);

        switch (command) {
            case MESH2D_CMD_MATERIAL:
            {
                uint16_t material_index;
                fread(&material_index, sizeof(uint16_t), 1, file);
                assert(material_index < mesh->material_transition_count);
                material_apply(&mesh->transition_materials[material_index]);
                break;
            }
            case MESH2D_CMD_ATTR:
            {
                uint8_t attrs;
                fread(&attrs, sizeof(uint8_t), 1, file);
                menu_set_attr_flags(attrs);
                break;
            }
            case MESH2D_CMD_MOVE_TO:
            {
                menu2d_vtx_t vtx;
                fread(&vtx, sizeof(menu2d_vtx_t), 1, file);
                menu_move_to(&vtx);
                break;
            }
            case MESH2D_CMD_LINE_TO:
            {
                menu2d_vtx_t vtx;
                fread(&vtx, sizeof(menu2d_vtx_t), 1, file);
                menu_line_to(&vtx);
                break;
            }
            default:
                assert(false);
        }
    }

    mesh->block = rspq_block_end();
}

void mesh2d_release(mesh2d_t* mesh) {
    if (mesh->material) {
        material_pair_release(mesh->material);
    }

    for (int i = 0; i < mesh->material_transition_count; i += 1) {
        material_release(&mesh->transition_materials[i]);
    }

    free(mesh->transition_materials);

    rspq_block_free(mesh->block);
}