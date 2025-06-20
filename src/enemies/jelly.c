#include "jelly.h"

#include "../render/render_scene.h"

void jelly_init(struct jelly* jelly, struct jelly_definition* definition) {
    jelly->transform.position = definition->position;
    jelly->transform.rotation = definition->rotation;

    renderable_single_axis_init(&jelly->renderable, &jelly->transform, "rom:/meshes/enemies/water_jelly.tmesh");
    render_scene_add_renderable(&jelly->renderable, 1.0f);
}

void jelly_destroy(struct jelly* jelly) {
    renderable_destroy(&jelly->renderable);
    render_scene_remove(&jelly->renderable);
}