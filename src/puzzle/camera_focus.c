#include "camera_focus.h"

#include <memory.h>
#include "../time/time.h"
#include "../cutscene/expression_evaluate.h"
#include "../cutscene/cutscene.h"
#include "../cutscene/cutscene_runner.h"
#include "../scene/scene.h"

#define PRE_DELAY   1.0f
#define POST_DELAY  1.0f

void camera_focus_update(void* data) {
    camera_focus_t* camera_focus = (camera_focus_t*)data;
    bool state = expression_get_bool(camera_focus->input);
    bool last_state = camera_focus->last_state;
    camera_focus->last_state = state;

    if (!state || last_state) {
        return;
    }

    if (camera_focus->repeat && camera_focus->did_fire) {
        expression_set_bool(camera_focus->output, true);
        return;
    }

    camera_set_fixed(
        &current_scene->camera_controller, 
        &camera_focus->position, 
        &camera_focus->rotation,
        camera_focus->fov
    );
    camera_focus->did_fire = true;
    
    struct cutscene_builder builder;
    cutscene_builder_init(&builder);
    cutscene_builder_pause(&builder, true, false, UPDATE_LAYER_WORLD);
    cutscene_builder_delay(&builder, PRE_DELAY);
    cutscene_builder_set_boolean(&builder, camera_focus->output, true);
    cutscene_builder_delay(&builder, POST_DELAY);
    cutscene_builder_pause(&builder, false, false, UPDATE_LAYER_WORLD);
    cutscene_builder_camera_return(&builder);

    struct cutscene* cutscene = cutscene_builder_finish(&builder);
    cutscene_runner_run(cutscene, 0, cutscene_runner_free_on_finish(), NULL, 0);
}

void camera_focus_init(camera_focus_t* camera_focus, struct camera_focus_definition* definition, entity_id entity_id) {
    memcpy(camera_focus, definition, sizeof(struct camera_focus_definition));

    update_add(camera_focus, camera_focus_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);
    camera_focus->last_state = true;
    camera_focus->did_fire = false;
}

void camera_focus_destroy(camera_focus_t* camera_focus) {
    update_remove(camera_focus);
}