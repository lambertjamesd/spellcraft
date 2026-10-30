#include "globals.h"

int global_load_location(global_location_t location) {
    return evaluation_context_load(savefile_get_globals(GLOBAL_ACCESS_MODE_READ), location.data_type, location.word_offset);
}

void global_save_location(global_location_t location, int value) {
    evaluation_context_save(savefile_get_globals(GLOBAL_ACCESS_MODE_WRITE), location.data_type, location.word_offset, value);
}