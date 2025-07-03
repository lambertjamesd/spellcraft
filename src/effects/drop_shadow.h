#ifndef __EFFECTS_DROP_SHADOW_H__
#define __EFFECTS_DROP_SHADOW_H__

#include "../collision/dynamic_object.h"
#include "../render/tmesh.h"

struct drop_shadow {
    struct dynamic_object* target;
    struct tmesh* mesh;
};

void drop_shadow_init(struct drop_shadow* drop_shadow, struct dynamic_object* target);
void drop_shadow_destroy(struct drop_shadow* drop_shadow);

#endif