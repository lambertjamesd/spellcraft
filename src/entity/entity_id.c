#include "entity_id.h"

static entity_id next_id = 0;

entity_id entity_id_new() {
    next_id += 1;
    return next_id;
}