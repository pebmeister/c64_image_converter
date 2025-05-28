#pragma once
#include <stdint.h>
#include <vector>

extern void apply_dithering(uint8_t* image, int width, int height, int channels, 
    const std::vector<std::array<uint8_t, 3>>& palette);

extern void apply_floyd_steinberg(uint8_t* image, int width, int height);
