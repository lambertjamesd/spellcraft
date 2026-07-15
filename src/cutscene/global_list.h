#ifndef __CUTSCENE_GLOBAL_LIST_H__
#define __CUTSCENE_GLOBAL_LIST_H__

#include "evaluation_context.h"

#define VAR_POS_fire_rune_has_level_0 0
#define VAR_TYP_fire_rune_has_level_0 DATA_TYPE_BOOL
#define VAR_LOC_fire_rune_has_level_0 (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 0}

#define VAR_POS_fire_rune_has_level_1 1
#define VAR_TYP_fire_rune_has_level_1 DATA_TYPE_BOOL
#define VAR_LOC_fire_rune_has_level_1 (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 1}

#define VAR_POS_fire_rune_has_level_2 2
#define VAR_TYP_fire_rune_has_level_2 DATA_TYPE_BOOL
#define VAR_LOC_fire_rune_has_level_2 (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 2}

#define VAR_POS_fire_rune_has_level_3 3
#define VAR_TYP_fire_rune_has_level_3 DATA_TYPE_BOOL
#define VAR_LOC_fire_rune_has_level_3 (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 3}

#define VAR_POS_fire_trials_boss_dead 4
#define VAR_TYP_fire_trials_boss_dead DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_boss_dead (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 4}

#define VAR_POS_fire_trials_bossdoor_kiln_0 5
#define VAR_TYP_fire_trials_bossdoor_kiln_0 DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_bossdoor_kiln_0 (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 5}

#define VAR_POS_fire_trials_bossdoor_kiln_1 6
#define VAR_TYP_fire_trials_bossdoor_kiln_1 DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_bossdoor_kiln_1 (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 6}

#define VAR_POS_fire_trials_bossdoor_open 7
#define VAR_TYP_fire_trials_bossdoor_open DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_bossdoor_open (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 7}

#define VAR_POS_fire_trials_key_0 8
#define VAR_TYP_fire_trials_key_0 DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_key_0 (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 8}

#define VAR_POS_fire_trials_key_1 9
#define VAR_TYP_fire_trials_key_1 DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_key_1 (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 9}

#define VAR_POS_fire_trials_key_2 10
#define VAR_TYP_fire_trials_key_2 DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_key_2 (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 10}

#define VAR_POS_fire_trials_key_door_unlocked 11
#define VAR_TYP_fire_trials_key_door_unlocked DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_key_door_unlocked (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 11}

#define VAR_POS_fire_trials_room_block_puzzle_visited 12
#define VAR_TYP_fire_trials_room_block_puzzle_visited DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_room_block_puzzle_visited (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 12}

#define VAR_POS_fire_trials_room_bossdoor_visited 13
#define VAR_TYP_fire_trials_room_bossdoor_visited DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_room_bossdoor_visited (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 13}

#define VAR_POS_fire_trials_room_bossfight_visited 14
#define VAR_TYP_fire_trials_room_bossfight_visited DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_room_bossfight_visited (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 14}

#define VAR_POS_fire_trials_room_box_visited 15
#define VAR_TYP_fire_trials_room_box_visited DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_room_box_visited (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 15}

#define VAR_POS_fire_trials_room_combat_interlude_visited 16
#define VAR_TYP_fire_trials_room_combat_interlude_visited DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_room_combat_interlude_visited (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 16}

#define VAR_POS_fire_trials_room_crossing_visited 17
#define VAR_TYP_fire_trials_room_crossing_visited DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_room_crossing_visited (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 17}

#define VAR_POS_fire_trials_room_default_visited 18
#define VAR_TYP_fire_trials_room_default_visited DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_room_default_visited (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 18}

#define VAR_POS_fire_trials_room_elevator_visited 19
#define VAR_TYP_fire_trials_room_elevator_visited DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_room_elevator_visited (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 19}

#define VAR_POS_fire_trials_room_entry_visited 20
#define VAR_TYP_fire_trials_room_entry_visited DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_room_entry_visited (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 20}

#define VAR_POS_fire_trials_room_fire_spin_visited 21
#define VAR_TYP_fire_trials_room_fire_spin_visited DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_room_fire_spin_visited (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 21}

#define VAR_POS_fire_trials_room_hallway_to_crossing_visited 22
#define VAR_TYP_fire_trials_room_hallway_to_crossing_visited DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_room_hallway_to_crossing_visited (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 22}

#define VAR_POS_fire_trials_room_hallway_to_rune_visited 23
#define VAR_TYP_fire_trials_room_hallway_to_rune_visited DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_room_hallway_to_rune_visited (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 23}

#define VAR_POS_fire_trials_room_rune_visited 24
#define VAR_TYP_fire_trials_room_rune_visited DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_room_rune_visited (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 24}

#define VAR_POS_fire_trials_room_vinewalk_visited 25
#define VAR_TYP_fire_trials_room_vinewalk_visited DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_room_vinewalk_visited (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 25}

#define VAR_POS_fire_trials_room_wheel_hallway_visited 26
#define VAR_TYP_fire_trials_room_wheel_hallway_visited DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_room_wheel_hallway_visited (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 26}

#define VAR_POS_fire_trials_room_wheels_visited 27
#define VAR_TYP_fire_trials_room_wheels_visited DATA_TYPE_BOOL
#define VAR_LOC_fire_trials_room_wheels_visited (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 27}

#define VAR_POS_has_entered_fire_trials 28
#define VAR_TYP_has_entered_fire_trials DATA_TYPE_BOOL
#define VAR_LOC_has_entered_fire_trials (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 28}

#define VAR_POS_has_staff_default 29
#define VAR_TYP_has_staff_default DATA_TYPE_BOOL
#define VAR_LOC_has_staff_default (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 29}

#define VAR_POS_has_talked_outside 30
#define VAR_TYP_has_talked_outside DATA_TYPE_BOOL
#define VAR_LOC_has_talked_outside (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 30}

#define VAR_POS_intro_played 31
#define VAR_TYP_intro_played DATA_TYPE_BOOL
#define VAR_LOC_intro_played (global_location_t){.data_type = DATA_TYPE_BOOL, .word_offset = 31}

#define VAR_POS_air_rune_level 4
#define VAR_TYP_air_rune_level DATA_TYPE_S8
#define VAR_LOC_air_rune_level (global_location_t){.data_type = DATA_TYPE_S8, .word_offset = 4}

#define VAR_POS_earth_rune_level 5
#define VAR_TYP_earth_rune_level DATA_TYPE_S8
#define VAR_LOC_earth_rune_level (global_location_t){.data_type = DATA_TYPE_S8, .word_offset = 5}

#define VAR_POS_fire_rune_level 6
#define VAR_TYP_fire_rune_level DATA_TYPE_S8
#define VAR_LOC_fire_rune_level (global_location_t){.data_type = DATA_TYPE_S8, .word_offset = 6}

#define VAR_POS_fire_trials_key_count 7
#define VAR_TYP_fire_trials_key_count DATA_TYPE_S8
#define VAR_LOC_fire_trials_key_count (global_location_t){.data_type = DATA_TYPE_S8, .word_offset = 7}

#define VAR_POS_ice_rune_level 8
#define VAR_TYP_ice_rune_level DATA_TYPE_S8
#define VAR_LOC_ice_rune_level (global_location_t){.data_type = DATA_TYPE_S8, .word_offset = 8}

#define VAR_POS_life_rune_level 9
#define VAR_TYP_life_rune_level DATA_TYPE_S8
#define VAR_LOC_life_rune_level (global_location_t){.data_type = DATA_TYPE_S8, .word_offset = 9}

#define VAR_POS_player_name 1
#define VAR_TYP_player_name DATA_TYPE_ADDRESS
#define VAR_LOC_player_name (global_location_t){.data_type = DATA_TYPE_ADDRESS, .word_offset = 1}

#define VAR_POS_recast_rune_level 18
#define VAR_TYP_recast_rune_level DATA_TYPE_S8
#define VAR_LOC_recast_rune_level (global_location_t){.data_type = DATA_TYPE_S8, .word_offset = 18}

#define VAR_POS_talk_count 10
#define VAR_TYP_talk_count DATA_TYPE_S16
#define VAR_LOC_talk_count (global_location_t){.data_type = DATA_TYPE_S16, .word_offset = 10}


#endif
