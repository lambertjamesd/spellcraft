#include "projectile.h"

#include "../render/render_scene.h"
#include "../time/time.h"
#include "../collision/collision_scene.h"

#include "assets.h"

#define PROJECTILE_SPEED    10.0f

static struct dynamic_object_type projectile_collision = {
    .minkowsi_sum = dynamic_object_box_minkowski_sum,
    .bounding_box = 0,
    .data = {
        .box = {
            .half_size = {0.25f, 0.25f, 0.25f},
        }
    },
    .bounce = 0.4f,
    .friction = 0.25f,
};

void projectile_render(struct projectile* projectile, struct render_batch* batch) {
    mat4x4* mtx = render_batch_get_transform(batch);

    if (!mtx) {
        return;
    }

    struct Transform transform;
    transform.position = projectile->pos;
    quatIdent(&transform.rotation);
    transform.scale = gOneVec;
    transformToMatrix(&transform, *mtx);

    render_batch_add_mesh(batch, spell_assets_get()->projectile_mesh, mtx);
}

void projectile_init(struct projectile* projectile, struct spell_data_source* data_source) {
    projectile->render_id = render_scene_add(&r_scene_3d, &projectile->pos, 0.2f, (render_scene_callback)projectile_render, projectile);

    projectile->data_source = data_source;
    projectile->data_output = NULL;

    projectile->pos = data_source->position;
    projectile->has_hit = 0;
    // TODO determine this somehow
    projectile->has_primary_event = 0;

    data_source->reference_count += 1;

    dynamic_object_init(&projectile->dynamic_object, &projectile_collision, &projectile->pos, NULL);
    collision_scene_add(&projectile->dynamic_object);

    vector3Scale(&data_source->direction, &projectile->dynamic_object.velocity, PROJECTILE_SPEED);

    if (!data_source->flags.controlled) {
        projectile->dynamic_object.velocity.y += PROJECTILE_SPEED * 0.5f;
    }
}

void projectile_update(struct projectile* projectile, struct spell_event_listener* event_listener, struct spell_data_source_pool* pool) {
    if (!projectile->data_output) {
        projectile->data_output = spell_data_source_pool_get(pool);

        if (projectile->data_output) {
            projectile->data_output->reference_count += 1;
            *projectile->data_output = *projectile->data_source;

            spell_event_listener_add(event_listener, SPELL_EVENT_SECONDARY, projectile->data_output);
        }
    } else {
        projectile->data_output->position = projectile->pos;
        projectile->data_output->direction = projectile->data_source->direction;
    }

    if (projectile->dynamic_object.active_contacts && !projectile->has_hit) {
        struct contact* first_contact = projectile->dynamic_object.active_contacts;

        struct spell_data_source* hit_source = spell_data_source_pool_get(pool);

        if (hit_source) {
            hit_source->reference_count = 0;
            hit_source->direction = first_contact->normal;
            hit_source->position = first_contact->point;
            hit_source->flags = projectile->data_source->flags;
            spell_event_listener_add(event_listener, SPELL_EVENT_PRIMARY, hit_source);
        }

        projectile->has_hit = 1;
    }

    if (projectile->has_hit) {
        spell_event_listener_add(event_listener, SPELL_EVENT_DESTROY, NULL);
    }
}

void projectile_destroy(struct projectile* projectile) {
    render_scene_remove(&r_scene_3d, projectile->render_id);
    projectile->render_id = 0;
    projectile->data_source->reference_count -= 1;
    if (projectile->data_output) {
        projectile->data_output->reference_count -= 1;
    }
    collision_scene_remove(&projectile->dynamic_object);
}