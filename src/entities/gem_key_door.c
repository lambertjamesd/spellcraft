#include "gem_key_door.h"    

#include "../cutscene/cutscene_runner.h"
#include "../cutscene/expression_evaluate.h"
#include "../time/time.h"

void gem_key_door_update(void* data) {
    gem_key_door_t* door = (gem_key_door_t*)data;
    door_base_update(&door->door_base);
}

bool gem_key_door_interact(struct interactable* interactable, entity_id from) {
    gem_key_door_t* door = (gem_key_door_t*)interactable->data;

    if (expression_get_bool(door->unlocked)) {
        return true;
    }

    int gem_count = expression_get_integer(door->key_gems);

    if (gem_count < door->gem_count) { 
        struct cutscene_builder builder;
        cutscene_builder_init(&builder);

        cutscene_builder_pause(&builder, true, false);

        char message[48];
        sprintf(message, "you need %d more gems to open the door", door->gem_count - gem_count);
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

    expression_set_integer(door->key_gems, gem_count - door->gem_count);
    expression_set_bool(door->unlocked, true);

    struct cutscene_builder builder;
    cutscene_builder_init(&builder);

    cutscene_builder_pause(&builder, true, false);

    char message[48];
    sprintf(message, "%d gems were spent to unlock the door", door->gem_count);
    cutscene_builder_dialog(&builder, message);

    cutscene_builder_pause(&builder, false, false);

    cutscene_runner_run(
        cutscene_builder_finish(&builder),
        0,
        cutscene_runner_free_on_finish(),
        door,
        0
    );

    door_base_set_locked(&door->door_base, false, NULL);

    return true;
}
    
void gem_key_door_init(gem_key_door_t* gem_key_door, struct gem_key_door_definition* definition, entity_id entity_id) {
    door_base_init(&gem_key_door->door_base, (door_base_definition_t*)definition, entity_id);

    gem_key_door->unlocked = definition->unlocked;
    gem_key_door->key_gems = definition->key_gems;
    gem_key_door->gem_count = definition->gem_count;

    door_base_set_locked(&gem_key_door->door_base, false, expression_get_bool(gem_key_door->unlocked) ? NULL : gem_key_door_interact);

    update_add(gem_key_door, gem_key_door_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);
}

void gem_key_door_destroy(gem_key_door_t* gem_key_door, struct gem_key_door_definition* definition) {
    door_base_destroy(&gem_key_door->door_base);
    update_remove(gem_key_door);
}

void gem_key_door_common_init() {

}

void gem_key_door_common_destroy() {

}
