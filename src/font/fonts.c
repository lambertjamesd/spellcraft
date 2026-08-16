#include "fonts.h"

#include <stdint.h>
#include "assert.h"

rdpq_font_t* font_types[FONT_COUNT];
static uint8_t font_ref_count[FONT_COUNT];
static const char* font_sources[FONT_COUNT] = {
    [FONT_DIALOG] = "rom:/fonts/Amarante-Regular.font64",
    [FONT_TITLE] = "rom:/fonts/HeavyEquipment.font64",
};

void font_type_use(font_type_t type) {
    assert(type > FONT_NONE && type < FONT_COUNT);
    assert(font_ref_count[type] < 255);

    if (font_ref_count[type] == 0) {
        font_types[type] = rdpq_font_load(font_sources[type]);
        rdpq_text_register_font(type, font_types[type]);
    }
    ++font_ref_count[type];
}

void font_type_release_font(void* data) {
    rdpq_font_free(data);
}

void font_type_release(font_type_t type) {
    assert(type > FONT_NONE && type < FONT_COUNT);
    assert(font_ref_count[type] > 0);

    --font_ref_count[type];
    if (font_ref_count[type] == 0) {
        rdpq_text_unregister_font(type);
        rspq_call_deferred(font_type_release_font, font_types[type]);
        font_types[type] = NULL;
    }
}

int measure_text(enum font_type font, const char* message, int char_count) {
    const char* curr = message;

    int result = 0;

    while (char_count) {
        rdpq_font_gmetrics_t metrics;
        bool was_found = rdpq_font_get_glyph_metrics(font_get(font), *curr, &metrics);
        ++curr;
        --char_count;

        if (was_found) {
            result += metrics.xadvance;
        }
    }
    
    return result;
}