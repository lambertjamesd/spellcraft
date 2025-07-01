#include "coloru8.h"

struct Coloru8 coloru8_lerp(struct Coloru8* a, struct Coloru8* b, float amount) {
    int amount_as_int = (int)(0x10000 * amount);
    int inv_amount_as_int = 0x10000 - amount_as_int;

    return (struct Coloru8){
        .r = (a->r * inv_amount_as_int + b->r * amount_as_int) >> 16,
        .g = (a->g * inv_amount_as_int + b->g * amount_as_int) >> 16,
        .b = (a->b * inv_amount_as_int + b->b * amount_as_int) >> 16,
        .a = (a->a * inv_amount_as_int + b->a * amount_as_int) >> 16,
    };
}