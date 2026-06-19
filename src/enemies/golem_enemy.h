#ifndef __ENTITIES_GOLEM_ENEMY_H__
#define __ENTITIES_GOLEM_ENEMY_H__

#include "../entities/entity_deps.h"

struct golem_enemy {
    transform_sa_t transform;
    renderable_t renderable;
    animator_t animator;
    dynamic_object_t collider;
    boolean_variable activated;
    spatial_trigger_t vision;
    entity_id target;
    bool is_active;
};

typedef struct golem_enemy golem_enemy_t;

void golem_enemy_init(golem_enemy_t* golem_enemy, struct golem_enemy_definition* definition, entity_id entity_id);
void golem_enemy_destroy(golem_enemy_t* golem_enemy, struct golem_enemy_definition* definition);
void golem_enemy_common_init();
void golem_enemy_common_destroy();

#endif