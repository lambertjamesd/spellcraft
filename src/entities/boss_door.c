#include "boss_door.h"

#include "../time/time.h"
#include "../cutscene/expression_evaluate.h"
#include "../scene/scene.h"

bool boss_door_interact(interactable_t* interactable, entity_id from) {
    boss_door_t* door = (boss_door_t*)interactable->data;

    if (expression_get_bool(door->is_unlocked)) {
        return true;
    }

    scene_queue_next(door->on_interact);

    return false;
}

static door_lock_definition_t boss_door_lock = {
    .mesh_filename = "rom:/meshes/objects/doors/gem_lock_ftrials_boss.tmesh",
    .animations_filename = "rom:/meshes/objects/doors/gem_lock_ftrials_boss.anim",
    .interact_blocker = boss_door_interact,
};

void boss_door_update(void* data) {
    boss_door_t* door = (boss_door_t*)data;

    door_base_update(&door->door_base);

    if (expression_get_bool(door->is_unlocked) && !door_base_is_unlocked(&door->door_base)) {
        door_base_unlock(&door->door_base);
    }
}
    
void boss_door_init(boss_door_t* door, struct boss_door_definition* definition, entity_id entity_id) {
    door_base_init(&door->door_base, (door_base_definition_t*)definition, entity_id, "rom:/meshes/objects/doors/door.tmesh");
    door->is_unlocked = definition->unlocked;
    door->on_interact = definition->on_interact;
    update_add(door, boss_door_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);

    if (door->is_unlocked != VARIABLE_DISCONNECTED && !expression_get_bool(door->is_unlocked)) {
        door_base_lock(&door->door_base, &boss_door_lock);
    }
}

void boss_door_destroy(boss_door_t* door, struct boss_door_definition* definition) {
    door_base_destroy(&door->door_base);
    update_remove(door);
}

void boss_door_common_init() {

}

void boss_door_common_destroy() {

}
