#ifndef __ENTITY_ID_H__
#define __ENTITY_ID_H__

typedef unsigned short entity_id;

enum fixed_entity_ids {
    ENTITY_ID_PLAYER = 1,

    ENTITY_ID_FIRST_DYNAMIC,
};

entity_id entity_id_new();

#endif