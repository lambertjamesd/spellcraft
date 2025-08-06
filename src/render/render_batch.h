#ifndef __RENDER_RENDER_BATCH_H__
#define __RENDER_RENDER_BATCH_H__

#include <libdragon.h>
#include <t3d/t3d.h>

#include "tmesh.h"
#include "armature.h"
#include "material.h"
#include "frame_alloc.h"

#include "../math/matrix.h"
#include "../math/transform.h"
#include "../math/transform_single_axis.h"

#define RENDER_BATCH_MAX_SIZE   256
#define RENDER_BATCH_TRANSFORM_COUNT    64
#define MAX_BILLBOARD_SPRITES           128

struct render_billboard_sprite {
    struct Vector3 position;
    float radius;
    color_t color;
};

enum render_batch_type {
    RENDER_BATCH_MESH,
    RENDER_BATCH_BILLBOARD,
    RENDER_BATCH_CALLBACK,
};

struct render_batch_billboard_element {
    struct render_billboard_sprite* sprites;
    uint16_t sprite_count;
};

struct render_batch;

typedef void (*RenderCallback)(void* data, struct render_batch* batch);

enum element_attr_type {
    ELEMENT_ATTR_NONE,
    ELEMENT_ATTR_POSE,
    ELEMENT_ATTR_IMAGE,
    ELEMENT_ATTR_PRIM_COLOR,
};

struct element_attr {
    uint8_t type;
    uint8_t offset;
    union {
        struct {
            T3DMat4FP* pose;
        } pose;
        struct {
            sprite_t* sprite;
        } image;
        struct {
            color_t color;
        } prim;
    };
};

struct render_batch_element {
    struct material* material;
    uint16_t type;
    uint8_t light_source;
    union {
        struct {
            rspq_block_t* block;
            // T3DMat4FP* if transform_count == 1, T3DMat4FP** if > 1
            void* transform;
            short transform_count;
            uint8_t attr_count;
            struct element_attr* attrs;
            color_t color;
            uint8_t use_prim_color;
        } mesh;
        struct render_batch_billboard_element billboard;
        struct {
            RenderCallback callback;
            void* data;
        } callback;
    };
};

struct render_batch {
    mat4x4 camera_matrix;
    struct frame_memory_pool* pool;
    struct render_batch_element elements[RENDER_BATCH_MAX_SIZE];
    short element_count;
};

void render_batch_init(struct render_batch* batch, struct Transform* camera_transform, struct frame_memory_pool* pool);

struct render_batch_element* render_batch_add(struct render_batch* batch);

struct render_batch_element* render_batch_add_tmesh(struct render_batch* batch, struct tmesh* mesh, void* transform, int transform_count, struct armature* armature, struct tmesh** attachments);

void render_batch_add_callback(struct render_batch* batch, struct material* material, RenderCallback callback, void* data);
// caller is responsible for populating sprite list
// the sprite count returned may be less than the sprite count requested
struct render_batch_billboard_element* render_batch_add_particles(struct render_batch* batch, struct material* material, int count);

struct render_batch_billboard_element render_batch_get_sprites(struct render_batch* batch, int count);
mat4x4* render_batch_get_transform(struct render_batch* batch);
T3DMat4FP* render_batch_get_transformfp(struct render_batch* batch);
T3DMat4FP* render_batch_transformfp_from_sa(struct render_batch* batch, struct TransformSingleAxis* transform);
T3DMat4FP* render_batch_transformfp_from_full(struct render_batch* batch, struct Transform* transform);
// !!! This stomps on the input pose so don't attempt to use it after calling this function
T3DMat4FP* render_batch_build_pose(T3DMat4* pose, int bone_count);

void render_batch_relative_mtx(struct render_batch* batch, mat4x4 into);

void render_batch_finish(struct render_batch* batch, mat4x4 view_proj, T3DViewport* viewport);

#endif