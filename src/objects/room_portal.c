
#include "room_portal.h"

#include <stdio.h>
#include "../render/render_scene.h"
#include "../scene/scene.h"
#include "../time/time.h"
#include "../math/mathf.h"

#define MAX_DISTANCE    3.0f
#define FADE_DISTANCE   2.0f

#define room_portal_other_room(portal, room)    ((portal)->room_a == (room) ? (portal)->room_b : (portal)->room_a)

struct Vector3 room_portal_distance(struct room_portal* portal, struct Vector3* from) {
    struct Vector3 offset;
    vector3Sub(
        from, 
        &portal->transform.position, 
        &offset
    );

    struct Vector3 local_offset;
    vector3RotateWith2Inv(&offset, &portal->transform.rotation, &local_offset);

    return local_offset;
}

void room_portal_update(void* data) {
    struct room_portal* portal = (struct room_portal*)data;

    struct Vector3 local_offset = room_portal_distance(portal, &current_scene->player.cutscene_actor.transform.position);
    float player_distance = local_offset.z;
    float camera_distance = room_portal_distance(portal, &current_scene->camera.transform.position).z;

    
    int side_a = player_distance > 0.0f;
    int last_side_a = portal->last_player_distance > 0.0f;
    
    float distance = minf(fabsf(player_distance), fabsf(camera_distance));

    if (player_distance * camera_distance < 0.0f) {
        distance = 0.0f;
    }

    int should_fade = distance < MAX_DISTANCE;

    if (should_fade) {
        float alpha = (distance - FADE_DISTANCE) * (1.0f / (MAX_DISTANCE - FADE_DISTANCE));
        if (alpha < 0.0f) {
            alpha = 0.0f;
        }
        portal->attrs[0].color = (color_t){0, 0, 0, (uint8_t)(255.0f * alpha)};
    }

    if (!portal->did_fade && should_fade) {
        if (scene_is_showing_room(current_scene, portal->room_a)) {
            scene_show_room(current_scene, portal->room_b);
            portal->current_room = portal->room_a;
        } else {
            scene_show_room(current_scene, portal->room_a);
            portal->current_room = portal->room_b;
        }
    } else if (portal->did_fade && !should_fade) {
        scene_hide_room(current_scene, room_portal_other_room(portal, portal->current_room));
        portal->attrs[0].color = (color_t){0, 0, 0, 255};
    }
    
    if (portal->last_player_distance) {
        if (side_a != last_side_a &&
            fabsf(local_offset.x) < portal->transform.scale &&
            fabsf(local_offset.y) < portal->transform.scale) {
            portal->current_room = room_portal_other_room(portal, portal->current_room);
        }
    }

    portal->did_fade = should_fade;

    if (player_distance != 0.0f) {
        portal->last_player_distance = player_distance;
    }
}

void room_portal_init(struct room_portal* portal, struct room_portal_definition* definition) {
    transformSaInit(&portal->transform, &definition->position, &definition->rotation, definition->scale);

    renderable_single_axis_init(&portal->renderable, &portal->transform, "rom:/meshes/objects/room_portal.tmesh");
    render_scene_add_renderable(&portal->renderable, definition->scale * 1.4f);
    update_add(portal, room_portal_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);
    
    portal->room_a = definition->room_a;
    portal->room_b = definition->room_b;

    portal->attrs[0].type = ELEMENT_ATTR_PRIM_COLOR;
    portal->attrs[0].color = (color_t){0, 0, 0, 255};
    portal->attrs[1].type = ELEMENT_ATTR_NONE;

    portal->renderable.attrs = portal->attrs;
    portal->last_player_distance = 0.0f;
    portal->did_fade = false;
}

void room_portal_destroy(struct room_portal* portal) {
    render_scene_remove(&portal->renderable);
    renderable_destroy(&portal->renderable);
    update_remove(portal);
}

void room_portal_common_init() {

}

void room_portal_common_destroy() {

}