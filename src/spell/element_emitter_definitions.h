#ifndef __SPELL_ELEMENT_EMITTER_DEFINITIONS_H__
#define __SPELL_ELEMENT_EMITTER_DEFINITIONS_H__

#include "element_emitter.h"

struct element_emitter_definition* element_emitter_find_def(enum element_type element_type, bool has_earth, bool has_air, bool has_fire, bool has_ice);

#endif