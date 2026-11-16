#ifndef __RESOURCE_SPRITE_CACHE_H__
#define __RESOURCE_SPRITE_CACHE_H__

#include <libdragon.h>

void sprite_cache_init();
sprite_t* sprite_cache_load(const char* filename);
void sprite_cache_release_direct(sprite_t* sprite);
void sprite_cache_release(sprite_t* sprite);

void sprite_cache_destroy();

#endif