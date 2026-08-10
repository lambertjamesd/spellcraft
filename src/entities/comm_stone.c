#include "comm_stone.h"    

#include "../math/mathf.h"

#define LERP_TIME   1.0f

void comm_stone_hide(comm_stone_t* stone) {
    if (stone->is_visible) {
        render_scene_remove(&stone->renderable);
        update_remove(stone);
        stone->is_visible = false;
    }
}

void comm_stone_update_pos(comm_stone_t* stone) {
    vector3Lerp(&stone->from, &stone->target, stone->activation_lerp, &stone->transform.position);
}

void comm_stone_update(void* data) {
    comm_stone_t* stone = (comm_stone_t*)data;

    float target_lerp = stone->is_active ? 1.0f : 0.0f;

    stone->activation_lerp = mathfMoveTowards(stone->activation_lerp, target_lerp, fixed_time_step * (1.0f / LERP_TIME));

    if (stone->is_visible) {
        comm_stone_update_pos(stone);

        if (stone->activation_lerp == 0.0f) {
            comm_stone_hide(stone);
        }
    }
}

void comm_stone_deactivate(comm_stone_t* stone) {
    stone->is_active = false;
}

bool comm_stone_is_animating(comm_stone_t* stone) {
    return stone->is_visible;
}

void comm_stone_activate(comm_stone_t* stone, vector3_t* from, vector3_t* target) {
    if (stone->is_active) {
        return;
    }

    if (!stone->is_visible) {
        stone->is_visible = true;
        render_scene_add_renderable(&stone->renderable, 0.5f);
        update_add(stone, comm_stone_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_CUTSCENE | UPDATE_LAYER_WORLD);
    }

    stone->from = *from;
    stone->target = *target;

    comm_stone_update_pos(stone);
}
    
void comm_stone_init(comm_stone_t* comm_stone, struct comm_stone_definition* definition, entity_id entity_id) {
    transformInit(&comm_stone->transform, &definition->position, &gQuaternionIdent, &gOneVec);
    renderable_init(&comm_stone->renderable, &comm_stone->transform, "rom:/meshes/player/comm_stone.tmesh");
    comm_stone->activation_lerp = 0.0f;
}

void comm_stone_destroy(comm_stone_t* comm_stone, struct comm_stone_definition* definition) {
    comm_stone_hide(comm_stone);
    renderable_destroy(&comm_stone->renderable);
}

void comm_stone_common_init() {

}

void comm_stone_common_destroy() {

}
