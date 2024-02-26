#ifndef __SPELL_PROJECTILE_H__
#define __SPELL_PROJECTILE_H__

#include "../math/vector3.h"
#include "spell_data_source.h"
#include "../collision/dynamic_object.h"

struct projectile {
    struct Vector3 pos;
    struct spell_data_source* data_source;
    struct spell_data_source* data_output;
    struct dynamic_object dynamic_object;
    int render_id;
};

void projectile_init(struct projectile* projectile, struct spell_data_source* source, struct spell_data_source* data_output);
void projectile_destroy(struct projectile* projectile);

void projectile_update(struct projectile* projectile);

#endif