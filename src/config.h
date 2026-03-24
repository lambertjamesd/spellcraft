#ifndef __GLOBAL_CONFIG_H__
#define __GLOBAL_CONFIG_H__

#define ENABLE_CHEATS   0

#define DEBUG_ENABLED   1

#define ENABLE_BIG_SCREEN_SHOT  0
#define ENABLE_LOD_RENDER_DEBUG 0

#if ENABLE_LOD_RENDER_DEBUG

#define LOD_RENDER_MODE_DEFAULT     -1  
#define LOD_RENDER_MODE_DETAILED    -2
#define LOD_RENDER_MODE_LOD3        -3

extern int lod_render_mode;

#endif

#endif