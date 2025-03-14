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
    result->mesh.transform_count = 0;
    result->mesh.pose = NULL;
    result->mesh.tmp_fixed_pose = NULL;
    result->mesh.color = (color_t){255, 255, 255, 255};
    result->mesh.use_prim_color = 0;

    return result;
}

T3DMat4FP* render_batch_build_pose(T3DMat4* pose, int bone_count) {
    if (!pose) {
        return NULL;
    }

    T3DMat4* end = pose + bone_count;

    for(T3DMat4* curr = pose; curr < end; curr += 1) {
        T3DMat4FP tmp;
        t3d_mat4_to_fixed(&tmp, curr);
        *((T3DMat4FP*)curr) = tmp;
    }

    data_cache_hit_writeback_invalidate(pose, sizeof(T3DMat4) * bone_count);

    return (T3DMat4FP*)pose;
}

void render_batch_relative_mtx(struct render_batch* batch, mat4x4 into) {
    into[3][0] -= batch->camera_matrix[3][0] * MODEL_WORLD_SCALE;
    into[3][1] -= batch->camera_matrix[3][1] * MODEL_WORLD_SCALE;
    into[3][2] -= batch->camera_matrix[3][2] * MODEL_WORLD_SCALE;
}

struct render_batch_element* render_batch_add_tmesh(struct render_batch* batch, struct tmesh* mesh, void* transform, int transform_count, struct armature* armature, struct tmesh** attachments) {
    struct render_batch_element* element = render_batch_add(batch);

    if (!element) {
        return NULL;
    }

    assert(!transform == !transform_count);

    element->mesh.block = mesh->block;
    element->material = mesh->material;
    element->mesh.transform = transform;
    element->mesh.transform_count = transform_count;
    element->mesh.pose = NULL;
    element->mesh.tmp_fixed_pose = UncachedAddr(mesh->armature_pose);

    if (armature && armature->bone_count) {
        T3DMat4* pose = armature_build_pose(armature, batch->pool);
        element->mesh.bone_count = armature->bone_count;
        element->mesh.pose = render_batch_build_pose(pose, armature->bone_count);


        if (attachments && mesh->attatchments) {
            for (int i = 0; i < mesh->attatchment_count; i += 1) {
                if (!attachments[i]) {
                    continue;
                }

                T3DMat4FP** matrices = frame_malloc(batch->pool, sizeof(T3DMat4FP*) * (transform_count + 2));

                if (!matrices) {
                    continue;
                }

                int matrix_index = 0;

                if (transform_count > 1) {
                    for (matrix_index = 0; matrix_index < transform_count; matrix_index += 1) {
                        matrices[matrix_index] = ((T3DMat4FP**)transform)[matrix_index];
                    }
                } else {
                    matrices[matrix_index++] = transform;
                }


                struct armature_attatchment* linkage = &mesh->attatchments[i];

                matrices[matrix_index++] = &element->mesh.pose[linkage->bone_index];
                matrices[matrix_index++] = &linkage->local_transform;

                render_batch_add_tmesh(batch, attachments[i], matrices, transform_count + 2, NULL, NULL);
            }
        }
    }

    return element;
}

void render_batch_add_callback(struct render_batch* batch, struct material* material, RenderCallback callback, void* data) {
    struct render_batch_element* element = render_batch_add(batch);

    if (!element) {
        return;
    }

    element->type = RENDER_BATCH_CALLBACK;
    element->material = material;
    element->callback.callback = callback;
    element->callback.data = data;
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
    return UncachedAddr(frame_malloc(batch->pool, sizeof(T3DMat4FP)));
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
    if (!tex->texture_enabled || (!tex->scroll_x && !tex->scroll_y)) {
        return;
    }

    int w = tex->width << 2;
    int h = tex->height << 2;

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
        x_offset + tex->s0, y_offset + tex->t0, 
        x_offset + tex->s1, y_offset + tex->t1
    );
}

static bool element_type_2d[] = {
    [RENDER_BATCH_MESH] = false,
    [RENDER_BATCH_BILLBOARD] = true,
    [RENDER_BATCH_CALLBACK] = false,
};

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
    ) * 0.5f * 4;

    float scale_y = sqrtf(
        view_proj_matrix[1][0] * view_proj_matrix[1][0] + 
        view_proj_matrix[1][1] * view_proj_matrix[1][1] +
        view_proj_matrix[1][2] * view_proj_matrix[1][2]
    ) * 0.5f * 4;

    sort_indices(order, batch->element_count, batch, (sort_compare)render_batch_compare_element);

    struct material* current_mat = 0;

    rdpq_set_mode_standard();
    rdpq_mode_persp(true);
    rdpq_mode_zbuf(true, true);
    t3d_state_set_drawflags(T3D_FLAG_DEPTH | T3D_FLAG_SHADED | T3D_FLAG_TEXTURED);

    bool is_sprite_mode = false;
    bool z_write = true;
    bool z_read = true;

    T3DMat4FP* default_mtx = render_batch_get_transformfp(batch);

    if (default_mtx) {
        mat4x4 scaleMtx;
        matrixFromScale(scaleMtx, MODEL_WORLD_SCALE);
        struct Vector3 camera_neg_pos = {
            batch->camera_matrix[3][0],
            batch->camera_matrix[3][1],
            batch->camera_matrix[3][2],
        };
        matrixApplyScaledPos(scaleMtx, &camera_neg_pos, -WORLD_SCALE);
        t3d_mat4_to_fixed_3x4(default_mtx, (T3DMat4*)scaleMtx);
    }

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

        bool should_sprite_mode = element_type_2d[element->type];

        if (should_sprite_mode != is_sprite_mode) {
            if (should_sprite_mode) {
                rdpq_mode_persp(false);
            } else {
                rdpq_mode_zoverride(false, 0, 0);
                rdpq_mode_persp(true);
            }

            is_sprite_mode = should_sprite_mode;
        }

        if (element->type == RENDER_BATCH_MESH) {
            if (!element->mesh.block) {
                continue;
            }

            if (element->mesh.pose && element->mesh.tmp_fixed_pose) {
                memcpy(element->mesh.tmp_fixed_pose, element->mesh.pose, sizeof(T3DMat4FP) * element->mesh.bone_count);
            }

            if (element->mesh.transform_count) {
                assert(element->mesh.transform);
                if (element->mesh.transform_count == 1) {
                    t3d_matrix_push(element->mesh.transform);
                } else {
                    for (int mtx_index = 0; mtx_index < element->mesh.transform_count; mtx_index += 1) {
                        t3d_matrix_push(((T3DMat4FP**)element->mesh.transform)[mtx_index]);
                    }
                }
            } else if (default_mtx) {
                t3d_matrix_push(default_mtx);
            }

            if (element->mesh.use_prim_color) {
                rdpq_set_prim_color(element->mesh.color);
            }

            rspq_block_run(element->mesh.block);

            if (element->mesh.transform_count) {
                t3d_matrix_pop(element->mesh.transform_count);
            } else if (default_mtx) {
                t3d_matrix_pop(1);
            }
        } else if (element->type == RENDER_BATCH_BILLBOARD) {
            for (int sprite_index = 0; sprite_index < element->billboard.sprite_count; ++sprite_index) {
                struct render_billboard_sprite sprite = element->billboard.sprites[sprite_index];

                struct Vector4 transformed;
                struct Vector3 scaled;
                vector3Scale(&sprite.position, &scaled, WORLD_SCALE);
                matrixVec3Mul(view_proj_matrix, &scaled, &transformed);

                if (transformed.w < 0.0f) {
                    continue;
                }

                float wInv = 1.0f / transformed.w;

                float x = (transformed.x * wInv + 1.0f) * 0.5f * 4.0f;
                float y = (-transformed.y * wInv + 1.0f) * 0.5f * 4.0f;
                float z = transformed.z * wInv * 0.5f + 0.5f;

                float size = sprite.radius * wInv * WORLD_SCALE;

                if (z < 0.0f || z > 1.0f) {
                    continue;
                }

                rdpq_mode_zoverride(true, z, 0);

                int screen_x = (int)(x * (viewport->size[0])) + viewport->offset[0] * 4;
                int screen_y = (int)(y * (viewport->size[1])) + viewport->offset[1] * 4;

                int half_screen_width = (int)(size * scale_x * viewport->size[0]);
                int half_screen_height = (int)(size * scale_y * viewport->size[1]);

                int image_w = 32;
                int image_h = 32;

                if (current_mat && current_mat->tex0.sprite) {
                    image_w = current_mat->tex0.sprite->width * 32;
                    image_h = current_mat->tex0.sprite->height * 32;
                }

                rdpq_set_prim_color(sprite.color);
                __rdpq_texture_rectangle_scaled_fx(
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
        } else if (element->type == RENDER_BATCH_CALLBACK) {
            if (default_mtx) {
                t3d_matrix_push(default_mtx);
            }
            element->callback.callback(element->callback.data, batch);
            if (default_mtx) {
                t3d_matrix_pop(1);
            }
        }
    }
}