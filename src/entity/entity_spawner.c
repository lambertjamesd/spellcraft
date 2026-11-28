#include "entity_spawner.h"

#include "../enemies/biter.h"
#include "../enemies/jelly.h"
#include "../enemies/jelly_king.h"
#include "../enemies/golem_enemy.h"
#include "../util/hash_map.h"

#include "../npc/npc.h"

#include "../objects/collectable.h"
#include "../objects/crate.h"
#include "../objects/door.h"
#include "../objects/ground_torch.h"
#include "../objects/training_dummy.h"
#include "../objects/treasure_chest.h"
#include "../objects/empty.h"
#include "../objects/water_cube.h"
#include "../objects/room_portal.h"
#include "../objects/burning_thorns.h"
#include "../objects/sign.h"
#include "../objects/trigger_cube.h"
#include "../objects/line_mesh.h"
#include "../entities/script_runner.h"
#include "../entities/pinwheel.h"
#include "../entities/breakable.h"
#include "../entities/jelly_pot.h"
#include "../entities/comm_stone.h"
#include "../entities/repair_scene.h"
#include "../entities/repair_part.h"
#include "../entities/item_pickup.h"
#include "../entities/repair_interaction.h"
#include "../entities/pulley_gate.h"
#include "../entities/dynamic_water.h"
#include "../entities/cut_rope.h"
#include "../entities/rune_upgrade.h"
#include "../entities/boss_door.h"
// include_list insert point

#include "../puzzle/bool_and_logic.h"
#include "../puzzle/camera_focus.h"
#include "../puzzle/electric_ball.h"
#include "../puzzle/electric_ball_grabber.h"
#include "../puzzle/electric_ball_dropper.h"
#include "../puzzle/elevator.h"
#include "../puzzle/timed_torch.h"
#include "../puzzle/step_switch.h"
#include "../puzzle/pottery_wheel.h"
#include "../puzzle/fan_switch.h"

#include "../pickups/mana_plant.h"

#include "fields.h"

#define ENTITY_DEFINITION(name) [ENTITY_TYPE_ ## name] = { \
    #name, \
    (entity_init)name ## _init, \
    (entity_destroy)name ## _destroy, \
    name ## _common_init, \
    name ## _common_destroy, \
    sizeof(struct name), \
    sizeof(struct name ## _definition), \
    fields_ ## name, \
    FIELD_COUNT_ ## name, \
    ENTITY_TYPE_ ## name \
}

static struct entity_definition scene_entity_definitions[ENTITY_TYPE_count] = {
    ENTITY_DEFINITION(empty),
    ENTITY_DEFINITION(biter),
    ENTITY_DEFINITION(collectable),
    ENTITY_DEFINITION(crate),
    ENTITY_DEFINITION(ground_torch),
    ENTITY_DEFINITION(npc),
    ENTITY_DEFINITION(training_dummy),
    ENTITY_DEFINITION(treasure_chest),
    ENTITY_DEFINITION(water_cube),
    ENTITY_DEFINITION(mana_plant),
    ENTITY_DEFINITION(jelly),
    ENTITY_DEFINITION(jelly_king),
    ENTITY_DEFINITION(door),
    ENTITY_DEFINITION(timed_torch_puzzle),
    ENTITY_DEFINITION(elevator),
    ENTITY_DEFINITION(room_portal),
    ENTITY_DEFINITION(burning_thorns),
    ENTITY_DEFINITION(bool_and_logic),
    ENTITY_DEFINITION(camera_focus),
    ENTITY_DEFINITION(sign),
    ENTITY_DEFINITION(electric_ball),
    ENTITY_DEFINITION(electric_ball_grabber),
    ENTITY_DEFINITION(electric_ball_dropper),
    ENTITY_DEFINITION(step_switch),
    ENTITY_DEFINITION(pottery_wheel),
    ENTITY_DEFINITION(fan_switch),
    ENTITY_DEFINITION(trigger_cube),
    ENTITY_DEFINITION(line_mesh),
    ENTITY_DEFINITION(script_runner),
    ENTITY_DEFINITION(golem_enemy),
    ENTITY_DEFINITION(pinwheel),
    ENTITY_DEFINITION(breakable),
    ENTITY_DEFINITION(jelly_pot),
    ENTITY_DEFINITION(comm_stone),
    ENTITY_DEFINITION(repair_scene),
    ENTITY_DEFINITION(repair_part),
    ENTITY_DEFINITION(item_pickup),
    ENTITY_DEFINITION(repair_interaction),
    ENTITY_DEFINITION(pulley_gate),
    ENTITY_DEFINITION(dynamic_water),
    ENTITY_DEFINITION(cut_rope),
    ENTITY_DEFINITION(rune_upgrade),
    ENTITY_DEFINITION(boss_door),
    // scene_entity_definitions insert point
};

static uint16_t scene_entity_count[ENTITY_TYPE_count];

struct entity_definition* entity_find_def(const char* name) {
   for (int i = 0; i < sizeof(scene_entity_definitions) / sizeof(*scene_entity_definitions); i += 1) {
        struct entity_definition* def = &scene_entity_definitions[i];

        if (strcmp(name, def->name) == 0) {
            return def;
        }
   }

   return NULL;
}

struct entity_definition* entity_def_get(unsigned index) {
    if (index >= sizeof(scene_entity_definitions) / sizeof(*scene_entity_definitions)) {
        return NULL;
    }

    return &scene_entity_definitions[index];
}

static struct hash_map entity_mapping;
static entity_id last_despwned_id;

#define ENTITY_STARTING_CAPACITY    32

struct entity_header {
    struct entity_definition* entity_def;
    entity_id id;
};

void entity_add_reference(enum entity_type_id entity_type) {
    if (!scene_entity_count[entity_type]) {
        scene_entity_definitions[entity_type].common_init();
    }
    ++scene_entity_count[entity_type];
}

bool entity_spawn_with_id(enum entity_type_id type, void* definition, enum fixed_entity_ids entity_id) {
    struct entity_definition* entity_def = entity_def_get(type);
    assert(entity_def);

    if (!entity_mapping.entries) {
        if (!hash_map_init(&entity_mapping, ENTITY_STARTING_CAPACITY)) {
            return false;
        }
    }
   
    void* entity = malloc(entity_def->entity_size + sizeof(struct entity_header));
    
    if (!entity || !hash_map_set(&entity_mapping, entity_id, entity)) {
        free(entity);
        return false;
    }

    entity_add_reference(type);

    struct entity_header* header = entity;
    header->entity_def = entity_def;
    header->id = entity_id; 

    entity_def->init(header + 1, definition, entity_id);

    return true;
}

entity_id entity_spawn(enum entity_type_id type, void* definition) {
    entity_id result = entity_id_new();
    if (entity_spawn_with_id(type, definition, result)) {
        return result;
    }
    return 0;
}

bool entity_spawn_singleton(enum entity_type_id type, void* definition, enum fixed_entity_ids entity_id) {
    if (hash_map_get(&entity_mapping, entity_id)) {
        return false;
    }

    return entity_spawn_with_id(type, definition, entity_id);
}

void entity_remove_reference(enum entity_type_id entity_type) {
    --scene_entity_count[entity_type];
    if (!scene_entity_count[entity_type]) {
        scene_entity_definitions[entity_type].common_destroy();
    }
}

bool entity_despawn(entity_id entity_id) {
    if (!entity_id) {
        return false;
    }

    void* entity = hash_map_get(&entity_mapping, entity_id);

    if (!entity) {
        return false;
    }

    // kinda heavy handed but I don't care
    rspq_wait();

    struct entity_header* header = entity;
    entity_remove_reference(header->entity_def->entity_type);

    header->entity_def->destroy(header + 1);
    free(entity);

    hash_map_delete(&entity_mapping, entity_id);
    last_despwned_id = entity_id;

    return true;
}

void entity_despawn_all() {
    for (struct hash_map_entry* entry = hash_map_next(&entity_mapping, NULL); entry; entry = hash_map_next(&entity_mapping, entry)) {
        struct entity_header* header = entry->value;
        header->entity_def->destroy(header + 1);
        free(entry->value);   
    }

    hash_map_destroy(&entity_mapping);
    last_despwned_id = 0;
}

void* entity_get(entity_id entity_id) {
    if (!entity_id) {
        return NULL;
    }

    struct entity_header* header = hash_map_get(&entity_mapping, entity_id);

    if (!header) {
        return NULL;
    }

    return header + 1;
}

entity_id entity_get_last_despawned() {
    return last_despwned_id;
}