#ifndef __UTIL_RSP_TIMER_H__
#define __UTIL_RSP_TIMER_H__

struct timer_output {
    int result;
    int padding;
} __attribute__((aligned(8)));

typedef struct timer_output timer_output_t;

void rsp_timer_init();
void rsp_timer_destroy();

void rsp_timer_start(unsigned index);
void rsp_timer_end(unsigned index, timer_output_t* output);

float rsp_timer_output_ms(timer_output_t* output);

#endif