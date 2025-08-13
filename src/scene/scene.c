#include "scene.h"

#include "../render/render_batch.h"
#include "../overworld/overworld_load.h"
#include "../util/memory_stream.h"
#include "../entity/entity_spawner.h"
#include "../cutscene/evaluation_context.h"
#include "../cutscene/expression_evaluate.h"

struct scene* current_scene;

static char next_scene_name[64];
static char next_entrance_name[16];

void scene_render_room(struct scene* scene, int room_index, struct render_batch* batch) {
    if (room_index < 0 || room_index >= scene->room_count) {
        return;
    }

    struct static_entity_range range = scene->room_static_ranges[room_index];

    struct static_entity* curr = scene->static_entities + range.start;
    struct static_entity* end = scene->static_entities + range.end;

    for (; curr < end; ++curr) {
        render_batch_add_tmesh(batch, &curr->tmesh, NULL, 0, NULL, NULL, NULL);
    }
}

void scene_render(void* data, struct render_batch* batch) {
    struct scene* scene = (struct scene*)data;
    
    for (int i = 0; i < MAX_LOADED_ROOM; i += 1) {
        if (scene->loaded_rooms[i].room_index == ROOM_INDEX_NONE) {
            continue;
        }

        scene_render_room(scene, scene->loaded_rooms[i].room_index, batch);
    }
}

void scene_check_despawns(struct scene* scene) {
    entity_id last_despawn = entity_get_last_despawned();

    if (last_despawn == scene->last_despawn_check) {
        return;
    }

    scene->last_despawn_check = last_despawn;

    for (int room_index = 0; room_index < MAX_LOADED_ROOM; room_index += 1) {
        loaded_room_t* room = &scene->loaded_rooms[room_index];

        if (room->room_index == ROOM_INDEX_NONE) {
            continue;
        }

        for (int entity_index = 0; entity_index < room->entity_count; entity_index += 1) {
            loaded_entity_t* entity = &room->entities[entity_index];

            if (!entity->id || entity_get(entity->id)) {
                continue;
            }

            entity->id = 0;
            expression_set_bool(entity->on_despawn, true);
        }
    }
}

void scene_update(void* data) {
    struct scene* scene = (struct scene*)data;

    struct Vector3 player_center = scene->player.cutscene_actor.transform.position;
    player_center.y += scene->player.cutscene_actor.collider.center.y;

    for (int i = 0; i < scene->loading_zone_count; i += 1) {
        if (box3DContainsPoint(&scene->loading_zones[i].bounding_box, &player_center)) {
            scene_queue_next(scene->loading_zones[i].scene_name);
        }
    }

    if (scene->overworld) {
        overworld_check_loaded_tiles(scene->overworld);
        overworld_check_collider_tiles(scene->overworld, player_get_position(&scene->player));
    }

    scene_check_despawns(scene);
}

void scene_queue_next(char* scene_name) {
    char* curr = scene_name;
    char* out = next_scene_name;
    while (*curr && *curr != '#') {
        *out++ = *curr++;
    }

    *out = '\0';

    out = next_entrance_name;

    if (!*curr) {
        next_entrance_name[0] = '\0';
        return;
    }

    // skip the bound symbol
    ++curr;

    while ((*out++ = *curr++));
}

void scene_clear_next() {
    next_scene_name[0] = '\0';
}

loaded_entity_t scene_load_entity(struct scene* scene, memory_stream_t* stream, evaluation_context_t* eval_context) {
    uint32_t expression_header;
    memory_stream_read(stream, &expression_header, sizeof(uint32_t));
    assert(expression_header == EXPECTED_EXPR_HEADER);
    uint16_t expression_size;
    memory_stream_read(stream, &expression_size, sizeof(uint16_t));
    char expresion_program[expression_size];
    memory_stream_read(stream, expresion_program, expression_size);
    struct expression expression;
    expression.expression_program = expresion_program;

    expression_evaluate(eval_context, &expression);

    int should_spawn = evaluation_context_pop(eval_context);

    uint16_t on_despawn;
    memory_stream_read(stream, &on_despawn, sizeof(uint16_t));

    uint16_t entity_type;
    uint16_t def_size;
    memory_stream_read(stream, &entity_type, sizeof(uint16_t));
    memory_stream_read(stream, &def_size, sizeof(uint16_t));
    struct entity_definition* def = entity_def_get(entity_type);
    assert(def->definition_size == def_size);

    if (should_spawn) {
        char def_data[def_size] __attribute__((aligned(8)));
        memory_stream_read(stream, def_data, def_size);
        // maybe todo, the types could be applied once on scene load
        // instead of on each time the entity is loaded
        scene_entity_apply_types(def, scene->string_table, def->fields, def->field_count);
        return (loaded_entity_t){
            .id = entity_spawn(entity_type, def_data),
            .on_despawn = on_despawn,
        };
    } else {
        memory_stream_read(stream, NULL, def_size);
        return (loaded_entity_t){
            .id = 0,
            .on_despawn = VARIABLE_DISCONNECTED,
        };
    }
}

void scene_remove_shared_reference(struct scene* scene, int entity_index) {
    assert(entity_index >= 0 && entity_index < scene->shared_entities.shared_entity_count);

    shared_room_entity_t* entity = &scene->shared_entities.entities[entity_index];
    --entity->ref_count; 

    if (entity->ref_count > 0) {
        return;
    }

    entity_despawn(entity->entity_id);
    entity->entity_id = 0;
}

void scene_add_shared_reference(struct scene* scene, int entity_index, evaluation_context_t* eval_context) {
    assert(entity_index >= 0 && entity_index < scene->shared_entities.shared_entity_count);

    shared_room_entity_t* entity = &scene->shared_entities.entities[entity_index];
    ++entity->ref_count;

    if (entity->ref_count > 1) {
        return;
    }

    memory_stream_t stream;
    memory_stream_init(&stream, entity->block, entity->block_size);
    entity->entity_id = scene_load_entity(scene, &stream, eval_context).id;
}

void scene_load_room(struct scene* scene, loaded_room_t* room, int room_index) {
    room_entity_block_t* room_source = &scene->room_entities[room_index];

    if (room_source->block == NULL) {
        room->entity_count = 0;
        room->entities = NULL;
        return;
    }

    memory_stream_t stream;
    memory_stream_init(&stream, room_source->block, room_source->block_size);

    uint16_t entity_count;
    memory_stream_read(&stream, &entity_count, sizeof(entity_count));

    room->entity_count = entity_count;
    room->entities = entity_count ? malloc(sizeof(loaded_entity_t) * entity_count) : NULL;

    struct evaluation_context eval_context;
    evaluation_context_init(&eval_context, 0); 

    for (int i = 0; i < entity_count; i += 1) {
        room->entities[i] = scene_load_entity(scene, &stream, &eval_context);
    }

    for (int i = 0; i < room_source->shared_entity_count; i += 1) {
        scene_add_shared_reference(scene, room_source->shared_entity_index[i], &eval_context);
    }

    evaluation_context_destroy(&eval_context);
}

bool scene_show_room(struct scene* scene, int room_index) {
    if (scene_is_showing_room(scene, room_index)) {
        return true;
    }

    for (int i = 0; i < MAX_LOADED_ROOM; i += 1) {
        loaded_room_t* room = &scene->loaded_rooms[i];

        if (room->room_index == ROOM_INDEX_NONE) {
            room->room_index = room_index;
            scene_load_room(scene, room, room_index);
            return true;
        }
    }

    return false;
}

void scene_hide_room(struct scene* scene, int room_index) {
    for (int i = 0; i < MAX_LOADED_ROOM; i += 1) {
        loaded_room_t* room = &scene->loaded_rooms[i];

        if (room->room_index == room_index) {
            for (int entity_index = 0; entity_index < room->entity_count; entity_index += 1) {
                entity_despawn(room->entities[entity_index].id);
            }
            free(room->entities);
            room->entities = NULL;
            room->room_index = ROOM_INDEX_NONE;


            room_entity_block_t* room_source = &scene->room_entities[room_index];
            for (int i = 0; i < room_source->shared_entity_count; i += 1) {
                scene_remove_shared_reference(scene, room_source->shared_entity_index[i]);
            }
            break;
        }
    }
}


bool scene_is_showing_room(struct scene* scene, int room_index) {
    for (int i = 0; i < MAX_LOADED_ROOM; i += 1) {
        loaded_room_t* room = &scene->loaded_rooms[i];

        if (room->room_index == room_index) {
            return true;
        }
    }

    return false;
}

bool scene_has_next() {
    return next_scene_name[0] != 0;
}

char* scene_get_next() {
    return next_scene_name;
}

char* scene_get_next_entry() {
    return next_entrance_name;
}

struct named_location* scene_find_location(char* name) {
    for (int i = 0; i < current_scene->named_location_count; i += 1) {
        if (strcmp(name, current_scene->named_locations[i].name) == 0) {
            return &current_scene->named_locations[i];
        }
    }

    return NULL;
}

void scene_entity_apply_types(void* definition, char* string_table, struct entity_field_type_location* type_locations, int type_location_count) {
    for (int i = 0; i < type_location_count; i += 1) {
        switch (type_locations[i].type) {
            case ENTITY_FIELD_TYPE_STRING: {
                char** entry_location = (char**)((char*)definition + type_locations[i].offset);
                *entry_location += (int)string_table;
                break;
            }
        }
    }
}