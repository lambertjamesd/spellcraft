#ifndef __UTIL_RSP_TIMER_H__
#define __UTIL_RSP_TIMER_H__

void rsp_timer_init();
void rsp_timer_destroy();

void rsp_timer_start(unsigned index);
void rsp_timer_end(unsigned index);

float rsp_timer_get(unsigned index);

#endif