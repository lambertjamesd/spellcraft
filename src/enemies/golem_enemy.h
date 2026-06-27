#ifndef __ENTITIES_GOLEM_ENEMY_H__
#define __ENTITIES_GOLEM_ENEMY_H__

#include "../entities/entity_deps.h"

#include "../effects/sword_trail.h"

enum golem_enemy_state {
    GOLEM_STATE_IDLE,
    GOLEM_STATE_ACTIVATING,
    GOLEM_STATE_FOLLOW,
    GOLEM_STATE_PUNCH,
    GOLEM_STATE_SPIN_ATTACK,
    GOLEM_STATE_SEARCH,
    GOLEM_STATE_DEACTIVATE,
    GOLEM_STATE_DIE,
};

typedef enum golem_enemy_state golem_enemy_state_t;

struct golem_fist {
    dynamic_object_t collider;
    vector3_t position;
    sword_trail_t* trail;
    armature_attachment_t* attachment;
    bool is_active;
};

typedef struct golem_fist golem_fist_t;

struct golem_enemy {
    transform_sa_t transform;
    renderable_t renderable;
    animator_t animator;
    dynamic_object_t collider;

    health_t health;

    golem_fist_t fist_r;
    golem_fist_t fist_l;

    golem_enemy_state_t state;
    boolean_variable activated;
    entity_id target;
    bool was_activated;
    spatial_trigger_t vision;
    vector2_t head_rotation;
    transform_sa_t head_transform;
    float animator_speed;
    float target_speed;
    float attack_timer;
};

typedef struct golem_enemy golem_enemy_t;

void golem_enemy_init(golem_enemy_t* golem_enemy, struct golem_enemy_definition* definition, entity_id entity_id);
void golem_enemy_destroy(golem_enemy_t* golem_enemy, struct golem_enemy_definition* definition);
void golem_enemy_common_init();
void golem_enemy_common_destroy();

#endif