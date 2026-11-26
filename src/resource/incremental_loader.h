#ifndef __RESOURCE_INCREMENTAL_LOADER_H__
#define __RESOURCE_INCREMENTAL_LOADER_H__

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

enum incremental_resource_type {
    INCREMENTAL_RESOURCE_TMESH,
    INCREMENTAL_RESOURCE_ROOM,
};

typedef enum incremental_resource_type incremental_resource_type_t;

struct incremental_loader_step {
    void* resource;
    void* file;
    uint16_t type;
    uint16_t step;
    uint16_t index;
    uint16_t arg;
};

typedef struct incremental_loader_step incremental_loader_step_t;

typedef void (*incremental_loader_complete)(void* data, void* resource);

#define MAX_RESOURCE_DEPTH      4

struct incremental_loader {
    incremental_loader_step_t steps[MAX_RESOURCE_DEPTH];
    uint16_t depth;
    incremental_loader_complete complete_callback;
    void* data;
};

typedef struct incremental_loader incremental_loader_t;

struct incremental_step_result {
    uint16_t step_delta;
    uint16_t index_delta;
};

typedef struct incremental_step_result incremental_step_result_t;

void incremental_loader_init(incremental_loader_t* loader, incremental_resource_type_t start_type, void* resource, void* file, uint16_t arg);
bool incremental_loader_step(incremental_loader_t* loader, uint64_t max_ticks);
void incremental_loader_finish(incremental_loader_t* loader);

void incremental_loader_push(incremental_loader_t* loader, incremental_resource_type_t type, void* resource, void* file, uint16_t arg);

void incremental_loader_load_full(incremental_resource_type_t type, void* resource, void* file, uint16_t arg);

void incremental_loader_enqueue(incremental_resource_type_t type, void* resource, void* file, incremental_loader_complete callback, void* data, uint16_t arg);
void incremental_loader_process_queue(uint64_t max_ticks);
void incremental_loader_flush_queue();

#define INCREMENTAL_STEP_END     0xFFFF

#define INCREMENTAL_STEP_INDEX (incremental_step_result_t){.step_delta = 0, .index_delta = 1}
#define INCREMENTAL_STEP_ONCE (incremental_step_result_t){.step_delta = 1, .index_delta = 0}
#define INCREMENTAL_STEP_N(n) (incremental_step_result_t){.step_delta = n, .index_delta = 0}
#define INCREMENTAL_STEP_FINISH (incremental_step_result_t){.step_delta = INCREMENTAL_STEP_END, .index_delta = 0}

#endif