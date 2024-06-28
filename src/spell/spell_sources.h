#ifndef __SPELL_SPELL_SOURCES_H__
#define __SPELL_SPELL_SOURCES_H__

#include "spell_data_source.h"
#include "mana_pool.h"

struct spell_sources {
    struct spell_data_source_pool data_sources;
    struct mana_pool mana_pool; 
};

void spell_sources_init(struct spell_sources* sources);

#endif