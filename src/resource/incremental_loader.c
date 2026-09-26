#include "incremental_loader.h"

#include <libdragon.h>

#include "../render/tmesh.h"
#include "../scene/scene.h"

void incremental_loader_init(incremental_loader_t* loader, incremental_resource_type_t start_type, void* resource, void* file) {
    loader->steps[0] = (incremental_loader_step_t){
        .resource = resource,
        .type = start_type,
        .file = file,
        .step = 0,
        .index = 0,
    };
    loader->depth = 1;
    loader->complete_callback = NULL;
    loader->data = NULL;
}

bool incremental_loader_step(incremental_loader_t* loader, uint64_t max_ticks) {
    uint64_t start_time = TICKS_READ();

    do {
        incremental_loader_step_t* curr = &loader->steps[loader->depth-1];

        incremental_step_result_t step_result;

        switch (curr->type) {
            case INCREMENTAL_RESOURCE_TMESH:
                step_result = tmesh_load_incremental(loader, curr);
                break;
            case INCREMENTAL_RESOURCE_ROOM:
                step_result = scene_room_load_incremental(loader, curr);
                break;
            default:
                debugf("bad loader type\n");
                assert(false);
        }

        if (step_result.step_delta == INCREMENTAL_STEP_END) {
            loader->depth -= 1;
            if (loader->depth == 0) {
                if (loader->complete_callback) {
                    loader->complete_callback(loader->data, curr->resource);
                }
                return true;
            }
        } else if (step_result.step_delta) {
            curr->step += step_result.step_delta;
            curr->index = 0;
        } else {
            curr->index += step_result.index_delta;
        }
    } while (TICKS_READ() - start_time < max_ticks);

    return false;
}

void incremental_loader_finish(incremental_loader_t* loader) {
    while (!incremental_loader_step(loader, TICKS_FROM_MS(100))) {};
}

void incremental_loader_push(incremental_loader_t* loader, incremental_resource_type_t type, void* resource, void* file) {
    assert(loader->depth < MAX_RESOURCE_DEPTH);
    loader->steps[loader->depth] = (incremental_loader_step_t){
        .type = type,
        .resource = resource,
        .file = file,
    };
    loader->depth += 1;
}

void incremental_loader_load_full(incremental_resource_type_t type, void* resource, void* file) {
    incremental_loader_t loader;
    incremental_loader_init(&loader, type, resource, file);
    incremental_loader_finish(&loader);
}

#define MAX_QUEUE_LENGTH    4

static incremental_loader_t loader_queue[MAX_QUEUE_LENGTH];
static uint8_t next_write;
static uint8_t next_read;

void incremental_loader_enqueue(incremental_resource_type_t type, void* resource, void* file, incremental_loader_complete callback, void* data) {
    incremental_loader_t* next = &loader_queue[next_write];
    next_write = (next_write + 1) % MAX_QUEUE_LENGTH;

    if (next->depth) {
        incremental_loader_finish(next);
    }

    incremental_loader_init(next, type, resource, file);
    next->complete_callback = callback;
    next->data = data;
}

void incremental_loader_process_queue(uint64_t max_ticks) {
    incremental_loader_t* next = &loader_queue[next_read];

    if (next->depth == 0) {
        return;
    }

    if (incremental_loader_step(next, max_ticks)) {
        next_read = (next_read + 1) % MAX_QUEUE_LENGTH;
    }
}

void incremental_loader_flush_queue() {
    while (loader_queue[next_read].depth > 0) {
        incremental_loader_finish(&loader_queue[next_read]);
        next_read = (next_read + 1) % MAX_QUEUE_LENGTH;
    }
}