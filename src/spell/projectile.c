#include "projectile.h"

#include <stdbool.h>

#include "../render/render_scene.h"
#include "../time/time.h"
#include "../collision/collision_scene.h"

#include "assets.h"

#define PROJECTILE_SPEED    10.0f

static struct dynamic_object_type projectile_collision = {
    .minkowsi_sum = dynamic_object_sphere_minkowski_sum,
    .bounding_box = dynamic_object_sphere_bounding_box,
    .data = {
        .sphere = {
            .radius = 0.25f,
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

    render_batch_add_tmesh(batch, spell_assets_get()->projectile_mesh, mtx, NULL);
}

void projectile_init(struct projectile* projectile, struct spell_data_source* data_source, struct spell_event_options event_options) {
    render_scene_add(&projectile->pos, 0.2f, (render_scene_callback)projectile_render, projectile);

    projectile->data_source = data_source;
    projectile->data_output = NULL;

    projectile->pos = data_source->position;
    projectile->has_hit = 0;
    projectile->has_primary_event = event_options.has_primary_event;
    projectile->has_secondary_event = event_options.has_secondary_event;
    projectile->is_controlled = 0;

    spell_data_source_retain(data_source);

    dynamic_object_init(
        entity_id_new(), 
        &projectile->dynamic_object, 
        &projectile_collision, 
        COLLISION_LAYER_DAMAGE_ENEMY,
        &projectile->pos, 
        NULL
    );
    collision_scene_add(&projectile->dynamic_object);

    vector3Scale(&data_source->direction, &projectile->dynamic_object.velocity, PROJECTILE_SPEED);
    projectile->dynamic_object.has_gravity = 0;

    if (data_source->flags.controlled) {
        projectile->is_controlled = 1;
    }
}

bool projectile_is_active(struct projectile* projectile) {
    if (projectile->has_primary_event || !projectile->has_secondary_event) {
        return !projectile->has_hit;
    }

    if (!projectile->data_output) {
        return false;
    }

    return projectile->data_output->reference_count > 1;
}

void projectile_update(struct projectile* projectile, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    if (projectile->is_controlled) {
        vector3Scale(&projectile->data_source->direction, &projectile->dynamic_object.velocity, PROJECTILE_SPEED);
    }

    if (projectile->data_source->flags.cast_state != SPELL_CAST_STATE_ACTIVE) {
        projectile->is_controlled = 0;
        projectile->dynamic_object.has_gravity = 1;
    }

    if (projectile->has_secondary_event) {
        if (!projectile->data_output) {
            projectile->data_output = spell_data_source_pool_get(&spell_sources->data_sources);

            if (projectile->data_output) {
                spell_data_source_retain(projectile->data_output);
                projectile->data_output->direction = projectile->data_source->direction;
                projectile->data_output->position = projectile->data_source->position;
                projectile->data_output->flags = projectile->data_source->flags;
                projectile->data_output->target = projectile->dynamic_object.entity_id;

                spell_event_listener_add(event_listener, SPELL_EVENT_SECONDARY, projectile->data_output, 0.0f);
            }
        } else {
            projectile->data_output->position = projectile->pos;
            projectile->data_output->direction = projectile->data_source->direction;
            projectile->data_output->flags.cast_state = projectile->data_source->flags.cast_state;
        }
    }

    if (projectile->dynamic_object.active_contacts && !projectile->has_hit) {
        if (projectile->has_primary_event) {
            struct contact* first_contact = projectile->dynamic_object.active_contacts;

            struct spell_data_source* hit_source = spell_data_source_pool_get(&spell_sources->data_sources);

            if (hit_source) {
                hit_source->direction = first_contact->normal;
                hit_source->position = first_contact->point;
                hit_source->flags = projectile->data_source->flags;
                hit_source->flags.cast_state = SPELL_CAST_STATE_INSTANT;
                hit_source->target = first_contact->other_object;
                spell_event_listener_add(event_listener, SPELL_EVENT_PRIMARY, hit_source, 0.0f);
            }
        }

        projectile->has_hit = 1;
    }

    if (!projectile_is_active(projectile)) {
        spell_event_listener_add(event_listener, SPELL_EVENT_DESTROY, NULL, 0.0f);
    }
}

void projectile_destroy(struct projectile* projectile) {
    render_scene_remove(projectile);
    spell_data_source_release(projectile->data_source);
    if (projectile->data_output) {
        spell_data_source_release(projectile->data_output);
    }
    collision_scene_remove(&projectile->dynamic_object);
}