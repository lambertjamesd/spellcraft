#include "room_portal.h"


void room_portal_init(struct room_portal* portal, struct room_portal_definition* definition) {
    transformSaInit(&portal->transform, &definition->position, &definition->rotation, definition->scale);
    portal->room_a = definition->room_a;
    portal->room_b = definition->room_b;
}

void room_portal_destroy(struct room_portal* portal) {

}