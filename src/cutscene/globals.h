#ifndef __CUTSCENE_GLOBALS_H__
#define __CUTSCENE_GLOBALS_H__

#include <stdint.h>
#include <stdbool.h>
#include "evaluation_context.h"
#include "../savefile/savefile.h"
#include "global_list.h"

#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define CONCAT_IMPL(a, b) a##b

#define LOAD_DATA_TYPE_S8(offset)   (int8_t)evaluation_context_load(savefile_get_globals(GLOBAL_ACCESS_MODE_READ), DATA_TYPE_S8, offset)
#define LOAD_DATA_TYPE_S16(offset)   (int16_t)evaluation_context_load(savefile_get_globals(GLOBAL_ACCESS_MODE_READ), DATA_TYPE_S16, offset)
#define LOAD_DATA_TYPE_S32(offset)   (int32_t)evaluation_context_load(savefile_get_globals(GLOBAL_ACCESS_MODE_READ), DATA_TYPE_S32, offset)
#define LOAD_DATA_TYPE_BOOL(offset)   (bool)evaluation_context_load(savefile_get_globals(GLOBAL_ACCESS_MODE_READ), DATA_TYPE_BOOL, offset)
#define LOAD_DATA_TYPE_F32(offset)   evaluation_context_load_float(savefile_get_globals(GLOBAL_ACCESS_MODE_READ), DATA_TYPE_F32, offset)
#define LOAD_DATA_TYPE_ADDRESS(offset)   (int32_t)evaluation_context_load(savefile_get_globals(GLOBAL_ACCESS_MODE_READ), DATA_TYPE_ADDRESS, offset)

#define LOAD_GLOBAL(name) CONCAT(LOAD_, VAR_TYP_##name)(VAR_POS_##name)

int global_load_location(global_location_t location);

#endif