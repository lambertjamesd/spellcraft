#include "callback_list.h"

#include <malloc.h>
#include <memory.h>
#include <assert.h>
#include <stdbool.h>
#include "blist.h"

void callback_list_reset(struct callback_list* list, int data_size, int min_capcity, data_compare data_compare_callback) {
    free(list->elements);
    list->elements = NULL;
    memset(list, 0, sizeof(struct callback_list));
    // aligned to 4 bytes
    list->element_size = (sizeof(struct callback_element) + data_size + 3) & ~3;
    list->elements = malloc(list->element_size * min_capcity);
    list->capacity = min_capcity;
    list->data_compare_callback = data_compare_callback;
}

int callback_list_element_compare(void* array, int a_index, int b_index) {
    struct callback_list* list = (struct callback_list*)array;
    struct callback_element* a = callback_list_get(list, a_index);
    struct callback_element* b = callback_list_get(list, b_index);

    if (list->data_compare_callback) {
        int compare_result = list->data_compare_callback(a + 1, b + 1);

        if (compare_result != 0) {
            return compare_result;
        }
    }

    return (int)a->callback - (int)b->callback;
}

callback_id callback_list_insert(struct callback_list* list, void* callback, void* data) {
    assert(list->element_size);

    if (list->capacity == list->count) {
        list->capacity *= 2;
        list->elements = realloc(list->elements, list->element_size * list->capacity);
    }

    list->next_id += 1;
    callback_id result = list->next_id;

    struct callback_element* element = callback_list_get(list, list->count);

    element->callback = callback;
    element->id = result;

    int insert_index = blist_insertion_index(list, list->count, callback_list_element_compare);

    struct callback_element* new_pos = callback_list_get(list, insert_index);

    if (new_pos != element) {
        // shift exisiting elements
        memmove(callback_list_get(list, insert_index + 1), new_pos, (int)element - (int)new_pos);

        // reassign into new position
        element = callback_list_get(list, insert_index);
        element->callback = callback;
        element->id = result;
    }

    // copy data payload in
    memcpy(element + 1, data, list->element_size - sizeof(struct callback_element));

    list->count += 1;

    return result;
}

void callback_list_remove(struct callback_list* list, callback_id id) {
    struct callback_element* found_at = NULL;
    struct callback_element* element = list->elements;

    for (int i = 0; i < list->count; i += 1) {
        if (element->id == id) {
            found_at = element;
            break;
        }

        element = callback_list_next(list, element);
    }

    if (found_at) {
        struct callback_element* array_end = callback_list_get(list, list->count);
        struct callback_element* next_element = callback_list_next(list, found_at);

        if (next_element < array_end) {
            memmove(found_at, next_element, (int)array_end - (int)next_element); 
        }

        list->count -= 1;
    }
}