#include "camera_focus.h"

#include <memory.h>
#include "../time/time.h"
#include "../cutscene/expression_evaluate.h"
#include "../cutscene/cutscene.h"
#include "../cutscene/cutscene_runner.h"
#include "../scene/scene.h"
#include "../util/hash_map.h"

#define PRE_DELAY   1.0f
#define POST_DELAY  1.0f

static hash_map_t camera_focus_entries;

void camera_focus_update(void* data) {
    camera_focus_t* camera_focus = (camera_focus_t*)data;
    bool state = expression_get_bool(camera_focus->input);
    bool last_state = camera_focus->last_state;
    camera_focus->last_state = state;

    if (!state || last_state) {
        return;
    }

    if (!camera_focus->repeat && camera_focus->did_fire) {
        expression_set_bool(camera_focus->output, true);
        return;
    }

    camera_set_fixed(
        &current_scene->player.camera_controller, 
        &camera_focus->position, 
        &camera_focus->rotation,
        camera_focus->fov
    );
    camera_focus->did_fire = true;
    
    struct cutscene_builder builder;
    cutscene_builder_init(&builder);
    cutscene_builder_pause(&builder, true, false);
    cutscene_builder_delay(&builder, PRE_DELAY);
    cutscene_builder_set_boolean(&builder, camera_focus->output, true);
    cutscene_builder_delay(&builder, POST_DELAY);
    cutscene_builder_pause(&builder, false, false);
    cutscene_builder_camera_return(&builder);

    struct cutscene* cutscene = cutscene_builder_finish(&builder);
    cutscene_runner_run(&(cutscene_run_args_t){
        .cutscene = cutscene,
        .function_index = 0,
        .channel = CUTSCENE_CHANNEL_INTERACTION,
        .finish_callback = cutscene_runner_free_on_finish(),
        .data = NULL,
        .subject = 0,
    });
}

void camera_focus_init(camera_focus_t* camera_focus, struct camera_focus_definition* definition, entity_id entity_id) {
    memcpy(camera_focus, definition, sizeof(struct camera_focus_definition));

    update_add(camera_focus, camera_focus_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);
    camera_focus->last_state = expression_get_bool(camera_focus->input);

    if (camera_focus->last_state) {
        expression_set_bool(camera_focus->output, true);
    }

    camera_focus->did_fire = false;
    camera_focus->id = entity_id;

    hash_map_set(&camera_focus_entries, entity_id, camera_focus);
}

void camera_focus_destroy(camera_focus_t* camera_focus) {
    update_remove(camera_focus);

    hash_map_delete(&camera_focus_entries, camera_focus->id);
}

void camera_focus_common_init() {
    hash_map_init(&camera_focus_entries, 4);
}

void camera_focus_common_destroy() {
    hash_map_destroy(&camera_focus_entries);
}

camera_focus_t* camera_focus_find(entity_id id) {
    return hash_map_get(&camera_focus_entries, id);
}