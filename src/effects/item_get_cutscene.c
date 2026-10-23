#include "item_get_cutscene.h"

#include "../cutscene/cutscene_runner.h"
#include "../player/player.h"

static vector3_t relative_player_pos = {
    0.393962f,
    1.16192f,
    0.481766f,
};

void item_get_float_above_hand(void* data) {
    renderable_t* renderable = (renderable_t*)data;

    renderable->hide = false;

    transform_sa_t* transform = renderable->transform.transform;
    transformSaTransformPoint(&player_get()->cutscene_actor.transform, &relative_player_pos, &transform->position);
}

void item_get_cutscene_run(cutscene_builder_t* builder, const char* message, renderable_t* renderable, item_get_finish on_finish, void* data) {   
    renderable->hide = true;
    cutscene_builder_npc_animate(builder, ENTITY_ID_PLAYER, "item_get", false);
    cutscene_builder_camera_animate(builder, "item_get", ENTITY_ID_PLAYER, 0);

    cutscene_builder_delay(builder, 1.0f);
    cutscene_builder_callback(builder, item_get_float_above_hand, renderable);

    cutscene_builder_camera_wait(builder);
    cutscene_builder_npc_animate(builder, ENTITY_ID_PLAYER, "item_get_loop", true);
    cutscene_builder_dialog(builder, message);
    cutscene_builder_npc_animate(builder, ENTITY_ID_PLAYER, "item_get_store", false);
    cutscene_builder_callback(builder, on_finish, data);
    cutscene_builder_camera_return(builder);
}