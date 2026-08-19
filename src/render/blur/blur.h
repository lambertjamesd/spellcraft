#ifndef __BLUR_BLUR_H__
#define __BLUR_BLUR_H__

void blur_init();
void blur_destroy();
void blur_buffer(void* buffer, float strength);

#endif