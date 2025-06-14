#include "mana_plant.h"

#include "../render/render_scene.h"

void mana_plant_init(struct mana_plant* plant, struct mana_plant_definition* definition) {
    plant->transform.position = definition->position;
    plant->transform.rotation = definition->rotation;

    renderable_single_axis_init(&plant->renderable, &plant->transform, "rom:/meshes/pickups/mana_plant.tmesh");
    render_scene_add_renderable(&plant->renderable, 1.73f);
}

void mana_plant_destroy(struct mana_plant* plant) {
    render_scene_remove(&plant->renderable);
    renderable_destroy(&plant->renderable);
}