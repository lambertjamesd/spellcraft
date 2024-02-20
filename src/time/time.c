#include "time.h"

#include <malloc.h>
#include <memory.h>

#include "../util/blist.h"
#include "../util/callback_list.h"

struct update_element {
    void* data;
    short priority;
    short mask;
};

struct update_state {
    struct callback_list callbacks;
};

#define MIN_UPDATE_COUNT    64

static struct update_state g_update_state;

int update_compare_elements(void* a, void* b) {
    struct update_element* a_el = (struct update_element*)a;
    struct update_element* b_el = (struct update_element*)b;
    return a_el->priority - b_el->priority;
}

void update_reset() {
    callback_list_reset(&g_update_state.callbacks, sizeof(struct update_element), MIN_UPDATE_COUNT, update_compare_elements);
}

update_id update_add(void* data, update_callback callback, int priority, int mask) {
    struct update_element element;

    element.data = data;
    element.priority = priority;
    element.mask = mask;

    return callback_list_insert(&g_update_state.callbacks, callback, &element);
}

void update_remove(update_id id) {
    callback_list_remove(&g_update_state.callbacks, id);
}

void update_dispatch(int mask) {

}