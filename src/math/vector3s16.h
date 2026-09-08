#ifndef __MATH_VECTOR3S16_H__
#define __MATH_VECTOR3S16_H__

struct Vector3s16 {
    short x;
    short y;
    short z;
};

typedef struct Vector3s16 vector3s16_t;

void vector3s16Min(vector3s16_t* a, vector3s16_t* b, vector3s16_t* out);
void vector3s16Max(vector3s16_t* a, vector3s16_t* b, vector3s16_t* out);

#endif