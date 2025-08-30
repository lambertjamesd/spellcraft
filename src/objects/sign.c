#include "sign.h"

#include "../render/render_scene.h"
#include "../cutscene/cutscene_runner.h"
#include "../collision/shapes/box.h"
#include "../collision/collision_scene.h"

static struct dynamic_object_type sign_collider = {
    BOX_COLLIDER(0.5f, 0.25f, 0.05f),
};

void sign_interact(struct interactable* interactable, entity_id from) {
    sign_t* sign = (sign_t*)interactable->data;
    if (sign->read_cutscene) {
        cutscene_runner_run(sign->read_cutscene, NULL, NULL, sign->dynamic_object.entity_id);
    }
}

void sign_init(sign_t* sign, struct sign_definition* def, entity_id entity_id) {
    transformSaInit(&sign->transform, &def->position, &def->rotation, 1.0f);

    interactable_init(&sign->interactable, entity_id, sign_interact, sign);

    renderable_single_axis_init(&sign->renderable, &sign->transform, "rom:/meshes/objects/sign.tmesh");
    render_scene_add_renderable(&sign->renderable, 1.0f);

    dynamic_object_init(
        entity_id,
        &sign->dynamic_object,
        &sign_collider,
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE | COLLISION_LAYER_Z_TARGET,
        &sign->transform.position,
        &sign->transform.rotation
    );

    sign->dynamic_object.center.y = 1.0f;
    sign->dynamic_object.is_fixed = 1;
    sign->dynamic_object.weight_class = 3;

    collision_scene_add(&sign->dynamic_object);

    if (def->message) {
        sign->read_cutscene = cutscene_load(def->message);
    } else {
        sign->read_cutscene = NULL;
    }
}

void sign_destroy(sign_t* sign) {
    interactable_destroy(&sign->interactable);
    renderable_destroy(&sign->renderable);
    render_scene_remove(&sign->renderable);
    collision_scene_remove(&sign->dynamic_object);
    cutscene_free(sign->read_cutscene);
}