#include "explosion.h"

#include "../render/render_scene.h"
#include "assets.h"
#include "../time/time.h"
#include "../math/mathf.h"

#define EXPLOSION_TIME      0.25f
#define EXPLOSION_RADIUS    2.0f

void explosion_render(struct explosion* explosion, struct render_batch* batch) {
    struct render_batch_billboard_element* element = render_batch_add_particles(batch, spell_assets_get()->fire_particle_mesh, 1);

    int alpha = 255 - ((int)((255.0f / (EXPLOSION_TIME * EXPLOSION_TIME)) * explosion->total_time * explosion->total_time));

    if (alpha < 0) {
        alpha = 0;
    } else if (alpha > 255) {
        alpha = 255;
    }

    element->sprites->position = explosion->position;
    element->sprites->radius = explosion->total_time * (EXPLOSION_RADIUS / EXPLOSION_TIME);
    element->sprites->color.r = 255;
    element->sprites->color.g = 255;
    element->sprites->color.b = 255;
    element->sprites->color.a = alpha;
}

void explosion_init(struct explosion* explosion, struct spell_data_source* source, struct spell_event_options event_options) {
    render_scene_add(&explosion->data_source->position, 4.0f, (render_scene_callback)explosion_render, explosion);

    explosion->data_source = source;
    spell_data_source_retain(source);

    explosion->position = source->position;
}

void explosion_destroy(struct explosion* explosion) {
    render_scene_remove(explosion);
    spell_data_source_release(explosion->data_source);
}

void explosion_update(struct explosion* explosion, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    explosion->total_time += fixed_time_step;

    if (explosion->total_time >= EXPLOSION_TIME) {
        spell_event_listener_add(event_listener, SPELL_EVENT_DESTROY, NULL, 0.0f);
    }
}
