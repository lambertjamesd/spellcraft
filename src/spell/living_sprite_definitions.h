#ifndef __SPELL_LIVING_SPRITE_DEFINITIONS_H__
#define __SPELL_LIVING_SPRITE_DEFINITIONS_H__

#include "./living_sprite.h"

struct living_sprite_definition* living_sprite_find_def(enum element_type element_type, bool has_air, bool has_fire, bool has_ice);

#endif