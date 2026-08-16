#ifndef __TUTORIAL_MENU_H__
#define __TUTORIAL_MENU_H__

#include "../scene/scene_definition.h"

typedef enum tutorial_menu_state tutorial_menu_state_t;

struct tutorial_menu {
    tutorial_menu_state_t state;
};

typedef struct tutorial_menu tutorial_menu_t;

void tutorial_set_step(tutorial_menu_state_t state);

#endif