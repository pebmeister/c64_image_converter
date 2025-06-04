#pragma once
#include <array>
#include <stdint.h>
#include <vector>

// Commodore 64 color palette (RGB values)
extern const std::vector<std::array<uint8_t, 3>> c64_palette;

// Calculate Euclidean distance between two RGB colors
extern float color_distance(const uint8_t* color1, const uint8_t* color2);
extern uint8_t find_color_index(const uint8_t* color, const std::vector<std::array<uint8_t, 3>>& pallette);
extern float color_distance(const uint8_t* color1, int index);

// Find closest C64 palette color
extern uint8_t find_closest_color(const uint8_t* color, 
    const std::vector<std::array<uint8_t, 3>>& palette);

enum C64Color {
    C64_BLACK, C64_WHITE, C64_RED, C64_CYAN,
    C64_PURPLE, C64_GREEN, C64_BLUE, C64_YELLOW,
    C64_ORANGE, C64_BROWN, C64_LT_RED, C64_DK_GREY,
    C64_MD_GREY, C64_LT_GREEN, C64_LT_BLUE, C64_LT_GREY
};
