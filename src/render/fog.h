#ifndef __RENDER_FOG_H__
#define __RENDER_FOG_H__

#include <libdragon.h>

enum fog_priority {
    FOG_PRIORITY_EFFECT,
    FOG_PRIORITY_ROOM,
    FOG_PRIORITY_SCENE,

    FOG_PRIORITY_COUNT,
};

typedef enum fog_priority fog_priority_t;

struct fog_state {
    color_t color;
    float min, max;
};

typedef struct fog_state fog_state_t;

void fog_set(fog_priority_t priority, fog_state_t state, float duration);
void fog_clear(fog_priority_t priority, float duration);

fog_state_t fog_get();

#endif