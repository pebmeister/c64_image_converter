#pragma once
#include <stdint.h>
#include <array>
#include <vector>
#include "pallet.h"

extern void reduce_colors_per_multicolor_block(uint8_t* image, int width, int height, int channels,
    const std::vector<std::array<uint8_t, 3>>&palette = c64_palette,
    int default_bg_index = C64_BLACK, bool c64_constraints = true);

extern void reduce_colors_per_block(uint8_t* image, int width, int height, int channels,
    const std::vector<std::array<uint8_t, 3>>&palette = c64_palette,
    int default_bg_index = C64_BLACK, bool c64_constraints = true);

extern void convert_to_c64_hires(uint8_t* image, int width, int height, int bg_color = C64_BLACK);
extern void convert_to_c64_multicolor(uint8_t* image, int width, int height, int bg_color = C64_BLACK);

