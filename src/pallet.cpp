#include <limits>
#include <cmath>
#include <vector>
#include "pallet.h"
#include <string>


const std::vector<std::string> c64_color_names = {
    "Black", "White", "Red", "Cyan", "Purple", "Green", "Blue", "Yellow",
    "Orange", "Brown", "Light Red", "Dark Grey", "Medium Grey",
    "Light Green", "Light Blue", "Light Grey"
};

std::string get_color_name(uint8_t color_index)
{
    return (color_index < c64_color_names.size()) ? c64_color_names[color_index] : "Unknown";
}

// Commodore 64 color palette (RGB values)
const std::vector<std::array<uint8_t, 3>> c64_palette = {{
    {0, 0, 0},       // Black
    {255, 255, 255}, // White
    {136, 0, 0},     // Red
    {170, 255, 238}, // Cyan
    {204, 68, 204},  // Purple
    {0, 204, 85},    // Green
    {0, 0, 170},     // Blue
    {238, 238, 119}, // Yellow
    {221, 136, 85},  // Orange
    {102, 68, 0},    // Brown
    {255, 119, 119}, // Light red
    {51, 51, 51},    // Dark grey
    {119, 119, 119}, // Medium grey
    {170, 255, 102}, // Light green
    {0, 136, 255},   // Light blue
    {187, 187, 187}  // Light grey
}};

// Calculate Euclidean distance between two RGB colors
float color_distance(const uint8_t* color1, const uint8_t* color2) {
    float r_diff = static_cast<float>(color1[0]) - color2[0];
    float g_diff = static_cast<float>(color1[1]) - color2[1];
    float b_diff = static_cast<float>(color1[2]) - color2[2];
    return std::sqrt(r_diff * r_diff + g_diff * g_diff + b_diff * b_diff);
}

// Find closest C64 palette color
uint8_t find_closest_color(const uint8_t* color, const std::vector<std::array<uint8_t, 3>>& pallette) {
    uint8_t closest = 0;
    float min_dist = std::numeric_limits<float>::max();
    
    for (uint8_t i = 0; i < pallette.size(); ++i) {
        float dist = color_distance(color, pallette[i].data());
        if (dist < min_dist) {
            min_dist = dist;
            closest = i;
        }
    }
    
    return closest;
}
