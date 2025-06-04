#include <array>
#include <algorithm>
#include <vector>
#include "blockreducer.h"
#include "pallet.h"
#include <cassert>
#include <map>

void reduce_colors_per_multicolor_block(uint8_t* image, int width, int height, int channels,
    const std::vector<std::array<uint8_t, 3>>& palette = c64_palette);

void reduce_colors_per_block(uint8_t* image, int width, int height, int channels,
    const std::vector<std::array<uint8_t, 3>>& palette = c64_palette);


void convert_to_c64_hires(uint8_t* image, int width, int height, int bg_color)
{
    reduce_colors_per_block(image, width, height, 3, c64_palette);
}

void convert_to_c64_multicolor(uint8_t* image, int width, int height, int bg_color)
{
    reduce_colors_per_multicolor_block(image, width, height, 3, c64_palette);
}

void reduce_colors_per_multicolor_block(uint8_t* image, int width, int height, int channels,
    const std::vector<std::array<uint8_t, 3>>& palette)
{
    const int palette_size = static_cast<int>(palette.size());
    assert(palette_size == 16);
    const int block_width = 4;  // Multicolor blocks are 4x8 pixels
    const int block_height = 8;

    std::map<std::pair<int, int>, std::array<int, 16>> colorfreqdict;

    // Step 1: Get frequency of colors in each 4x8 block
    for (auto y = 0; y < height; y++) {
        auto block_row = y / block_height;

        for (auto x = 0; x < width; x++) {
            auto block_col = x / block_width;

            if (colorfreqdict.find({ block_row, block_col }) == colorfreqdict.end()) {
                colorfreqdict[{block_row, block_col}] = { 0 };
            }

            auto idx = (y * width + x) * 3;
            uint8_t color_idx = find_closest_color(&image[idx], palette);
            colorfreqdict[{block_row, block_col}][color_idx]++;
        }
    }

    // Step 2: For each block, select 3 most common colors + background (black)
    std::map<std::pair<int, int>, std::array<uint8_t, 4>> block_colors;
    for (auto block_row = 0; block_row < height / block_height; block_row++) {
        for (auto block_col = 0; block_col < width / block_width; block_col++) {
            auto& freq = colorfreqdict[{block_row, block_col}];

            // Find top 3 colors (excluding background)
            std::array<uint8_t, 3> top_colors = { 0, 0, 0 };
            for (int i = 1; i < 16; i++) { // Skip background (0)
                if (freq[i] > freq[top_colors[0]]) {
                    top_colors[2] = top_colors[1];
                    top_colors[1] = top_colors[0];
                    top_colors[0] = i;
                }
                else if (freq[i] > freq[top_colors[1]]) {
                    top_colors[2] = top_colors[1];
                    top_colors[1] = i;
                }
                else if (freq[i] > freq[top_colors[2]]) {
                    top_colors[2] = i;
                }
            }

            // Store colors: [background, mc1, mc2, mc3]
            block_colors[{block_row, block_col}] = { 0, top_colors[0], top_colors[1], top_colors[2] };
        }
    }

    // Step 3: Remap pixels to their closest selected color
    for (auto y = 0; y < height; y++) {
        auto block_row = y / block_height;

        for (auto x = 0; x < width; x++) {
            auto block_col = x / block_width;
            auto& colors = block_colors[{block_row, block_col}];

            auto idx = (y * width + x) * 3;
            uint8_t original_idx = find_closest_color(&image[idx], palette);

            // Find closest color from our selected 4
            float min_dist = FLT_MAX;
            uint8_t best_color = 0;
            for (int i = 0; i < 4; i++) {
                float dist = color_distance(&image[idx], colors[i]);
                if (dist < min_dist) {
                    min_dist = dist;
                    best_color = colors[i];
                }
            }

            // Set pixel to closest color
            for (int i = 0; i < 3; i++) {
                image[idx + i] = palette[best_color][i];
            }
        }
    }
}

void reduce_colors_per_block(uint8_t* image, int width, int height, int channels,
    const std::vector<std::array<uint8_t, 3>>& palette)
{
    const int palette_size = static_cast<int>(palette.size());
    assert(palette_size == 16);
    std::map<std::pair<int, int>, std::array<int, 16>> colorfreqdict;
    // step 1 get the frequency of each 8x8 bloack
    for (auto y = 0; y < height; y++) {
        auto row = y / 8;

        for (auto x = 0; x < width; ++x) {
            auto ch = x / 8;

            if (colorfreqdict.find({ row, ch }) == colorfreqdict.end()) {
                colorfreqdict[{row, ch}] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
            }
            auto& freq = colorfreqdict[{row, ch}];

            auto idx = (y * width + x) * 3;
            uint8_t color_idx = find_closest_color(&image[idx], c64_palette);
            freq[color_idx]++;
        }
    }

    std::map<std::pair<int, int>, std::array<int, 2>> colordict;
    for (auto row = 0; row < 25; ++row) {
        for (auto ch = 0; ch < 40; ++ch) {
            auto& freq = colorfreqdict[{row, ch}];
            auto hi = 0;
            auto next = 0;

            for (auto i = 1; i < 16; ++i) {
                if (freq[i] > freq[hi]) {
                    next = hi;
                    hi = i;
                }
                else if (freq[i] > freq[next]) {
                    next = i;
                }
            }

            colordict[{row, ch}][0] = hi;
            colordict[{row, ch}][1] = next;
        }
    }

    // Remap pixels
    for (auto y = 0; y < height; y++) {
        auto row = y / 8;

        for (auto x = 0; x < width; ++x) {
            auto ch = x / 8;

            auto& color = colordict[{row, ch}];
            auto idx = (y * width + x) * 3;

            float d0 = color_distance(&image[idx], color[0]);
            float d1 = color_distance(&image[idx], color[1]);

            auto n = (d0 < d1) ? 0 : 1;
            for (auto i = 0; i < 3; ++i)
                image[idx + i] = c64_palette[color[n]][i];
        }
    }
}