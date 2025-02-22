#include "element_emitter_definitions.h"

#include "assets.h"
#include "../effects/lightning_effect.h"
#include "../effects/scale_in_fade_out.h"
#include "../collision/shapes/sweep.h"
#include "../collision/shapes/cylinder.h"
#include "../collision/shapes/cone.h"
#include "../collision/shapes/sphere.h"

void* fire_effect_start(struct Vector3* pos, struct Vector3* direction, float radius) {
    return scale_in_fade_out_new(spell_assets_get()->fire_sweep_mesh, pos, direction, radius);
}

void* fire_around_effect_start(struct Vector3* pos, struct Vector3* direction, float radius) {
    return scale_in_fade_out_new(spell_assets_get()->fire_around_mesh, pos, direction, radius);
}

void* fire_push_effect_start(struct Vector3* pos, struct Vector3* direction, float radius) {
    return scale_in_fade_out_new(spell_assets_get()->fire_push_mesh, pos, direction, radius);
}

void* ice_effect_start(struct Vector3* pos, struct Vector3* direction, float radius) {
    return scale_in_fade_out_new(spell_assets_get()->ice_sweep_mesh, pos, direction, radius);
}

void* ice_push_effect_start(struct Vector3* pos, struct Vector3* direction, float radius) {
    return scale_in_fade_out_new(spell_assets_get()->ice_push_mesh, pos, direction, radius);
}

void* ice_around_effect_start(struct Vector3* pos, struct Vector3* direction, float radius) {
    return scale_in_fade_out_new(spell_assets_get()->ice_around_mesh, pos, direction, radius);
}

static struct lightning_effect_def lightning_def = {
    .spread = 1.1f,
};

void* lightning_effect_start(struct Vector3* pos, struct Vector3* direction, float radius) {
    return lightning_effect_new(pos, &lightning_def);
}

static struct lightning_effect_def lightning_around_def = {
    .spread = 3.14f,
};

void* lightning_around_effect_start(struct Vector3* pos, struct Vector3* direction, float radius) {
    return scale_in_fade_out_new(spell_assets_get()->lightning_around_mesh, pos, direction, radius);
}

void* water_around_effect_start(struct Vector3* pos, struct Vector3* direction, float radius) {
    return scale_in_fade_out_new(spell_assets_get()->water_around_mesh, pos, direction, radius);
}

void* water_spray_effect_start(struct Vector3* pos, struct Vector3* direction, float radius) {
    return scale_in_fade_out_new(spell_assets_get()->water_spray_mesh, pos, direction, radius);
}

void effect_nop_stop(void*) {};
bool effect_always_stopped(void*) {
    return false;
}

static struct element_emitter_definition fire_definition = {
    .element_type = ELEMENT_TYPE_FIRE,
    .collider_type = {
        .minkowsi_sum = cone_minkowski_sum,
        .bounding_box = cone_bounding_box,
        .data = {
            .cone = {
                .size = {0.3f, 0.3f, 1.0f},
            }
        }
    },
    .scale = 4.0f,
    .mana_per_second = 1.0f,
    .damage_per_frame = 1.0f,
    .on_effect_start = fire_push_effect_start,
    .on_effect_update = (on_effect_update)scale_in_fade_out_set_transform,
    .on_effect_stop = (on_effect_stop)scale_in_fade_out_stop,
    .is_effect_running = (is_effect_running)scale_in_fade_out_is_running,
    .effect_free = (effect_free)scale_in_fade_out_free,
};

static struct element_emitter_definition fire_around_definition = {
    .element_type = ELEMENT_TYPE_FIRE,
    .collider_type = {
        .minkowsi_sum = sphere_minkowski_sum,
        .bounding_box = sphere_bounding_box,
        .data = {
            .sphere = {
                .radius = 1.3f,
            }
        }
    },
    .scale = 2.0f,
    .mana_per_second = 1.0f,
    .damage_per_frame = 1.0f,
    .on_effect_start = fire_around_effect_start,
    .on_effect_update = (on_effect_update)scale_in_fade_out_set_transform,
    .on_effect_stop = (on_effect_stop)scale_in_fade_out_stop,
    .is_effect_running = (is_effect_running)scale_in_fade_out_is_running,
    .effect_free = (effect_free)scale_in_fade_out_free,
};

static struct element_emitter_definition fire_push_definition = {
    .element_type = ELEMENT_TYPE_FIRE,
    .collider_type = {
        .minkowsi_sum = cone_minkowski_sum,
        .bounding_box = cone_bounding_box,
        .data = {
            .cone = {
                .size = {0.3f, 0.3f, 1.0f},
            }
        }
    },
    .scale = 4.0f,
    .mana_per_second = 1.0f,
    .damage_per_frame = 1.0f,
    .on_effect_start = fire_push_effect_start,
    .on_effect_update = (on_effect_update)scale_in_fade_out_set_transform,
    .on_effect_stop = (on_effect_stop)scale_in_fade_out_stop,
    .is_effect_running = (is_effect_running)scale_in_fade_out_is_running,
    .effect_free = (effect_free)scale_in_fade_out_free,
};

static struct element_emitter_definition ice_definition = {
    .element_type = ELEMENT_TYPE_ICE,
    .collider_type = {
        .minkowsi_sum = cone_minkowski_sum,
        .bounding_box = cone_bounding_box,
        .data = {
            .cone = {
                .size = {0.3f, 0.3f, 1.0f},
            }
        }
    },
    .scale = 4.0f,
    .mana_per_second = 1.0f,
    .damage_per_frame = 1.0f,
    .on_effect_start = ice_push_effect_start,
    .on_effect_update = (on_effect_update)scale_in_fade_out_set_transform,
    .on_effect_stop = (on_effect_stop)scale_in_fade_out_stop,
    .is_effect_running = (is_effect_running)scale_in_fade_out_is_running,
    .effect_free = (effect_free)scale_in_fade_out_free,
};

static struct element_emitter_definition ice_push_definition = {
    .element_type = ELEMENT_TYPE_ICE,
    .collider_type = {
        .minkowsi_sum = sweep_minkowski_sum,
        .bounding_box = sweep_bounding_box,
        .data = {
            .sweep = {
                .range = {0.707f, 0.707f},
                .radius = 1.0f,
                .half_height = 0.0625f,
            }
        }
    },
    .scale = 4.0f,
    .mana_per_second = 1.0f,
    .damage_per_frame = 1.0f,
    .on_effect_start = ice_push_effect_start,
    .on_effect_update = (on_effect_update)scale_in_fade_out_set_transform,
    .on_effect_stop = (on_effect_stop)scale_in_fade_out_stop,
    .is_effect_running = (is_effect_running)scale_in_fade_out_is_running,
    .effect_free = (effect_free)scale_in_fade_out_free,
};

static struct element_emitter_definition ice_around_definition = {
    .element_type = ELEMENT_TYPE_ICE,
    .collider_type = {
        .minkowsi_sum = sphere_minkowski_sum,
        .bounding_box = sphere_bounding_box,
        .data = {
            .sphere = {
                .radius = 1.3f,
            }
        }
    },
    .scale = 2.0f,
    .mana_per_second = 1.0f,
    .damage_per_frame = 1.0f,
    .on_effect_start = ice_around_effect_start,
    .on_effect_update = (on_effect_update)scale_in_fade_out_set_transform,
    .on_effect_stop = (on_effect_stop)scale_in_fade_out_stop,
    .is_effect_running = (is_effect_running)scale_in_fade_out_is_running,
    .effect_free = (effect_free)scale_in_fade_out_free,
};

static struct element_emitter_definition lightning_definition = {
    .element_type = ELEMENT_TYPE_LIGHTNING,
    .collider_type = {
        .minkowsi_sum = sweep_minkowski_sum,
        .bounding_box = sweep_bounding_box,
        .data = {
            .sweep = {
                .range = {0.707f, 0.707f},
                .radius = 1.0f,
                .half_height = 0.0625f,
            }
        }
    },
    .scale = 4.0f,
    .mana_per_second = 1.0f,
    .damage_per_frame = 1.0f,
    .on_effect_start = lightning_effect_start,
    .on_effect_update = (on_effect_update)lightning_effect_set_position,
    .on_effect_stop = effect_nop_stop,
    .is_effect_running = effect_always_stopped,
    .effect_free = (effect_free)lightning_effect_free,
};

static struct element_emitter_definition lightning_around_definition = {
    .element_type = ELEMENT_TYPE_LIGHTNING,
    .collider_type = {
        .minkowsi_sum = sphere_minkowski_sum,
        .bounding_box = sphere_bounding_box,
        .data = {
            .sphere = {
                .radius = 1.3f,
            }
        }
    },
    .scale = 2.0f,
    .mana_per_second = 1.0f,
    .damage_per_frame = 1.0f,
    .on_effect_start = lightning_around_effect_start,
    .on_effect_update = (on_effect_update)scale_in_fade_out_set_transform,
    .on_effect_stop = (on_effect_stop)scale_in_fade_out_stop,
    .is_effect_running = (is_effect_running)scale_in_fade_out_is_running,
    .effect_free = (effect_free)scale_in_fade_out_free,
};


static struct element_emitter_definition water_around_definition = {
    .element_type = ELEMENT_TYPE_WATER,
    .collider_type = {
        .minkowsi_sum = sphere_minkowski_sum,
        .bounding_box = sphere_bounding_box,
        .data = {
            .sphere = {
                .radius = 1.3f,
            }
        }
    },
    .scale = 2.0f,
    .mana_per_second = 1.0f,
    .damage_per_frame = 1.0f,
    .on_effect_start = water_around_effect_start,
    .on_effect_update = (on_effect_update)scale_in_fade_out_set_transform,
    .on_effect_stop = (on_effect_stop)scale_in_fade_out_stop,
    .is_effect_running = (is_effect_running)scale_in_fade_out_is_running,
    .effect_free = (effect_free)scale_in_fade_out_free,
};

static struct element_emitter_definition water_spray_definition = {
    .element_type = ELEMENT_TYPE_WATER,
    .collider_type = {
        .minkowsi_sum = cone_minkowski_sum,
        .bounding_box = cone_bounding_box,
        .data = {
            .cone = {
                .size = {0.3f, 0.3f, 1.0f},
            }
        }
    },
    .scale = 2.0f,
    .mana_per_second = 1.0f,
    .damage_per_frame = 1.0f,
    .on_effect_start = water_spray_effect_start,
    .on_effect_update = (on_effect_update)scale_in_fade_out_set_transform,
    .on_effect_stop = (on_effect_stop)scale_in_fade_out_stop,
    .is_effect_running = (is_effect_running)scale_in_fade_out_is_running,
    .effect_free = (effect_free)scale_in_fade_out_free,
};

#define ELEMENT_DEF_INDEX(has_air, has_fire, has_ice)   (((has_air) ? 0x4 : 0) + ((has_fire) ? 0x2 : 0) + ((has_ice) ? 0x1 : 0))

static struct element_emitter_definition* fire_definitions[] = {
    [ELEMENT_DEF_INDEX(0, 0, 0)] = &fire_definition,
    [ELEMENT_DEF_INDEX(0, 0, 1)] = &lightning_definition,
    [ELEMENT_DEF_INDEX(0, 1, 0)] = &fire_around_definition,
    [ELEMENT_DEF_INDEX(0, 1, 1)] = &lightning_around_definition,

    // TODO
    [ELEMENT_DEF_INDEX(1, 0, 0)] = &fire_push_definition,
    [ELEMENT_DEF_INDEX(1, 0, 1)] = &lightning_definition,
    [ELEMENT_DEF_INDEX(1, 1, 0)] = &fire_around_definition,
    [ELEMENT_DEF_INDEX(1, 1, 1)] = &lightning_around_definition,
};

static struct element_emitter_definition* ice_definitions[] = {
    [ELEMENT_DEF_INDEX(0, 0, 0)] = &ice_definition,
    // TODO
    [ELEMENT_DEF_INDEX(0, 0, 1)] = &ice_around_definition,
    [ELEMENT_DEF_INDEX(0, 1, 0)] = &water_spray_definition,
    [ELEMENT_DEF_INDEX(0, 1, 1)] = &water_around_definition,

    // TODO
    [ELEMENT_DEF_INDEX(1, 0, 0)] = &ice_push_definition,
    [ELEMENT_DEF_INDEX(1, 0, 1)] = &ice_definition,
    [ELEMENT_DEF_INDEX(1, 1, 0)] = &lightning_definition,
    [ELEMENT_DEF_INDEX(1, 1, 1)] = &lightning_around_definition,
};

struct element_emitter_definition* element_emitter_find_def(enum element_type element_type, bool has_earth, bool has_air, bool has_fire, bool has_ice) {
    switch (element_type) {
        case ELEMENT_TYPE_FIRE:
            return fire_definitions[ELEMENT_DEF_INDEX(has_air, has_earth, has_ice)];
        case ELEMENT_TYPE_ICE:
            return ice_definitions[ELEMENT_DEF_INDEX(has_air, has_fire, has_earth)];
        default:
            return NULL;
    }
}