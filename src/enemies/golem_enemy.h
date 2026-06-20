#ifndef __ENTITIES_GOLEM_ENEMY_H__
#define __ENTITIES_GOLEM_ENEMY_H__

#include "../entities/entity_deps.h"

enum golem_enemy_state {
    GOLEM_STATE_IDLE,
    GOLEM_STATE_ACTIVATING,
    GOLEM_STATE_FOLLOW,
    GOLEM_STATE_PUNCH,
    GOLEM_STATE_DEACTIVATE,
};

typedef enum golem_enemy_state golem_enemy_state_t;

struct golem_enemy {
    transform_sa_t transform;
    renderable_t renderable;
    animator_t animator;
    dynamic_object_t collider;
    dynamic_object_t fist_r_collider;
    vector3_t fist_r_position;
    golem_enemy_state_t state;
    boolean_variable activated;
    spatial_trigger_t vision;
    entity_id target;
    float animator_speed;
    float target_speed;
};

typedef struct golem_enemy golem_enemy_t;

void golem_enemy_init(golem_enemy_t* golem_enemy, struct golem_enemy_definition* definition, entity_id entity_id);
void golem_enemy_destroy(golem_enemy_t* golem_enemy, struct golem_enemy_definition* definition);
void golem_enemy_common_init();
void golem_enemy_common_destroy();

#endif