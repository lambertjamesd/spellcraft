#include "render_batch.h"

void render_batch_init(struct render_batch* batch) {
    batch->element_count = 0;
}

struct render_batch_element* render_batch_add(struct render_batch* batch) {
    if (batch->element_count >= RENDER_BATCH_MAX_SIZE) {
        return NULL;
    }

    struct render_batch_element* result = &batch->elements[batch->element_count];
    ++batch->element_count;

    result->material = NULL;
    result->list = 0;

    return result;
}

int render_batch_compare_element(struct render_batch_element* a, struct render_batch_element* b) {
    return (int)a->material - (int)b->material;
}

void render_batch_sort(struct render_batch* batch, uint16_t* order, uint16_t* order_tmp, int start, int end) {
    if (start + 1 >= end) {
        return;
    }

    int mid = (start + end) >> 1;

    render_batch_sort(batch, order, order_tmp, start, mid);
    render_batch_sort(batch, order, order_tmp, mid, end);

    int a = start;
    int b = mid;
    int output = start;

    while (a < mid || b < end) {
        short a_index = order[a];
        short b_index = order[b];

        if (b >= end || (a < mid && render_batch_compare_element(&batch->elements[a_index], &batch->elements[b_index]) < 0)) {
            order_tmp[output] = order[a];
            ++output;
            ++a;
        } else {
            order_tmp[output] = order[b];
            ++output;
            ++b;
        }
    }

    for (int i = start; i < end; ++i) {
        order[i] = order_tmp[i];
    }
}

void render_batch_finish(struct render_batch* batch) {
    uint16_t order[RENDER_BATCH_MAX_SIZE];
    uint16_t order_tmp[RENDER_BATCH_MAX_SIZE];

    for (int i = 0; i < batch->element_count; ++i) {
        order[i] = i;
    }

    render_batch_sort(batch, order, order_tmp, 0, batch->element_count);

    struct material* current_mat = 0;

    glEnable(GL_RDPQ_MATERIAL_N64);
    rdpq_set_mode_standard();

    for (int i = 0; i < batch->element_count; ++i) {
        int index = order[i];
        struct render_batch_element* element = &batch->elements[index];

        if (!element->list) {
            continue;
        }

        if (current_mat != element->material) {
            glCallList(element->material->list);
            current_mat = element->material;
        }

        glPushMatrix();
        glMultMatrixf((GLfloat*)element->transform);
        glCallList(element->list);
        glPopMatrix();
    }

    glDisable(GL_RDPQ_MATERIAL_N64);
}