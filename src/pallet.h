#pragma once
#include <array>
#include <stdint.h>
#include <vector>

// Commodore 64 color palette (RGB values)
extern const std::vector<std::array<uint8_t, 3>> c64_palette;

// Calculate Euclidean distance between two RGB colors
extern float color_distance(const uint8_t* color1, const uint8_t* color2);

// Find closest C64 palette color
extern uint8_t find_closest_color(const uint8_t* color, 
    const std::vector<std::array<uint8_t, 3>>& pallete);

