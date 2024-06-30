#include "render_batch.h"

#include "../util/sort.h"
#include "../time/time.h"
#include "defs.h"

void render_batch_init(struct render_batch* batch, struct Transform* camera_transform, struct frame_memory_pool* pool) {
    batch->element_count = 0;
    batch->pool = pool;

    transformToMatrix(camera_transform, batch->camera_matrix);
}

struct render_batch_element* render_batch_add(struct render_batch* batch) {
    if (batch->element_count >= RENDER_BATCH_MAX_SIZE) {
        return NULL;
    }

    struct render_batch_element* result = &batch->elements[batch->element_count];
    ++batch->element_count;

    result->material = NULL;
    result->type = RENDER_BATCH_MESH;
    result->mesh.block = 0;
    result->mesh.transform = NULL;
    result->mesh.armature = NULL;
    result->mesh.tmp_fixed_pose = NULL;

    return result;
}

void render_batch_add_tmesh(struct render_batch* batch, struct tmesh* mesh, T3DMat4FP* transform, struct armature* armature) {
    struct render_batch_element* element = render_batch_add(batch);

    if (!element) {
        return;
    }

    element->mesh.block = mesh->block;
    element->material = mesh->material;
    element->mesh.transform = transform;
    element->mesh.armature = armature && armature->bone_count ? armature : NULL;
    element->mesh.tmp_fixed_pose = mesh->armature_pose;
}

struct render_batch_billboard_element* render_batch_add_particles(struct render_batch* batch, struct material* material, int count) {
    struct render_batch_element* result = render_batch_add(batch);

    result->type = RENDER_BATCH_BILLBOARD;
    result->material = material;
    result->billboard = render_batch_get_sprites(batch, count);

    return &result->billboard;
}

struct render_batch_billboard_element render_batch_get_sprites(struct render_batch* batch, int count) {
    struct render_batch_billboard_element result;

    result.sprites = frame_malloc(batch->pool, count * sizeof(struct render_billboard_sprite));
    result.sprite_count = count;

    return result;
}

mat4x4* render_batch_get_transform(struct render_batch* batch) {
    return frame_malloc(batch->pool, sizeof(mat4x4));
}

T3DMat4FP* render_batch_get_transformfp(struct render_batch* batch) {
    return frame_malloc(batch->pool, sizeof(T3DMat4FP));
}

int render_batch_compare_element(struct render_batch* batch, uint16_t a_index, uint16_t b_index) {
    struct render_batch_element* a = &batch->elements[a_index];
    struct render_batch_element* b = &batch->elements[b_index];

    if (a == b) {
        return 0;
    }

    if (a->material->sort_priority != b->material->sort_priority) {
        return a->material->sort_priority - b->material->sort_priority;
    }

    if (a->material != b->material) {
        return (int)a->material - (int)b->material;
    }

    return a->type - b->type;
}

void render_batch_check_texture_scroll(int tile, struct material_tex* tex) {
    if (!tex->sprite || (!tex->scroll_x && !tex->scroll_y)) {
        return;
    }

    int w = tex->sprite->width << 2;
    int h = tex->sprite->height << 2;

    int x_offset = (int)(game_time * tex->scroll_x * w) % w;
    int y_offset = (int)(game_time * tex->scroll_y * h) % h;

    if (x_offset < 0) {
        x_offset += w;
    }

    if (y_offset < 0) {
        y_offset += h;
    }

    rdpq_set_tile_size_fx(
        tile, 
        x_offset, y_offset, 
        x_offset + w, 
        y_offset + h
    );
}

void render_batch_finish(struct render_batch* batch, mat4x4 view_proj_matrix, T3DViewport* viewport) {
    uint16_t order[RENDER_BATCH_MAX_SIZE];
    uint16_t order_tmp[RENDER_BATCH_MAX_SIZE];

    for (int i = 0; i < batch->element_count; ++i) {
        order[i] = i;
    }

    // used to scale billboard sprites
    float scale_x = sqrtf(
        view_proj_matrix[0][0] * view_proj_matrix[0][0] + 
        view_proj_matrix[0][1] * view_proj_matrix[0][1] +
        view_proj_matrix[0][2] * view_proj_matrix[0][2]
    ) * 0.5f;

    float scale_y = sqrtf(
        view_proj_matrix[1][0] * view_proj_matrix[1][0] + 
        view_proj_matrix[1][1] * view_proj_matrix[1][1] +
        view_proj_matrix[1][2] * view_proj_matrix[1][2]
    ) * 0.5f;

    sort_indices(order, batch->element_count, batch, (sort_compare)render_batch_compare_element);

    struct material* current_mat = 0;

    rdpq_set_mode_standard();
    rdpq_mode_persp(true);
    rdpq_mode_zbuf(true, true);
    t3d_state_set_drawflags(T3D_FLAG_DEPTH | T3D_FLAG_SHADED | T3D_FLAG_TEXTURED);

    bool is_sprite_mode = false;
    bool z_write = true;
    bool z_read = true;

    for (int i = 0; i < batch->element_count; ++i) {

        int index = order[i];
        struct render_batch_element* element = &batch->elements[index];

        if (current_mat != element->material) {
            if (element->material->block) {
                rspq_block_run(element->material->block);
            }

            render_batch_check_texture_scroll(TILE0, &element->material->tex0);
            render_batch_check_texture_scroll(TILE1, &element->material->tex1);

            bool need_z_write = (element->material->flags & MATERIAL_FLAGS_Z_WRITE) != 0;
            bool need_z_read = (element->material->flags & MATERIAL_FLAGS_Z_READ) != 0;

            if (need_z_write != z_write || need_z_read != z_read) {
                rdpq_mode_zbuf(need_z_read, need_z_write);
                z_write = need_z_write;
                z_read = need_z_read;
            }

            current_mat = element->material;
        }

        if (element->type == RENDER_BATCH_MESH) {
            if (is_sprite_mode) {
                rdpq_mode_zoverride(false, 0, 0);
                rdpq_mode_persp(true);
                is_sprite_mode = false;
            }

            if (!element->mesh.block) {
                continue;
            }

            if (element->mesh.armature && element->mesh.tmp_fixed_pose) {
                T3DMat4* pose = frame_malloc(batch->pool, sizeof(T3DMat4) * element->mesh.armature->bone_count);

                if (!pose) {
                    continue;
                }

                for(int i = 0; i < element->mesh.armature->bone_count; i++) {
                    int parent_index = element->mesh.armature->parent_linkage[i];

                    assert(parent_index == NO_BONE_PARENT || parent_index < i);

                    if (parent_index == NO_BONE_PARENT) {
                        transformToMatrix(&element->mesh.armature->pose[i], pose[i].m);
                    } else {
                        mat4x4 tmp;
                        transformToMatrix(&element->mesh.armature->pose[i], tmp);
                        matrixMul(pose[parent_index].m, tmp, pose[i].m);
                    }

                }

                for(int i = 0; i < element->mesh.armature->bone_count; i++) {
                    // TODO modify pose and 
                    t3d_mat4_to_fixed(&element->mesh.tmp_fixed_pose[i], &pose[i]);
                }

                data_cache_hit_writeback_invalidate(element->mesh.tmp_fixed_pose, sizeof(T3DMat4FP) * element->mesh.armature->bone_count);
            }

            if (element->mesh.transform) {
                data_cache_hit_writeback_invalidate(element->mesh.transform, sizeof(T3DMat4FP));
                t3d_matrix_push(element->mesh.transform);
            }

            rspq_block_run(element->mesh.block);

            if (element->mesh.transform) {
                t3d_matrix_pop(1);
            }
        } else if (element->type == RENDER_BATCH_BILLBOARD) {
            if (!is_sprite_mode) {
                is_sprite_mode = true;
                rdpq_mode_persp(false);
            }

            for (int sprite_index = 0; sprite_index < element->billboard.sprite_count; ++sprite_index) {
                struct render_billboard_sprite sprite = element->billboard.sprites[sprite_index];

                struct Vector4 transformed;
                matrixVec3Mul(view_proj_matrix, &sprite.position, &transformed);

                if (transformed.w < 0.0f) {
                    continue;
                }

                float wInv = 1.0f / transformed.w;

                float x = (transformed.x * wInv + 1.0f) * 0.5f;
                float y = (-transformed.y * wInv + 1.0f) * 0.5f;
                float z = transformed.z * wInv * 0.5f + 0.5f;

                float size = sprite.radius * wInv;

                if (z < 0.0f || z > 1.0f) {
                    continue;
                }

                rdpq_mode_zoverride(true, z, 0);

                int screen_x = (int)(x * (viewport->size[0])) + viewport->offset[0];
                int screen_y = (int)(y * (viewport->size[1])) + viewport->offset[1];

                int half_screen_width = (int)(size * scale_x * viewport->size[0]);
                int half_screen_height = (int)(size * scale_y * viewport->size[1]);

                int image_w = 32;
                int image_h = 32;

                if (current_mat && current_mat->tex0.sprite) {
                    image_w = current_mat->tex0.sprite->width;
                    image_h = current_mat->tex0.sprite->height;
                }

                rdpq_set_prim_color(sprite.color);
                rdpq_texture_rectangle_scaled(
                    TILE0, 
                    screen_x - half_screen_width, 
                    screen_y - half_screen_height, 
                    screen_x + half_screen_width, 
                    screen_y + half_screen_height, 
                    0,
                    0,
                    image_w,
                    image_h
                );
            }
        }
    }
}