
#ifndef _GRAPHICS_COLOR_H
#define _GRAPHICS_COLOR_H

struct Coloru8 {
    unsigned char r, g, b, a;
};

struct Coloru8 coloru8_lerp(struct Coloru8* a, struct Coloru8* b, float amount);

#endif