#include "inventory.h"

#include "../cutscene/expression_evaluate.h"
#include "../savefile/savefile.h"

extern struct global_location inventory_item_locations[ITEM_TYPE_COUNT];

struct spell flame_spell = {
    .symbols = {
        {.primary_rune = SPELL_SYMBOL_AIR, .flaming = 1}
    },
    .length = 1,

    .symbol_index = SPELL_ICON_FIRE,
};

struct spell dash_spell = {
    .symbols = {
        {.primary_rune = SPELL_SYMBOL_FIRE, .windy = 1}
    },
    .length = 1,

    .symbol_index = SPELL_ICON_DASH,
};

struct spell projectile_spell = {
    .symbols = {
        {.primary_rune = SPELL_SYMBOL_EARTH, .windy = 1}
    },
    .length = 1,

    .symbol_index = SPELL_ICON_DASH,
};


struct spell lightning_spell = {
    .symbols = {
        {.primary_rune = SPELL_SYMBOL_FIRE, .icy = 1}
    },
    .length = 1,

    .symbol_index = SPELL_ICON_FIRE,
};

enum inventory_item_type staff_item_types[INV_STAFF_COUNT] = {
    ITEM_TYPE_STAFF_DEFAULT,
    ITEM_TYPE_NONE,
    ITEM_TYPE_NONE,
    ITEM_TYPE_NONE,
};

struct inventory inventory;

void inventory_init() {
    memset(inventory.spell_slots, 0, sizeof(inventory.spell_slots));

    memset(inventory.built_in_spells, 0, sizeof(inventory.built_in_spells));

    inventory.built_in_spells[0] = &flame_spell;
    inventory.built_in_spells[1] = &dash_spell;

    inventory.spell_slots[0] = &flame_spell;
    inventory.spell_slots[1] = &dash_spell;
    inventory.spell_slots[2] = &projectile_spell;
    inventory.spell_slots[3] = &lightning_spell;

    for (int i = 0; i < MAX_CUSTOM_SPELLS; i += 1) {
        spell_init(&inventory.custom_spells[i], SPELL_ICON_CUSTOM_0 + i);
    }

    inventory.equipped_staff = &staff_stats_none;
}

void inventory_destroy() {
    
}

bool inventory_is_upgrade_item(enum inventory_item_type type) {
    return type >= SPELL_SYMBOL_FIRE && type <= SPELL_SYMBOL_LIFE;
}

void inventory_unlock_item(enum inventory_item_type type) {
    if (type >= ITEM_TYPE_COUNT) {
        return;
    }

    struct global_location* global = &inventory_item_locations[type];

    assert(global->data_type);

    if (inventory_is_upgrade_item(type)) {
        int prev = evaluation_context_load(savefile_get_globals(), global->data_type, global->word_offset);
        evaluation_context_save(savefile_get_globals(), global->data_type, global->word_offset, prev + 1);
    } else {
        evaluation_context_save(savefile_get_globals(), global->data_type, global->word_offset, true);
    }
}

int inventory_get_item_level(enum inventory_item_type type) {
    if (!inventory_is_upgrade_item(type)) {
        return 0;
    }

    struct global_location* global = &inventory_item_locations[type];
    return evaluation_context_load(savefile_get_globals(), global->data_type, global->word_offset);
}

struct staff_stats* inventory_equipped_staff() {
    return inventory.equipped_staff;
}

void inventory_equip_staff(enum inventory_item_type type) {
    for (int i = 0; i < INV_STAFF_COUNT; i += 1) {
        if (staff_stats[i].item_type == type) {
            inventory.equipped_staff = &staff_stats[i];
            return;
        }
    }

    inventory.equipped_staff = &staff_stats_none;
}

struct spell* inventory_get_equipped_spell(unsigned index) {
    assert(index < MAX_SPELL_SLOTS);
    return inventory.spell_slots[index];
}

void inventory_set_equipped_spell(unsigned index, struct spell* spell) {
    assert(index < MAX_SPELL_SLOTS);
    inventory.spell_slots[index] = spell;
}

struct spell* inventory_get_built_in_spell(unsigned x, unsigned y) {
    assert(x < INVENTORY_SPELL_COLUMNS && y < INVENTORY_SPELL_ROWS);
    return inventory.built_in_spells[
        y * INVENTORY_SPELL_COLUMNS + x
    ];
}

struct spell* inventory_get_custom_spell(unsigned index) {
    assert(index < MAX_CUSTOM_SPELLS);
    return &inventory.custom_spells[index];
}

bool inventory_has_item(enum inventory_item_type item) {
    return inventory_get_count(item) != 0;
}

void inventory_set_has_item(enum inventory_item_type item, bool value) {
    if (item >= ITEM_TYPE_COUNT || item == ITEM_TYPE_NONE) {
        return;
    }

    struct global_location* global = &inventory_item_locations[item];

    if (!global->data_type) {
        debugf("item %d has no mapping\n", item);
    }

    assert(global->data_type);

    evaluation_context_save(savefile_get_globals(GLOBAL_ACCESS_MODE_WRITE), global->data_type, global->word_offset, value);
}

int inventory_get_count(enum inventory_item_type item) {
    if (item >= ITEM_TYPE_COUNT || item == ITEM_TYPE_NONE) {
        return false;
    }

    struct global_location* global = &inventory_item_locations[item];

    if (!global->data_type) {
        debugf("item %d has no mapping\n", item);
    }

    assert(global->data_type);

    return evaluation_context_load(savefile_get_globals(GLOBAL_ACCESS_MODE_READ), global->data_type, global->word_offset);
}

boolean_variable inventory_get_item_ref(enum inventory_item_type item) {
    if (item >= ITEM_TYPE_COUNT || item == ITEM_TYPE_NONE) {
        return VARIABLE_DISCONNECTED;
    }

    struct global_location* global = &inventory_item_locations[item];

    if (!global->data_type) {
        debugf("item %d has no mapping\n", item);
    }

    assert(global->data_type == DATA_TYPE_BOOL);

    return global->word_offset;
}