#include "damage.h"

#include <memory.h>

void damaged_set_reset(struct damaged_set* set) {
    memset(set, 0, sizeof(struct damaged_set));
}

bool damaged_set_check(struct damaged_set* set, entity_id id) {
    if (!set) {
        return true;
    }

    entity_id* max = set->damaged_entities + MAX_DAMAGED_SET_SIZE;

    for (entity_id* it = set->damaged_entities; it < max; ++it) {
        if (*it == id) {
            return false;
        }

        if (*it == 0) {
            *it = id;
            return true;
        }
    }

    return true;
}