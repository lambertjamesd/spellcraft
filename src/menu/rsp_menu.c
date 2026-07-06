#include "rsp_menu.h"

#define F32_TO_FIXED(val) (int32_t)((val) * (float)(1<<16))

void menu_transform_to_fixed(transform_2d_fp_t* output, const transform_2d_t input) {
    int16_t* int_part = output->int_part;
    uint16_t* frac_part = output->frac_part;
    const float* input_ptr = input;
    
    for (int i = 0; i < 2; i += 1) {
        uint32_t fixed_a = F32_TO_FIXED(input_ptr[0]);
        uint32_t fixed_b = F32_TO_FIXED(input_ptr[1]);
        uint32_t fixed_c = F32_TO_FIXED(input_ptr[2]);

        *(uint64_t*)int_part = 
            ((uint64_t)(fixed_a & 0xFFFF0000) << 32) | 
            ((uint64_t)(fixed_b & 0xFFFF0000) << 16) | 
            (uint64_t)(fixed_c & 0xFFFF0000);
        
        *(uint64_t*)frac_part = 
            ((uint64_t)(fixed_a & 0xFFFF) << 48) | 
            ((uint64_t)(fixed_b & 0xFFFF) << 32) | 
            ((uint64_t)(fixed_c & 0xFFFF) << 16);

        int_part += 4;
        frac_part += 4;
        input_ptr += 3;
    }
}