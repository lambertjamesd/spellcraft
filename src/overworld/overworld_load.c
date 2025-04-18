#include "overworld_load.h"

#include <malloc.h>
#include <assert.h>
#include "../render/render_scene.h"
#include "../collision/collision_scene.h"
#include "overworld_render.h"
#include "../resource/mesh_collider.h"
#include "../resource/tmesh_cache.h"
#include "../math/transform.h"
#include <memory.h>

struct overworld_tile* overworld_tile_load(FILE* file) {
    uint8_t mesh_count;
    fread(&mesh_count, 1, 1, file);
    uint16_t detail_mesh_count;
    fread(&detail_mesh_count, 2, 1, file);
    uint16_t detail_count;
    fread(&detail_count, 2, 1, file);

    struct overworld_tile* result = malloc(
        sizeof(struct overworld_tile) +
        sizeof(struct tmesh) * mesh_count +
        sizeof(rspq_block_t*) * mesh_count +
        sizeof(struct tmesh*) * detail_mesh_count +
        sizeof(T3DMat4FP) * detail_count
    );

    fread(&result->starting_y, sizeof(float), 1, file);
    result->terrain_mesh_count = mesh_count;
    result->detail_mesh_count = detail_mesh_count;
    result->detail_count = detail_count;

    result->terrain_meshes = (struct tmesh*)(result + 1);
    result->render_blocks = (rspq_block_t**)(result->terrain_meshes + mesh_count);
    result->detail_meshes = (struct tmesh**)(result->render_blocks + mesh_count);
    result->detail_matrices = (T3DMat4FP*)(result->detail_meshes + detail_mesh_count);

    for (int i = 0; i < detail_mesh_count; i += 1) {
        uint8_t filename_len;
        fread(&filename_len, 1, 1, file);
        char filename[filename_len + 1];
        fread(filename, filename_len, 1, file);
        filename[filename_len] = '\0';

        result->detail_meshes[i] = tmesh_cache_load(filename);
    }

    T3DMat4FP* curr_matrix = UncachedAddr(result->detail_matrices);

    for (int i = 0; i < mesh_count; i += 1) {
        
        tmesh_load(&result->terrain_meshes[i], file);
        rspq_block_begin();
        rspq_block_run(result->terrain_meshes[i].material->block);
        rspq_block_run(result->terrain_meshes[i].block);
        
        uint16_t block_detail_count;
        fread(&block_detail_count, 2, 1, file);
        
        struct material* curr_material = NULL;
        for (int detail_index = 0; detail_index < block_detail_count; detail_index += 1) {
            uint16_t detail_type;
            fread(&detail_type, 2, 1, file);
            struct Transform transform;
            fread(&transform, sizeof(struct Transform), 1, file);

            struct tmesh* mesh = result->detail_meshes[detail_type];

            if (curr_material != mesh->material) {
                rspq_block_run(mesh->material->block);
                curr_material = mesh->material;
            }

            T3DMat4 mtx;
            transformToMatrix(&transform, mtx.m);
            t3d_mat4_to_fixed(curr_matrix, &mtx);
            t3d_matrix_push(curr_matrix);
            rspq_block_run(mesh->block);
            t3d_matrix_pop(1);

            ++curr_matrix;
        }

        result->render_blocks[i] = rspq_block_end();
    }

    return result;
}

void overworld_tile_free(struct overworld_tile* tile) {
    for (int i = 0; i < tile->terrain_mesh_count; i += 1) {
        tmesh_release(&tile->terrain_meshes[i]);
    }
    for (int i = 0; i < tile->detail_mesh_count; i += 1) {
        tmesh_cache_release(tile->detail_meshes[i]);
    }
    free(tile);
}

struct overworld_actor_tile* overworld_actor_tile_load(FILE* file) {
    struct overworld_actor_tile* result = malloc(sizeof(struct overworld_actor_tile));
    mesh_collider_load(&result->collider, file);
    return result;
}

void overworld_actor_tile_free(struct overworld_actor_tile* tile) {
    mesh_collider_release(&tile->collider);
    free(tile);
}

// EXPR
#define EXPECTED_OVERWORLD_HEADER 0x4F565744

void overworld_render_step(void* data, mat4x4 view_proj_matrix, struct Camera* camera, T3DViewport* viewport, struct frame_memory_pool* pool) {
    struct overworld* overworld = (struct overworld*)data;
    overworld_render(overworld, view_proj_matrix, camera, viewport, pool);
}

struct overworld* overworld_load(const char* filename) {
    struct overworld* result = malloc(sizeof(struct overworld));
    memset(result, 0, sizeof(struct overworld));

    result->file = fopen(filename, "rb");

    int header;
    fread(&header, 1, 4, result->file);
    assert(header == EXPECTED_OVERWORLD_HEADER);

    fread(&result->tile_x, sizeof(uint16_t), 1, result->file);
    fread(&result->tile_y, sizeof(uint16_t), 1, result->file);
    fread(&result->min, sizeof(struct Vector2), 1, result->file);
    fread(&result->tile_size, sizeof(float), 1, result->file);

    fread(&result->lod0.entry_count, 1, 1, result->file);

    result->lod0.entries = malloc(sizeof(struct overworld_lod0_entry) * result->lod0.entry_count);

    for (int i = 0; i < result->lod0.entry_count; i += 1) {
        fread(&result->lod0.entries[i].x, sizeof(uint16_t), 1, result->file);
        fread(&result->lod0.entries[i].z, sizeof(uint16_t), 1, result->file);
        fread(&result->lod0.entries[i].priority, sizeof(uint16_t), 1, result->file);
        tmesh_load(&result->lod0.entries[i].mesh, result->file);
    }

    result->inv_tile_size = 1.0f / result->tile_size;
    result->tile_definitions = malloc(result->tile_x * result->tile_y * sizeof(struct overworld_tile_def));

    fread(result->tile_definitions, sizeof(struct overworld_tile_def), result->tile_x * result->tile_y, result->file);
    
    result->load_next.x = NO_TILE_COORD;
    result->load_next.y = NO_TILE_COORD;

    render_scene_add_step(overworld_render_step, result);

    return result;
}

void overworld_free(struct overworld* overworld) {
    for (int x = 0; x < 4; x += 1) {
        for (int y = 0; y < 4; y += 1) {
            if (overworld->loaded_tiles[x][y]) {
                overworld_tile_free(overworld->loaded_tiles[x][y]);
            }
        }
    }

    for (int i = 0; i < overworld->lod0.entry_count; i += 1) {
        tmesh_release(&overworld->lod0.entries[i].mesh);
    }
    free(overworld->lod0.entries);

    render_scene_remove_step(overworld);

    fclose(overworld->file);
    free(overworld->tile_definitions);
    free(overworld);
}

void overworld_check_loaded_tiles(struct overworld* overworld) {
    if (overworld->load_next.x == NO_TILE_COORD || overworld->load_next.y == NO_TILE_COORD) {
        return;
    }

    int slot_x = overworld->load_next.x & 0x3;
    int slot_y = overworld->load_next.y & 0x3;

    if (overworld->loaded_tiles[slot_x][slot_y]) {
        rdpq_call_deferred((void (*)(void*))overworld_tile_free, overworld->loaded_tiles[slot_x][slot_y]);
    }

    fseek(overworld->file, overworld->tile_definitions[overworld->load_next.x + overworld->load_next.y * overworld->tile_x].visual_block_offset, SEEK_SET);
    
    struct overworld_tile* tile = overworld_tile_load(overworld->file);
    overworld->loaded_tiles[slot_x][slot_y] = tile;
    overworld->render_blocks[slot_x][slot_y] = (struct overworld_tile_render_block){
        .render_blocks = tile->render_blocks,
        .x = overworld->load_next.x,
        .z = overworld->load_next.y,
        .y_height = tile->terrain_mesh_count,
        .starting_y = tile->starting_y,
    };

    overworld->load_next.x = NO_TILE_COORD;
    overworld->load_next.y = NO_TILE_COORD;
}

void overworld_check_collider_tiles(struct overworld* overworld, struct Vector3* player_pos) {
    float tile_x = (player_pos->x - overworld->min.x) * overworld->inv_tile_size;
    float tile_y = (player_pos->z - overworld->min.y) * overworld->inv_tile_size;

    int min_x = (int)floorf(tile_x - 0.5f);
    int min_y = (int)floorf(tile_y - 0.5f);

    int max_x = (int)ceilf(tile_x + 0.5f);
    int max_y = (int)ceilf(tile_y + 0.5f);

    for (int x = min_x; x < max_x; x += 1) {
        for (int y = min_y; y < max_y; y += 1) {
            struct overworld_actor_tile** slot = &overworld->loaded_actor_tiles[x & 0x1][y & 0x1];
        
            if (*slot) {
                if ((*slot)->x == x && (*slot)->y == y) {
                    // already loaded
                    continue;;
                }
        
                collision_scene_remove_static_mesh(&(*slot)->collider);
                overworld_actor_tile_free(*slot);
            }
        
            if (x < 0 || y < 0 || x >= overworld->tile_x || y >= overworld->tile_y) {
                *slot = NULL;
                continue;
            }
            
            fseek(overworld->file, overworld->tile_definitions[x + y * overworld->tile_x].actor_block_offset, SEEK_SET);
            *slot = overworld_actor_tile_load(overworld->file);
            (*slot)->x = x;
            (*slot)->y = y;
            collision_scene_add_static_mesh(&(*slot)->collider);
        }
    }
}