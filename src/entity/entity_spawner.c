#include "entity_spawner.h"

#include "../enemies/biter.h"
#include "../enemies/jelly.h"
#include "../enemies/jelly_king.h"
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

#include "../puzzle/timed_torch.h"
#include "../puzzle/elevator.h"
#include "../puzzle/bool_and_logic.h"
#include "../puzzle/camera_focus.h"

#include "../pickups/mana_plant.h"

#define ENTITY_DEFINITION(name, fields) [ENTITY_TYPE_ ## name] = { \
    #name, \
    (entity_init)name ## _init, \
    (entity_destroy)name ## _destroy, \
    sizeof(struct name), \
    sizeof(struct name ## _definition), \
    fields, \
    sizeof(fields) / sizeof(*fields) \
}

static struct entity_field_type_location fields_empty[] = {};
static struct entity_field_type_location fields_npc[] = {
    { .offset = offsetof(struct npc_definition, dialog), .type = ENTITY_FIELD_TYPE_STRING },
};

static struct entity_definition scene_entity_definitions[] = {
    ENTITY_DEFINITION(empty, fields_empty),
    ENTITY_DEFINITION(biter, fields_empty),
    ENTITY_DEFINITION(collectable, fields_empty),
    ENTITY_DEFINITION(crate, fields_empty),
    ENTITY_DEFINITION(ground_torch, fields_empty),
    ENTITY_DEFINITION(npc, fields_npc),
    ENTITY_DEFINITION(training_dummy, fields_empty),
    ENTITY_DEFINITION(treasure_chest, fields_empty),
    ENTITY_DEFINITION(water_cube, fields_empty),
    ENTITY_DEFINITION(mana_plant, fields_empty),
    ENTITY_DEFINITION(jelly, fields_empty),
    ENTITY_DEFINITION(jelly_king, fields_empty),
    ENTITY_DEFINITION(door, fields_empty),
    ENTITY_DEFINITION(timed_torch_puzzle, fields_empty),
    ENTITY_DEFINITION(elevator, fields_empty),
    ENTITY_DEFINITION(room_portal, fields_empty),
    ENTITY_DEFINITION(burning_thorns, fields_empty),
    ENTITY_DEFINITION(bool_and_logic, fields_empty),
    ENTITY_DEFINITION(camera_focus, fields_empty),
};

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

entity_id entity_spawn(enum entity_type_id type, void* definition) {
    struct entity_definition* entity_def = entity_def_get(type);

    if (!entity_def) {
        return 0;
    }

    if (!entity_mapping.entries) {
        if (!hash_map_init(&entity_mapping, ENTITY_STARTING_CAPACITY)) {
            return 0;
        }
    }
   
    entity_id result = entity_id_new();
    void* entity = malloc(entity_def->entity_size + sizeof(struct entity_header));
    
    if (!hash_map_set(&entity_mapping, result, entity)) {
        free(entity);
        return 0;
    }

    struct entity_header* header = entity;
    header->entity_def = entity_def;
    header->id = result; 

    entity_def->init(header + 1, definition, result);
    return result;
}

bool entity_despawn(entity_id entity_id) {
    if (!entity_id) {
        return false;
    }

    void* entity = hash_map_get(&entity_mapping, entity_id);

    if (!entity) {
        return false;
    }

    struct entity_header* header = entity;
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