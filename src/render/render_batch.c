#include "render_batch.h"

#include "../util/sort.h"
#include "../time/time.h"

void render_batch_init(struct render_batch* batch, struct Transform* camera_transform, struct frame_memory_pool* pool) {
    batch->element_count = 0;
    batch->transform_count = 0;
    batch->sprite_count = 0;
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

    return result;
}

void render_batch_add_tmesh(struct render_batch* batch, struct tmesh* mesh, mat4x4* transform, struct armature* armature) {
    struct render_batch_element* element = render_batch_add(batch);

    if (!element) {
        return;
    }

    element->mesh.block = mesh->block;
    element->material = mesh->material;
    element->mesh.transform = transform;
    element->mesh.armature = armature && armature->bone_count ? armature : NULL;
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

    result.sprites = &batch->sprites[batch->sprite_count];

    if (batch->sprite_count + count > RENDER_BATCH_TRANSFORM_COUNT) {
        result.sprite_count = RENDER_BATCH_TRANSFORM_COUNT - batch->sprite_count;
    } else {
        result.sprite_count = count;
    }

    batch->sprite_count += result.sprite_count;

    return result;
}

mat4x4* render_batch_get_transform(struct render_batch* batch) {
    if (batch->transform_count >= RENDER_BATCH_TRANSFORM_COUNT) {
        return NULL;
    }

    mat4x4* result = &batch->transform[batch->transform_count];
    ++batch->transform_count;
    return result;
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

void render_batch_finish(struct render_batch* batch, mat4x4 view_proj_matrix, struct render_viewport* viewport) {
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

    glViewport(viewport->x, viewport->y, viewport->w, viewport->h);
    glScissor(viewport->x, viewport->y, viewport->w, viewport->h);

    glEnable(GL_CULL_FACE);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_RDPQ_MATERIAL_N64);
    // glEnable(GL_RDPQ_TEXTURING_N64);
    rdpq_set_mode_standard();
    rdpq_mode_persp(true);

    bool is_sprite_mode = false;

    for (int i = 0; i < batch->element_count; ++i) {
        int index = order[i];
        struct render_batch_element* element = &batch->elements[index];

        if (current_mat != element->material) {
            if (element->material->block) {
                rspq_block_run(element->material->block);
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

            if (element->mesh.transform) {
                T3DMat4FP* mtxfp = frame_malloc(batch->pool, sizeof(T3DMat4FP));

                if (!mtxfp) {
                    continue;
                }

                t3d_mat4_to_fixed(mtxfp, (const T3DMat4*)element->mesh.transform);
                t3d_matrix_push(mtxfp);
            }

            if (element->mesh.armature) {
                mat4x4 pose[element->mesh.armature->bone_count];

                for(uint32_t i=0; i<element->mesh.armature->bone_count; i++)
                {
                    int parent_index = element->mesh.armature->parent_linkage[i];

                    assert(parent_index == NO_BONE_PARENT || parent_index < i);

                    if (parent_index == NO_BONE_PARENT) {
                        transformToMatrix(&element->mesh.armature->pose[i], pose[i]);
                    } else {
                        mat4x4 tmp;
                        transformToMatrix(&element->mesh.armature->pose[i], tmp);
                        matrixMul(pose[parent_index], tmp, pose[i]);
                    }
                }
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

                int screen_x = (int)(x * (viewport->w)) + viewport->x;
                int screen_y = (int)(y * (viewport->h)) + viewport->y;

                int half_screen_width = (int)(size * scale_x * viewport->w);
                int half_screen_height = (int)(size * scale_y * viewport->h);

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

    glDisable(GL_RDPQ_MATERIAL_N64);
    glDisable(GL_RDPQ_TEXTURING_N64);
}