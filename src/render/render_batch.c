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
    result->transform = NULL;

    return result;
}

void render_batch_add_mesh(struct render_batch* batch, struct mesh* mesh, mat4x4* transform) {
    for (int i = 0; i < mesh->submesh_count; ++i) {
        struct render_batch_element* element = render_batch_add(batch);

        if (!element) {
            return;
        }

        element->list = mesh->list + i;
        element->material = mesh->materials[i];
        element->transform = transform;
    }
}

mat4x4* render_batch_get_transform(struct render_batch* batch) {
    if (batch->transform_count >= RENDER_BATCH_TRANSFORM_COUNT) {
        return NULL;
    }

    mat4x4* result = &batch->transform[batch->transform_count];
    ++batch->transform_count;
    return result;
}

int render_batch_compare_element(struct render_batch_element* a, struct render_batch_element* b) {
    if (a == b) {
        return 0;
    }

    if (a->material->sortPriority != b->material->sortPriority) {
        return a->material->sortPriority - b->material->sortPriority;
    }

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

    glEnable(GL_CULL_FACE);
    glEnable(GL_RDPQ_MATERIAL_N64);
    // glEnable(GL_RDPQ_TEXTURING_N64);
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

        if (element->transform) {
            glPushMatrix();
            glMultMatrixf((GLfloat*)element->transform);
        }

        glCallList(element->list);

        if (element->transform) {
            glPopMatrix();
        }
    }

    glDisable(GL_RDPQ_MATERIAL_N64);
    glDisable(GL_RDPQ_TEXTURING_N64);
}