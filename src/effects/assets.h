#ifndef __EFFECTS_ASSET_H__
#define __EFFECTS_ASSET_H__

#include "../render/tmesh.h"

struct effect_assets {
    struct tmesh* drop_shadow;
    struct tmesh* hit_effect;
};

void effect_assets_init();
struct effect_assets* effect_assets_get();

#endif