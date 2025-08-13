#include "room_portal.h"

#include "../render/render_scene.h"

void room_portal_init(struct room_portal* portal, struct room_portal_definition* definition) {
    transformSaInit(&portal->transform, &definition->position, &definition->rotation, definition->scale);

    renderable_single_axis_init(&portal->renderable, &portal->transform, "rom:/meshes/objects/room_portal.tmesh");
    render_scene_add_renderable(&portal->renderable, definition->scale * 1.4f);
    
    portal->room_a = definition->room_a;
    portal->room_b = definition->room_b;

    portal->attrs[0].type = ELEMENT_ATTR_PRIM_COLOR;
    portal->attrs[0].prim.color = (color_t){0, 0, 0, 255};
    portal->attrs[1].type = ELEMENT_ATTR_NONE;

    portal->renderable.attrs = portal->attrs;
}

void room_portal_destroy(struct room_portal* portal) {
    render_scene_remove(&portal->renderable);
    renderable_destroy(&portal->renderable);
}