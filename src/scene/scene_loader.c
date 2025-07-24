#include "scene_loader.h"

#include <malloc.h>
#include <string.h>
#include <libdragon.h>
#include "../resource/mesh_collider.h"
#include "../resource/tmesh_cache.h"
#include "../render/render_scene.h"
#include "../time/time.h"
#include "../cutscene/cutscene_runner.h"
#include "../cutscene/evaluation_context.h"
#include "../cutscene/expression_evaluate.h"
#include "../overworld/overworld_load.h"

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

#include "../pickups/mana_plant.h"

#include "../collision/collision_scene.h"


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
};

// WRLD
#define EXPECTED_HEADER 0x57524C44

struct entity_definition* scene_find_def(const char* name) {
   for (int i = 0; i < sizeof(scene_entity_definitions) / sizeof(*scene_entity_definitions); i += 1) {
        struct entity_definition* def = &scene_entity_definitions[i];

        if (strcmp(name, def->name) == 0) {
            return def;
        }
   }

   return NULL;
}

// EXPR
#define EXPECTED_CONDITION_HEADER 0x45585052

bool scene_load_check_condition(FILE* file) {
    struct expression expression;
    int header;
    fread(&header, 1, 4, file);
    assert(header == EXPECTED_CONDITION_HEADER);
    uint16_t byte_size;
    fread(&byte_size, 1, 2, file);
    char expression_program[byte_size];
    expression.expression_program = expression_program;
    fread(expression.expression_program, 1, byte_size, file);

    struct evaluation_context eval_context;
    evaluation_context_init(&eval_context, 0);

    expression_evaluate(&eval_context, &expression);

    int result = evaluation_context_pop(&eval_context);

    evaluation_context_destroy(&eval_context);

    return result != 0;
}

void scene_load_entity(struct scene* scene, struct entity_data* entity_data, FILE* file) {
    uint16_t entity_type_id;
    fread(&entity_type_id, 2, 1, file);
    struct entity_definition* def = scene_get_entity(entity_type_id);

    assert(def);

    fread(&entity_data->entity_count, 2, 1, file);
    uint16_t definition_size;
    fread(&definition_size, 2, 1, file);
    assert(definition_size == def->definition_size);

    char* entity = malloc(def->entity_size * entity_data->entity_count);
    char entity_def_data[definition_size * entity_data->entity_count];
    char* entity_def = entity_def_data;

    entity_data->definition = def;
    entity_data->entities = entity;

    fread(entity_def_data, definition_size, entity_data->entity_count, file);

    int final_count = 0;

    for (int entity_index = 0; entity_index < entity_data->entity_count; entity_index += 1) {
        if (scene_load_check_condition(file)) {
            scene_entity_apply_types(entity_def, scene->string_table, def->fields, def->field_count);
            def->init(entity, entity_def);
            entity += def->entity_size;
            final_count += 1;
        }

        entity_def += def->definition_size;
    }

    entity_data->entity_count = final_count;
}

void scene_destroy_entity(struct entity_data* entity_data) {
    char* entity = entity_data->entities;

    for (int entity_index = 0; entity_index < entity_data->entity_count; entity_index += 1) {
        entity_data->definition->destroy(entity);
        entity += entity_data->definition->entity_size;
    }

    free(entity_data->entities);
}

void scene_load_camera_animations(struct camera_animation_list* list, const char* filename, FILE* file) {
    uint16_t count;
    fread(&count, sizeof(count), 1, file);
    camera_animation_list_init(list, count, 0);

    int filename_len = strlen(filename);
    char modified_filename[filename_len + 1];
    strcpy(modified_filename, filename + strlen("rom:/"));
    strcpy(modified_filename + filename_len - 10, "sanim");

    list->rom_location = dfs_rom_addr(modified_filename);

    for (int i = 0; i < count; ++i) {
        uint8_t name_length;
        fread(&name_length, 1, 1, file);
        char* name = malloc(name_length + 1);
        fread(name, name_length, 1, file);
        name[name_length] = '\0';

        struct camera_animation* animation = &list->animations[i];

        animation->name = name;
        fread(&animation->frame_count, sizeof(uint16_t), 1, file);
        fread(&animation->rom_offset, sizeof(uint32_t), 1, file);
        animation->rom_offset += list->rom_location;
    }
}

struct scene* scene_load(const char* filename) {
    FILE* file = asset_fopen(filename, NULL);

    int header;
    fread(&header, 1, 4, file);
    assert(header == EXPECTED_HEADER);

    struct scene* scene = malloc(sizeof(struct scene));

    struct player_definition player_def;
    player_def.location = gZeroVec;
    player_def.rotation = gRight2;

    uint8_t location_count;
    fread(&location_count, 1, 1, file);

    bool found_entry = false;

    struct cutscene* starting_cutscene = NULL;

    struct named_location* named_locations = malloc(sizeof(struct named_location) * location_count);
    struct named_location* end = named_locations + location_count;
    
    for (struct named_location* curr = named_locations; curr < end; ++curr) {
        uint8_t name_length;
        fread(&name_length, 1, 1, file);
        curr->name = malloc(name_length + 1);
        fread(curr->name, name_length, 1, file);
        curr->name[name_length] = '\0';

        uint8_t on_enter_length;
        fread(&on_enter_length, 1, 1, file);
        char on_enter[on_enter_length + 1];
        fread(on_enter, on_enter_length, 1, file);
        on_enter[on_enter_length] = '\0';

        fread(&curr->position, sizeof(struct Vector3), 1, file);
        fread(&curr->rotation, sizeof(struct Vector2), 1, file);
        fread(&curr->room_index, sizeof(uint16_t), 1, file);

        if (found_entry) {
            continue;
        }

        if (strcmp(curr->name, "default") == 0) {
            player_def.location = curr->position;
            player_def.rotation = curr->rotation;
        }

        if (strcmp(curr->name, scene_get_next_entry()) == 0) {
            player_def.location = curr->position;
            player_def.rotation = curr->rotation;
            found_entry = true;

            if (on_enter_length) {
                starting_cutscene = cutscene_load(on_enter);
            }
        }
    }

    scene->named_locations = named_locations;
    scene->named_location_count = location_count;

    inventory_init();
    camera_init(&scene->camera, 70.0f, 1.0f, 125.0f);
    player_init(&scene->player, &player_def, &scene->camera.transform);
    camera_controller_init(&scene->camera_controller, &scene->camera, &scene->player);

    pause_menu_init(&scene->pause_menu);
    hud_init(&scene->hud, &scene->player);

    uint16_t static_count;
    fread(&static_count, 2, 1, file);
    scene->static_entity_count = static_count;

    scene->static_entities = malloc(sizeof(struct static_entity) * static_count);

    for (int i = 0; i < static_count; ++i) {
        uint8_t str_len;
        fread(&str_len, 1, 1, file);

        struct static_entity* entity = &scene->static_entities[i];
        tmesh_load(&scene->static_entities[i].tmesh, file);
    }

    uint16_t room_count;
    fread(&room_count, 2, 1, file);
    scene->room_count = room_count;

    scene->room_static_ranges = malloc(sizeof(struct static_entity_range) * room_count);
    fread(scene->room_static_ranges, sizeof(struct static_entity_range), room_count, file);

    mesh_collider_load(&scene->mesh_collider, file);
    collision_scene_add_static_mesh(&scene->mesh_collider);

    uint16_t strings_length;
    fread(&strings_length, 2, 1, file);

    scene->string_table = malloc(strings_length);
    fread(scene->string_table, strings_length, 1, file);

    fread(&scene->entity_data_count, 2, 1, file);

    scene->entity_data = malloc(sizeof(struct entity_data) * scene->entity_data_count);

    for (int i = 0; i < scene->entity_data_count; i += 1) {
        scene_load_entity(scene, &scene->entity_data[i], file);
    }

    fread(&scene->loading_zone_count, 2, 1, file);

    scene->loading_zones = malloc(sizeof(struct loading_zone) * scene->loading_zone_count);
    fread(scene->loading_zones, sizeof(struct loading_zone), scene->loading_zone_count, file);

    for (int i = 0; i < scene->loading_zone_count; i += 1) {
        scene->loading_zones[i].scene_name += (int)scene->string_table;
    }

    uint8_t overworld_filename_length;
    fread(&overworld_filename_length, 1, 1, file);
    if (overworld_filename_length) {
        char overworld_filename[overworld_filename_length + 1];
        fread(overworld_filename, overworld_filename_length, 1, file);
        overworld_filename[overworld_filename_length] = '\0';
        scene->overworld = overworld_load(overworld_filename);
    } else {
        scene->overworld = NULL;
    }

    scene_load_camera_animations(&scene->camera_animations, filename, file);

    fclose(file);

    render_scene_add(NULL, 0.0f, scene_render, scene);
    update_add(scene, scene_update, UPDATE_PRIORITY_CAMERA, UPDATE_LAYER_WORLD);

    if (starting_cutscene) {
        cutscene_runner_run(starting_cutscene, cutscene_runner_free_on_finish(), NULL);
    }

    return scene;
}

void scene_release(struct scene* scene) {
    if (!scene) {
        return;
    }

    for (int i = 0; i < scene->static_entity_count; ++i) {
        struct static_entity* entity = &scene->static_entities[i];
        tmesh_release(&entity->tmesh);
    }
    free(scene->static_entities);

    render_scene_remove(scene);
    update_remove(scene);

    pause_menu_destroy(&scene->pause_menu);
    hud_destroy(&scene->hud);
    player_destroy(&scene->player);
    camera_controller_destroy(&scene->camera_controller);

    inventory_destroy();

    collision_scene_remove_static_mesh(&scene->mesh_collider);
    mesh_collider_release(&scene->mesh_collider);

    for (int i = 0; i < scene->entity_data_count; i += 1) {
        scene_destroy_entity(&scene->entity_data[i]);
    }

    if (scene->overworld) {
        overworld_free(scene->overworld);
    }
    
    free(scene->entity_data);

    free(scene->string_table);
    free(scene->loading_zones);

    for (int i = 0; i < scene->named_location_count; i += 1) {
        free(scene->named_locations[i].name);
    }
    free(scene->named_locations);

    camera_animation_list_destroy(&scene->camera_animations);

    free(scene);
}

struct entity_definition* scene_get_entity(unsigned index) {
    if (index >= sizeof(scene_entity_definitions) / sizeof(*scene_entity_definitions)) {
        return NULL;
    }

    return &scene_entity_definitions[index];
}