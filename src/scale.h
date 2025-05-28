#pragma once
#include <stdint.h>

// Scale image down to fit within C64 resolution while maintaining aspect ratio
void scale_to_c64(const uint8_t* input, int in_width, int in_height, 
                 uint8_t* output, int out_width, int out_height, int channels) ;