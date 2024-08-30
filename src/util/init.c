#include "init.h"

#include "../spell/assets.h"
#include "../menu/menu_common.h"
#include "../render/render_scene.h"
#include "../time/time.h"
#include "../collision/collision_scene.h"
#include "../entity/health.h"
#include "../entity/interactable.h"
#include "../menu/menu_rendering.h"
#include "../objects/collectable.h"
#include "../menu/dialog_box.h"
#include "../cutscene/cutscene_runner.h"
#include "../cutscene/cutscene_actor.h"

void init_engine() {
    spell_assets_init();
    menu_common_init();
    render_scene_reset();
    update_reset();
    collision_scene_reset();
    health_reset();
    interactable_reset();
    menu_reset();
    collectable_assets_load();
    dialog_box_init();
    cutscene_runner_init();
    cutscene_actor_reset();
}