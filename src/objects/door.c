#include "door.h"

#include "../cutscene/expression_evaluate.h"
#include "../time/time.h"

void door_update(void* data) {
    struct door* door = (struct door*)data;
    door_base_update(&door->door_base);
    bool is_unlocked = door->unlocked == VARIABLE_DISCONNECTED ? true : expression_get_bool(door->unlocked);
    door_base_set_locked(&door->door_base, !is_unlocked, NULL);
}

void door_init(struct door* door, struct door_definition* definition, entity_id id) {
    door_base_init(&door->door_base, (door_base_definition_t*)definition, id);
    door->unlocked = definition->unlocked;
    update_add(door, door_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);
}

void door_destroy(struct door* door) {
    door_base_destroy(&door->door_base);
    update_remove(door);
}

void door_common_init() {

}

void door_common_destroy() {

}