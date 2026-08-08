#include "door.h"

#include "../cutscene/cutscene_runner.h"
#include "../cutscene/expression_evaluate.h"
#include "../time/time.h"

static door_lock_definition_t magic_lock = {
    .mesh_filename = "rom:/meshes/objects/doors/lock.tmesh",
    .interact_blocker = NULL,
};

#define GEM_3_COUNT     3

bool door_gem_3_lock_interact(struct interactable* interactable, entity_id from) {
    door_t* door = (door_t*)interactable->data;

    if (expression_get_bool(door->unlocked)) {
        return true;
    }

    int gem_count = expression_get_integer(door->key_lock_3);

    if (gem_count < GEM_3_COUNT) { 
        struct cutscene_builder builder;
        cutscene_builder_init(&builder);

        cutscene_builder_pause(&builder, true, false);

        char message[48];
        sprintf(message, "you need %d more gems to open the door", GEM_3_COUNT - gem_count);
        cutscene_builder_dialog(&builder, message);

        cutscene_builder_pause(&builder, false, false);

        cutscene_runner_run(
            cutscene_builder_finish(&builder),
            0,
            cutscene_runner_free_on_finish(),
            door,
            0
        );
        
        return false;
    }

    expression_set_integer(door->key_lock_3, gem_count - GEM_3_COUNT);
    expression_set_bool(door->unlocked, true);

    door_base_unlock(&door->door_base);

    return true;
}

static door_lock_definition_t gem_3_lock = {
    .mesh_filename = "rom:/meshes/objects/doors/gem_lock_ftrials_3p.tmesh",
    .animations_filename = "rom:/meshes/objects/doors/gem_lock_ftrials_3p.anim",
    .interact_blocker = door_gem_3_lock_interact,
};

void door_update(void* data) {
    struct door* door = (struct door*)data;
    door_base_update(&door->door_base);

    if (door->key_lock_3 == VARIABLE_DISCONNECTED && door->unlocked != VARIABLE_DISCONNECTED) {
        bool should_unlock = expression_get_bool(door->unlocked);

        if (should_unlock != door_base_is_unlocked(&door->door_base)) {
            if (should_unlock) {
                door_base_unlock(&door->door_base);
            } else {
                door_base_lock(&door->door_base, &magic_lock);
            }
        }
    }
}

void door_init(struct door* door, struct door_definition* definition, entity_id id) {
    door_base_init(&door->door_base, (door_base_definition_t*)definition, id, "rom:/meshes/objects/doors/door.tmesh");
    door->unlocked = definition->unlocked;
    door->key_lock_3 = definition->key_lock_3;
    update_add(door, door_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);

    if (door->key_lock_3 != VARIABLE_DISCONNECTED && !expression_get_bool(definition->unlocked)) {
        door_base_lock(&door->door_base, &gem_3_lock);
    }
}

void door_destroy(struct door* door) {
    door_base_destroy(&door->door_base);
    update_remove(door);
}

void door_common_init() {

}

void door_common_destroy() {

}