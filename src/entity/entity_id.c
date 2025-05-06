#include "entity_id.h"

static entity_id next_id = ENTITY_ID_FIRST_DYNAMIC;

entity_id entity_id_new() {
    return next_id++;
}