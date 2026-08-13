#include "globals.h"

int global_load_location(global_location_t location) {
    return evaluation_context_load(savefile_get_globals(GLOBAL_ACCESS_MODE_READ), location.data_type, location.word_offset);
}