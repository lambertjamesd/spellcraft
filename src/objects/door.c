#include "door.h"

void door_init(struct door* door, struct door_definition* definition) {
    door->transform.position = definition->position;
    door->transform.rotation = definition->rotation;
}

void door_destroy(struct door* door) {

}