#include "init.h"

#include "../render/render_scene.h"
#include "../time/time.h"
#include "../collision/collision_scene.h"
#include "../entity/health.h"
#include "../entity/interactable.h"
#include "../menu/menu_rendering.h"
#include "../menu/dialog_box.h"
#include "../cutscene/cutscene_runner.h"
#include "../cutscene/cutscene_actor.h"
#include "../audio/audio.h"
#include "../menu/menu_common.h"
#include "../spell/mana_pool.h"
#include "../objects/assets.h"
#include "../spell/assets.h"
#include "../effects/assets.h"
#include "../player/inventory.h"
#include "../menu/rsp_menu.h"
#include "../render/blur/blur.h"

#include "../resource/animation_cache.h"
#include "../resource/material_cache.h"
#include "../resource/sprite_cache.h"
#include "../resource/tmesh_cache.h"
#include "../resource/wav_cache.h"

void init_engine() {
    animation_cache_init();
    material_cache_init();
    sprite_cache_init();
    tmesh_cache_init();
    wav_cache_init();

    spell_assets_init();
    effect_assets_init();
    object_assets_init();
    menu_common_init();
    render_scene_reset();
    update_reset();
    collision_scene_reset();
    health_reset();
    mana_pool_reset();
    interactable_reset();
    menu_reset();
    dialog_box_init();
    cutscene_runner_init();
    cutscene_actor_reset();
    audio_player_init();
    inventory_init();
    menu_init();
    blur_init();
}