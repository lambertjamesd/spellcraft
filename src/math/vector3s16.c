#include "vector3s16.h"

#include "../math/minmax.h"

void vector3s16Min(vector3s16_t* a, vector3s16_t* b, vector3s16_t* out) {
    out->x = MIN(a->x, b->x);
    out->y = MIN(a->y, b->y);
    out->z = MIN(a->z, b->z);
}

void vector3s16Max(vector3s16_t* a, vector3s16_t* b, vector3s16_t* out) {
    out->x = MAX(a->x, b->x);
    out->y = MAX(a->y, b->y);
    out->z = MAX(a->z, b->z);
}