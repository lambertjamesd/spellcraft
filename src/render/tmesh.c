#include "tmesh.h"

// T3MS
#define EXPECTED_HEADER 0x54334D53

enum TMeshCommandType {
    TMESH_COMMAND_VERTICES,
    TMESH_COMMAND_TRIANGLES,
};

void tmesh_load(struct tmesh* tmesh, FILE* file) {
    int header;
    fread(&header, 1, 4, file);
    assert(header == EXPECTED_HEADER);

    fread(&tmesh->vertex_count, sizeof(uint16_t), 1, file);
    tmesh->vertices = malloc(sizeof(T3DVertPacked) * tmesh->vertex_count);
    fread(&tmesh->vertices[0], sizeof(T3DVertPacked), tmesh->vertex_count, file);

    uint16_t command_count;
    fread(&command_count, sizeof(uint16_t), 1, file);

    rspq_block_begin();

    for (uint16_t i = 0; i < command_count; i += 1) {
        uint8_t command;
        fread(&command, sizeof(uint8_t), 1, file);

        switch (command) 
        {
            case TMESH_COMMAND_VERTICES:
                {
                    uint8_t offset;
                    uint8_t count;
                    uint16_t vertex_source;
                    fread(&offset, sizeof(uint8_t), 1, file);
                    fread(&count, sizeof(uint8_t), 1, file);
                    fread(&vertex_source, sizeof(uint16_t), 1, file);
                    t3d_vert_load(&tmesh->vertices[vertex_source], offset, count);
                    break;
                }
            case TMESH_COMMAND_TRIANGLES:
            {
                uint8_t triangle_count;
                fread(&triangle_count, sizeof(uint8_t), 1, file);

                for (int i = 0 ; i < triangle_count; i += 1) {
                    uint8_t indices[3];
                    fread(&indices[0], sizeof(uint8_t), 3, file);
                    t3d_tri_draw(indices[0], indices[1], indices[2]);
                }
                t3d_tri_sync();
                break;
            }
            default:
                assert(false);
        }
    }

    tmesh->block = rspq_block_end();
}

void tmesh_release(struct tmesh* tmesh) {
    rspq_block_free(tmesh->block);
    free(tmesh->vertices);
}