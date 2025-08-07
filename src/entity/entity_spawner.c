#include "entity_spawner.h"

#include "../enemies/biter.h"
#include "../enemies/jelly.h"
#include "../enemies/jelly_king.h"

#include "../npc/npc.h"

#include "../objects/collectable.h"
#include "../objects/crate.h"
#include "../objects/door.h"
#include "../objects/ground_torch.h"
#include "../objects/training_dummy.h"
#include "../objects/treasure_chest.h"
#include "../objects/empty.h"
#include "../objects/water_cube.h"

#include "../puzzle/timed_torch.h"
#include "../puzzle/elevator.h"

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