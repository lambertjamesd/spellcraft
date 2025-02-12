#ifndef __SPELL_TELEPORT_H__
#define __SPELL_TELEPORT_H__

#include "spell_sources.h"
#include "spell_event.h"

enum teleport_dir {
    TELEPORT_DIR_SIDE,
    TELEPORT_DIR_UP_DOWN,
};

struct teleport {
    entity_id target;
    struct Vector3 saved_velocity;
    float teleport_time;
    enum teleport_dir dir;
};

void teleport_init(struct teleport* teleport, struct spell_data_source* source, struct spell_event_options event_options, enum teleport_dir dir);
void teleport_destroy(struct teleport* teleport);

bool teleport_update(struct teleport* teleport, struct spell_event_listener* event_listener, struct spell_sources* spell_sources);

#endif