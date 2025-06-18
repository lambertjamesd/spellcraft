#include "projectile.h"

#include <stdbool.h>

#include "../collision/collision_scene.h"
#include "../collision/shapes/sphere.h"
#include "../entity/health.h"
#include "../render/defs.h"
#include "../render/render_scene.h"
#include "../time/time.h"

#include "assets.h"

static float projectile_speed[] = {
    [ELEMENT_TYPE_NONE] = 7.0f,
    [ELEMENT_TYPE_FIRE] = 10.0f,
    [ELEMENT_TYPE_ICE] = 10.0f,
    [ELEMENT_TYPE_LIGHTNING] = 100.0f,
};

static float projectile_vertical_speed[] = {
    [ELEMENT_TYPE_NONE] = 1.0f,
    [ELEMENT_TYPE_FIRE] = 0.0f,
    [ELEMENT_TYPE_ICE] = 3.0f,
    [ELEMENT_TYPE_LIGHTNING] = 0.0f,
};

static float projectile_accel[] = {
    [ELEMENT_TYPE_NONE] = 7.0f,
    [ELEMENT_TYPE_FIRE] = 0.0f,
    [ELEMENT_TYPE_ICE] = 6.0f,
    [ELEMENT_TYPE_LIGHTNING] = 0.0f,
};

static struct dynamic_object_type projectile_collision = {
    .minkowsi_sum = sphere_minkowski_sum,
    .bounding_box = sphere_bounding_box,
    .data = {
        .sphere = {
            .radius = 0.25f,
        }
    },
    .bounce = 0.4f,
    .friction = 0.25f,
};

void projectile_render(struct projectile* projectile, struct render_batch* batch) {
    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    mat4x4 mtx;
    struct Transform transform;
    transform.position = projectile->pos;
    quatIdent(&transform.rotation);
    transform.scale = gOneVec;
    transformToWorldMatrix(&transform, mtx);
    render_batch_relative_mtx(batch, mtx);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    struct tmesh* mesh;

    switch (projectile->element) {
        case ELEMENT_TYPE_FIRE:
            mesh = spell_assets_get()->projectile_mesh;
            break;
        case ELEMENT_TYPE_ICE:
            mesh = spell_assets_get()->projectile_ice_mesh;
            break;
        case ELEMENT_TYPE_LIGHTNING:
            mesh = spell_assets_get()->projectile_mesh;
            break;
        default:
            mesh = spell_assets_get()->projectile_mesh;
            break;
    }

    render_batch_add_tmesh(batch, mesh, mtxfp, 1, NULL, NULL);
}

void projectile_init(struct projectile* projectile, struct spell_data_source* data_source, union spell_modifier_flags modifiers, struct spell_event_options event_options, enum element_type element) {
    projectile->data_source = data_source;
    projectile->data_output = NULL;

    projectile->pos = data_source->position;
    projectile->has_hit = 0;
    projectile->has_primary_event = event_options.has_primary_event;
    projectile->has_secondary_event = event_options.has_secondary_event;
    projectile->is_controlled = 0;
    projectile->element = element;

    spell_data_source_retain(data_source);

    dynamic_object_init(
        entity_id_new(), 
        &projectile->dynamic_object, 
        &projectile_collision, 
        COLLISION_LAYER_DAMAGE_ENEMY,
        &projectile->pos, 
        NULL
    );
    projectile->dynamic_object.collision_group = COLLISION_GROUP_PLAYER;
    projectile->dynamic_object.density_class = DYNAMIC_DENSITY_HEAVY;

    vector3Scale(&data_source->direction, &projectile->dynamic_object.velocity, projectile_speed[element]);

    if (modifiers.flaming) {
        projectile->dynamic_object.has_gravity = 0;
    } else {
        projectile->dynamic_object.velocity.y = projectile_vertical_speed[element];
    }

    if (modifiers.living) {
        projectile->is_controlled = 1;
    }

    projectile->start_animation = mesh_animation_new(&projectile->pos, &gRight2, spell_assets_get()->projectile_appear, spell_assets_get()->projectile_appear_clip);
}

bool projectile_is_active(struct projectile* projectile) {
    // if (projectile->dynamic_object.is_out_of_bounds) {
    //     return false;
    // }

    if (projectile->has_primary_event || !projectile->has_secondary_event) {
        return !projectile->has_hit;
    }

    if (!projectile->data_output) {
        return false;
    }

    return projectile->data_output->reference_count > 1;
}

bool projectile_update(struct projectile* projectile, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    if (projectile->start_animation) {
        if (mesh_animation_update(projectile->start_animation)) {
            return true;
        }
        mesh_animation_free(projectile->start_animation);
        collision_scene_add(&projectile->dynamic_object);
        render_scene_add(&projectile->pos, 0.2f, (render_scene_callback)projectile_render, projectile);
        projectile->start_animation = NULL;
    }

    if (projectile->is_controlled) {
        vector3Scale(&projectile->data_source->direction, &projectile->dynamic_object.velocity, projectile_speed[projectile->element]);
    }

    if (projectile->data_source->flags.cast_state == SPELL_CAST_STATE_ACTIVE) {
        projectile->dynamic_object.velocity.y += fixed_time_step * projectile_accel[projectile->element];
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
        struct contact* first_contact = projectile->dynamic_object.active_contacts;

        if (projectile->has_primary_event) {
            struct spell_data_source* hit_source = spell_data_source_pool_get(&spell_sources->data_sources);

            if (hit_source) {
                hit_source->direction = first_contact->normal;
                hit_source->position = first_contact->point;
                hit_source->flags = projectile->data_source->flags;
                hit_source->flags.cast_state = SPELL_CAST_STATE_INSTANT;
                hit_source->target = first_contact->other_object;
                vector3AddScaled(&hit_source->position, &hit_source->direction, projectile_collision.data.sphere.radius, &hit_source->position);
                spell_event_listener_add(event_listener, SPELL_EVENT_PRIMARY, hit_source, 0.0f);
            }
        }

        struct health* health = health_get(first_contact->other_object);

        if (health) {
            enum damage_type damage_type = DAMAGE_TYPE_PROJECTILE;

            if (projectile->element == ELEMENT_TYPE_FIRE) {
                damage_type |= DAMAGE_TYPE_FIRE;
            } else if (projectile->element == ELEMENT_TYPE_ICE) {
                damage_type |= DAMAGE_TYPE_ICE;
            } else if (projectile->element == ELEMENT_TYPE_LIGHTNING) {
                damage_type |= DAMAGE_TYPE_LIGHTING;
            }

            struct damage_info damage = {
                .amount = 1.0f,
                .type = damage_type,
                .source = projectile->dynamic_object.entity_id,
                .direction = projectile->dynamic_object.velocity,
            };
            
            health_damage(health, &damage);
        }

        projectile->has_hit = 1;
    }

    return projectile_is_active(projectile);
}

void projectile_destroy(struct projectile* projectile) {
    if (projectile->start_animation) {
        mesh_animation_free(projectile->start_animation);
    } else {
        collision_scene_remove(&projectile->dynamic_object);
        render_scene_remove(projectile);
    }

    spell_data_source_release(projectile->data_source);
    if (projectile->data_output) {
        spell_data_source_release(projectile->data_output);
    }
}