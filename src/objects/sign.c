#include "sign.h"

#include "../render/render_scene.h"

void sign_interact(struct interactable* interactable, entity_id from) {
    // TODO run cutscene
}

void sign_init(sign_t* sign, struct sign_definition* def, entity_id entity_id) {
    transformSaInit(&sign->transform, &def->position, &def->rotation, 1.0f);

    interactable_init(&sign->interactable, entity_id, sign_interact, sign);

    renderable_single_axis_init(&sign->renderable, &sign->transform, "rom:/meshes/objects/sign.tmesh");
    render_scene_add_renderable(&sign->renderable, 1.0f);
}

void sign_destroy(sign_t* sign) {
    interactable_destroy(&sign->interactable);
    renderable_destroy(&sign->renderable);
    render_scene_remove(&sign->renderable);
}