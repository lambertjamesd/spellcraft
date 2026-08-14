#ifndef __TUTORIAL_MENU_H__
#define __TUTORIAL_MENU_H__

enum tutorial_menu_state {
    TUTORAIL_MENU_STATE_NONE,
    TUTORAIL_MENU_HOLD_Z,
    TUTORAIL_MENU_PRESS_A,
};

typedef enum tutorial_menu_state tutorial_menu_state_t;

struct tutorial_menu {
    tutorial_menu_state_t state;
};

typedef struct tutorial_menu tutorial_menu_t;

void tutorial_show(tutorial_menu_state_t state);

#endif