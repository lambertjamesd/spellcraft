#include "metrics.h"

#include <libdragon.h>
#include "../menu/menu_rendering.h"
#include "../resource/material_cache.h"
#include "../render/defs.h"
#include "../time/time.h"

static float metrics[PERFORMANCE_METRIC_COUNT];
static uint64_t metric_start_times[PERFORMANCE_METRIC_COUNT];

static material_pair_t* overlay_material;

#define METRICS_WIDTH   100
#define METRICS_PADDING 2
#define METRICS_SPACING 2
#define METRICS_HEIGHT  4

#define FRAME_WIDTH     60

#define METRICS_MARGIN  20

float metrics_frame_ratio(enum performance_metric metric) {
    return metrics[metric] * (1.0f / 1000.0f) * scaled_time_step_inv;
}

void metrics_render(void* data) {
    material_apply(&overlay_material->apply);

    rdpq_sync_pipe();
    rdpq_set_prim_color((color_t){0, 0, 0, 128});

    int x = SCREEN_WD - METRICS_MARGIN - METRICS_WIDTH;
    int y = METRICS_MARGIN;

    // background
    rdpq_fill_rectangle(
        x - METRICS_PADDING,
        y - METRICS_PADDING,
        SCREEN_WD - METRICS_MARGIN + METRICS_PADDING,
        METRICS_MARGIN + METRICS_HEIGHT * 3 + METRICS_SPACING * 2 + METRICS_PADDING
    );
    
    // rsp time
    rdpq_sync_pipe();
    rdpq_set_prim_color((color_t){255, 0, 0, 255});
    int rsp_time_width = (int)(metrics_frame_ratio(PERFORMANCE_METRIC_RSP_TIME) * FRAME_WIDTH);

    rdpq_fill_rectangle(
        x,
        y,
        x + rsp_time_width,
        y + METRICS_HEIGHT
    );

    // cpu time
    y += METRICS_HEIGHT + METRICS_SPACING;

    rdpq_sync_pipe();
    rdpq_set_prim_color((color_t){0, 255, 0, 255});
    int render_time_width = (int)(metrics_frame_ratio(PERFORMANCE_METRIC_RENDER_TIME) * FRAME_WIDTH);
    rdpq_fill_rectangle(
        x,
        y,
        x + render_time_width,
        y + METRICS_HEIGHT
    );
    
    rdpq_sync_pipe();
    rdpq_set_prim_color((color_t){0, 0, 255, 255});
    int update_time_width = (int)(metrics_frame_ratio(PERFORMANCE_METRIC_UPDATE_TIME) * FRAME_WIDTH);
    rdpq_fill_rectangle(
        x + render_time_width,
        y,
        x + render_time_width + render_time_width,
        y + METRICS_HEIGHT
    );
    
    // ram
    y += METRICS_HEIGHT + METRICS_SPACING;

    rdpq_sync_pipe();
    rdpq_set_prim_color((color_t){255, 0, 255, 255});
    int ram_frag = (int)(metrics[PERFORMANCE_METRIC_RAM_FRAG] * metrics[PERFORMANCE_METRIC_RAM] * FRAME_WIDTH);
    rdpq_fill_rectangle(
        x,
        y,
        x + ram_frag,
        y + METRICS_HEIGHT
    );

    rdpq_sync_pipe();
    rdpq_set_prim_color((color_t){0, 255, 255, 255});
    int ram_usage = (int)(metrics[PERFORMANCE_METRIC_RAM] * FRAME_WIDTH);

    rdpq_fill_rectangle(
        x + ram_frag,
        y,
        x + ram_usage,
        y + METRICS_HEIGHT
    );
    
    // limit bar
    rdpq_sync_pipe();
    rdpq_set_prim_color((color_t){255, 255, 255, 255});
    rdpq_fill_rectangle(
        x + FRAME_WIDTH, METRICS_MARGIN,
        x + FRAME_WIDTH + 1,
        METRICS_MARGIN + METRICS_HEIGHT * 3 + METRICS_SPACING * 2
    );
}

void metrics_init() {
    menu_add_callback(metrics_render, &metrics, MENU_PRIORITY_METRICS);
    overlay_material = material_cache_load("rom:/materials/menu/solid_primitive.mat");
}

void metric_set(enum performance_metric metric, float value) {
    metrics[metric] = value;
}

void metric_cpu_start(enum performance_metric metric) {
    metric_start_times[metric] = get_ticks_us();
}

void metric_cpu_end(enum performance_metric metric) {
    metrics[metric] = (get_ticks_us() - metric_start_times[metric]) * (1.0f / 1000.0f);
}