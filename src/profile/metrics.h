#ifndef __PROFILE_METRICS_H__
#define __PROFILE_METRICS_H__

#define ENABLE_METRICS      1

enum performance_metric {
    PERFORMANCE_METRIC_RSP_TIME,
    PERFORMANCE_METRIC_RENDER_TIME,
    PERFORMANCE_METRIC_UPDATE_TIME,
    PERFORMANCE_METRIC_RAM,
    PERFORMANCE_METRIC_RAM_FRAG,
    
    PERFORMANCE_METRIC_COUNT,
};

void metrics_init();
void metric_set(enum performance_metric metric, float value);

void metric_cpu_start(enum performance_metric metric);
void metric_cpu_end(enum performance_metric metric);

#endif