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
float fixed_time_step;

int update_compare_elements(void* a, void* b) {
    struct update_element* a_el = (struct update_element*)a;
    struct update_element* b_el = (struct update_element*)b;
    return a_el->priority - b_el->priority;
}

void update_reset() {
    callback_list_reset(&g_update_state.callbacks, sizeof(struct update_element), MIN_UPDATE_COUNT, update_compare_elements);
    fixed_time_step = 1.0f / 30.0f;
}

void update_add(void* data, update_callback callback, int priority, int mask) {
    struct update_element element;

    element.data = data;
    element.priority = priority;
    element.mask = mask;

    callback_list_insert_with_id(&g_update_state.callbacks, callback, &element, (callback_id)data);
}

void update_remove(void* data) {
    callback_list_remove(&g_update_state.callbacks, (callback_id)data);
}

void update_dispatch(int mask) {
    callback_list_begin(&g_update_state.callbacks);

    struct callback_element* current = callback_list_get(&g_update_state.callbacks, 0);

    for (int i = 0; i < g_update_state.callbacks.count; ++i) {
        struct update_element* element = callback_element_get_data(current);

        if (element->mask & mask) {
            ((update_callback)current->callback)(element->data);
        }
        
        current = callback_list_next(&g_update_state.callbacks, current);
    }

    callback_list_end(&g_update_state.callbacks);
}