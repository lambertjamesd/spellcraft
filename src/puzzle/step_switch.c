#include "step_switch.h"

#include "../collision/collision_scene.h"
#include "../render/render_scene.h"
#include "../collision/shapes/box.h"
#include "../time/time.h"
#include "../cutscene/expression_evaluate.h"

static struct dynamic_object_type step_switch_type = {
    BOX_COLLIDER(0.5f, 0.1f, 0.5f),
};

void step_switch_update(void* data) {
    step_switch_t* step_switch = (step_switch_t*)data;

    bool is_stepping = false;

    for (contact_t* curr = step_switch->collider.active_contacts; curr; curr = curr->next) {
        if (curr->normal.y < -0.707f) {
            is_stepping =  true;
        }
    }

    expression_set_bool(step_switch->output, is_stepping);
}

void step_switch_init(step_switch_t* step_switch, struct step_switch_definition* definition, entity_id entity_id) {
    transformSaInit(&step_switch->transform, &definition->position, &gRight2, 1.0f);

    renderable_single_axis_init(&step_switch->renderable, &step_switch->transform, "rom:/meshes/objects/step_switch.tmesh");
    render_scene_add_renderable(&step_switch->renderable, 0.5f);

    dynamic_object_init(
        entity_id,
        &step_switch->collider,
        &step_switch_type,
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE,
        &step_switch->transform.position,
        NULL
    );

    step_switch->collider.center.y = 0.1f,
    step_switch->collider.is_fixed = 1;
    step_switch->collider.weight_class = 2;
    step_switch->output = definition->output;

    collision_scene_add(&step_switch->collider);

    update_add(step_switch, step_switch_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);
}

void step_switch_destroy(step_switch_t* step_switch) {
    collision_scene_remove(&step_switch->collider);
    render_scene_remove(&step_switch->renderable);
    renderable_destroy(&step_switch->renderable);
    update_remove(step_switch);
}