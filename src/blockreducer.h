#pragma once
#include <stdint.h>
#include <array>
#include <vector>
#include "pallet.h"

extern void reduce_colors_per_multicolor_block(uint8_t* image, int width, int height, int channels,
    const std::vector<std::array<uint8_t, 3>>&palette = c64_palette,
    int default_bg_index = 0, bool c64_constraints = true);

extern void reduce_colors_per_block(uint8_t* image, int width, int height, int channels,
    const std::vector<std::array<uint8_t, 3>>&palette = c64_palette,
    int default_bg_index = 0, bool c64_constraints = true);

