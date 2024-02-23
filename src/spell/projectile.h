#ifndef __SPELL_PROJECTILE_H__
#define __SPELL_PROJECTILE_H__

#include "../math/vector3.h"
#include "spell_data_source.h"

struct projectile {
    struct Vector3 pos;
    struct Vector3 vel;
    struct spell_data_source* data_source;
    struct spell_data_source* data_output;
    int render_id;
};

void projectile_init(struct projectile* projectile, struct spell_data_source* source, struct spell_data_source* data_output);
void projectile_destroy(struct projectile* projectile);

#endif