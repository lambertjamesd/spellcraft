#include "fields.h"
#include <stddef.h>

struct entity_field_type_location fields_empty[] = {};
struct entity_field_type_location fields_crate[] = {};
struct entity_field_type_location fields_collectable[] = {};
struct entity_field_type_location fields_training_dummy[] = {};
struct entity_field_type_location fields_treasure_chest[] = {};
struct entity_field_type_location fields_biter[] = {};
struct entity_field_type_location fields_ground_torch[] = {};
struct entity_field_type_location fields_water_cube[] = {};
struct entity_field_type_location fields_dynamic_water[] = {
    { .offset = offsetof(struct dynamic_water_definition, mesh), .type = ENTITY_FIELD_TYPE_STRING },
    { .offset = offsetof(struct dynamic_water_definition, mesh_lod1), .type = ENTITY_FIELD_TYPE_STRING },
};
struct entity_field_type_location fields_mana_plant[] = {};
struct entity_field_type_location fields_jelly[] = {};
struct entity_field_type_location fields_jelly_king[] = {};
struct entity_field_type_location fields_door[] = {};
struct entity_field_type_location fields_room_portal[] = {
    { .offset = offsetof(struct room_portal_definition, mesh), .type = ENTITY_FIELD_TYPE_STRING },
};
struct entity_field_type_location fields_burning_thorns[] = {
    { .offset = offsetof(struct burning_thorns_definition, mesh), .type = ENTITY_FIELD_TYPE_STRING },
};
struct entity_field_type_location fields_timed_torch_puzzle[] = {};
struct entity_field_type_location fields_elevator[] = {};
struct entity_field_type_location fields_bool_and_logic[] = {};
struct entity_field_type_location fields_camera_focus[] = {};
struct entity_field_type_location fields_sign[] = {
    { .offset = offsetof(struct sign_definition, message), .type = ENTITY_FIELD_TYPE_STRING },
};
struct entity_field_type_location fields_electric_ball[] = {};
struct entity_field_type_location fields_electric_ball_grabber[] = {};
struct entity_field_type_location fields_electric_ball_dropper[] = {};
struct entity_field_type_location fields_step_switch[] = {};
struct entity_field_type_location fields_npc[] = {
    { .offset = offsetof(struct npc_definition, dialog), .type = ENTITY_FIELD_TYPE_STRING },
};
struct entity_field_type_location fields_script_runner[] = {
    { .offset = offsetof(struct script_runner_definition, target), .type = ENTITY_FIELD_TYPE_STRING },
};
struct entity_field_type_location fields_pottery_wheel[] = {};
struct entity_field_type_location fields_fan_switch[] = {};
struct entity_field_type_location fields_trigger_cube[] = {};
struct entity_field_type_location fields_line_mesh[] = {
    { .offset = offsetof(struct line_mesh_definition, mesh), .type = ENTITY_FIELD_TYPE_LINE_MESH },
};
struct entity_field_type_location fields_golem_enemy[] = {};
struct entity_field_type_location fields_pinwheel[] = {};
struct entity_field_type_location fields_breakable[] = {};
struct entity_field_type_location fields_jelly_pot[] = {};
struct entity_field_type_location fields_comm_stone[] = {};
struct entity_field_type_location fields_repair_scene[] = {
    { .offset = offsetof(struct repair_scene_definition, exit_scene), .type = ENTITY_FIELD_TYPE_STRING },
};
struct entity_field_type_location fields_repair_part[] = {
    { .offset = offsetof(struct repair_part_definition, mesh), .type = ENTITY_FIELD_TYPE_STRING },
};
struct entity_field_type_location fields_item_pickup[] = {
    { .offset = offsetof(struct item_pickup_definition, mesh), .type = ENTITY_FIELD_TYPE_STRING },
};
struct entity_field_type_location fields_repair_interaction[] = {
    { .offset = offsetof(struct repair_interaction_definition, broken_mesh), .type = ENTITY_FIELD_TYPE_STRING },
    { .offset = offsetof(struct repair_interaction_definition, repaired_mesh), .type = ENTITY_FIELD_TYPE_STRING },
    { .offset = offsetof(struct repair_interaction_definition, on_interact), .type = ENTITY_FIELD_TYPE_STRING },
};
struct entity_field_type_location fields_pulley_gate[] = {};
struct entity_field_type_location fields_cut_rope[] = {};
struct entity_field_type_location fields_rune_upgrade[] = {
    { .offset = offsetof(struct rune_upgrade_definition, mesh), .type = ENTITY_FIELD_TYPE_STRING },
};
struct entity_field_type_location fields_boss_door[] = {
    { .offset = offsetof(struct boss_door_definition, on_interact), .type = ENTITY_FIELD_TYPE_STRING },
};
