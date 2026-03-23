#ifndef __UTIL_CLEANUP_H__
#define __UTIL_CLEANUP_H__

#include <stdbool.h>

typedef void (*cleanup_callback)(void*);

void cleanup_safe(cleanup_callback callback, void* data);

void cleanup_set_immediate(bool value);

#endif