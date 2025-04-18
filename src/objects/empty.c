#include "empty.h"

void empty_init(struct empty* empty, struct empty_definition* definition) {
    empty->position = definition->position;
}

void empty_destroy(struct empty* empty, struct empty_definition* definition) {
    // do nothing
}