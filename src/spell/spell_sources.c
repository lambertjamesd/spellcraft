#include "spell_sources.h"

static struct mana_pool_definition default_mana_definition = {
    .max_mana = 100.0f,
    .mana_regen_rate = 20.0f,
    .power_scale = 1.0f,
};

void spell_sources_init(struct spell_sources* sources) {
    spell_data_source_pool_init(&sources->data_sources);
    mana_pool_init(&sources->mana_pool, &default_mana_definition);
}