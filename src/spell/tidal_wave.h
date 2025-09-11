#ifndef __SPELL_TIDAL_WAVE_H__
#define __SPELL_TIDAL_WAVE_H__

#include "../math/transform_single_axis.h"
#include "../render/renderable.h"

#include "spell_sources.h"
#include "spell_event.h"

struct tidal_wave {
    transform_sa_t transform;
    renderable_t renderable;
   float timer; 
};

typedef struct tidal_wave tidal_wave_t;

void tidal_wave_init(tidal_wave_t* tidal_wave, struct spell_data_source* source, struct spell_event_options event_options);
void tidal_wave_destroy(tidal_wave_t* storm);
bool tidal_wave_update(tidal_wave_t* storm);

#endif