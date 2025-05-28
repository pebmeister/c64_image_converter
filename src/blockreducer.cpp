#include <array>
#include <algorithm>
#include <vector>
#include "blockreducer.h"
#include "pallet.h"

void reduce_colors_per_multicolor_block(uint8_t* image, int width, int height, int channels,
    const std::vector<std::array<uint8_t, 3>>& palette,
    int default_bg_index = 0)
{
    // Input validation
    if (!image || width <= 0 || height <= 0 || channels < 3 || palette.empty()) {
        return;
    }

    const int palette_size = static_cast<int>(palette.size());
    const int block_w = 4, block_h = 8; // C64 multicolor block size

    for (int by = 0; by < height; by += block_h) {
        for (int bx = 0; bx < width; bx += block_w) {
            std::vector<int> color_count(palette_size, 0);

            // Count palette color usage in block
            for (int y = 0; y < block_h && (by + y) < height; ++y) {
                for (int x = 0; x < block_w && (bx + x) < width; ++x) {
                    int idx = ((by + y) * width + (bx + x)) * channels;
                    uint8_t palette_idx = find_closest_color(&image[idx], palette);
                    color_count[palette_idx]++;
                }
            }

            // Prepare color frequency list (skip zero counts)
            std::vector<std::pair<int, int>> freq;
            for (int i = 0; i < palette_size; ++i) {
                if (color_count[i] > 0 && i != default_bg_index) {
                    freq.emplace_back(color_count[i], i);
                }
            }

            // Ensure we have enough colors (C64 multicolor needs bg + 1-3 colors)
            if (freq.empty()) {
                freq.emplace_back(1, (default_bg_index + 1) % palette_size); // Fallback color
            }
            else if (freq.size() < 3) {
                // Add secondary colors if needed
                for (int i = freq.size(); i < 3; ++i) {
                    freq.emplace_back(1, (freq.back().second + 1) % palette_size);
                }
            }

            std::sort(freq.begin(), freq.end(), std::greater<>());

            // C64 multicolor: 1 bg + up to 3 local colors
            std::array<int, 4> block_colors = { default_bg_index };
            for (int i = 0; i < std::min(3, static_cast<int>(freq.size())); ++i) {
                block_colors[i + 1] = freq[i].second;
            }

            // Remap pixels to the selected colors
            for (int y = 0; y < block_h && (by + y) < height; ++y) {
                for (int x = 0; x < block_w && (bx + x) < width; ++x) {
                    int idx = ((by + y) * width + (bx + x)) * channels;

                    // Find closest color from our selected 4
                    float min_dist = std::numeric_limits<float>::max();
                    int best_color = 0;
                    for (int k = 0; k < 4; ++k) {
                        float dist = color_distance(&image[idx], palette[block_colors[k]].data());
                        if (dist < min_dist) {
                            min_dist = dist;
                            best_color = k;
                        }
                    }

                    // Apply the color
                    const auto& color = palette[block_colors[best_color]];
                    image[idx] = color[0];
                    image[idx + 1] = color[1];
                    image[idx + 2] = color[2];
                }
            }
        }
    }
}

void reduce_colors_per_block(uint8_t* image, int width, int height, int channels,
    const std::vector<std::array<uint8_t, 3>>& palette,
    int default_bg_index = 0)
{
    // Input validation
    if (!image || width <= 0 || height <= 0 || channels < 3 || palette.empty()) {
        return;
    }

    const int palette_size = static_cast<int>(palette.size());
    const int block_size = 8; // C64 character block size

    for (int by = 0; by < height; by += block_size) {
        for (int bx = 0; bx < width; bx += block_size) {
            std::vector<int> color_count(palette_size, 0);

            // Count color frequencies in the block
            for (int y = 0; y < block_size && (by + y) < height; ++y) {
                for (int x = 0; x < block_size && (bx + x) < width; ++x) {
                    int idx = ((by + y) * width + (bx + x)) * channels;
                    uint8_t palette_idx = find_closest_color(&image[idx], palette);
                    color_count[palette_idx]++;
                }
            }

            // Find the two most frequent non-background colors
            int first = default_bg_index, second = default_bg_index;
            for (int i = 0; i < palette_size; ++i) {
                if (i == default_bg_index) continue;

                if (color_count[i] > color_count[first]) {
                    second = first;
                    first = i;
                }
                else if (color_count[i] > color_count[second] || second == default_bg_index) {
                    second = i;
                }
            }

            // If only one color found, pick a contrasting second color
            if (first == second || second == default_bg_index) {
                second = (first + palette_size / 2) % palette_size; // Opposite in palette
            }

            // Remap all pixels in the block
            for (int y = 0; y < block_size && (by + y) < height; ++y) {
                for (int x = 0; x < block_size && (bx + x) < width; ++x) {
                    int idx = ((by + y) * width + (bx + x)) * channels;

                    // Compare distances to both colors
                    float dist_first = color_distance(&image[idx], palette[first].data());
                    float dist_second = color_distance(&image[idx], palette[second].data());
                    float dist_bg = color_distance(&image[idx], palette[default_bg_index].data());

                    // Find closest of the three (including background)
                    const auto& color = (dist_bg < dist_first && dist_bg < dist_second)
                        ? palette[default_bg_index]
                        : (dist_first < dist_second)
                            ? palette[first]
                            : palette[second];

                            image[idx] = color[0];
                            image[idx + 1] = color[1];
                            image[idx + 2] = color[2];
                }
            }
        }
    }
}

void reduce_colors_per_multicolor_block(uint8_t* image, int width, int height, int channels,
    int default_bg_index, bool c64_constraints)
{
    if (!image || width <= 0 || height <= 0 || channels < 3 || c64_palette.empty()) return;

    const int block_w = 4, block_h = 8; // C64 multicolor block size
    const int palette_size = static_cast<int>(c64_palette.size());

    for (int by = 0; by < height; by += block_h) {
        for (int bx = 0; bx < width; bx += block_w) {
            std::vector<int> color_count(palette_size, 0);

            // Count colors in block
            for (int y = 0; y < block_h && (by + y) < height; ++y) {
                for (int x = 0; x < block_w && (bx + x) < width; ++x) {
                    int idx = ((by + y) * width + (bx + x)) * channels;
                    color_count[find_closest_color(&image[idx], c64_palette)]++;
                }
            }

            // Prepare color candidates (excluding background)
            std::vector<std::pair<int, int>> freq;
            for (int i = 0; i < palette_size; ++i) {
                if (color_count[i] > 0 && (!c64_constraints || i != default_bg_index)) {
                    freq.emplace_back(color_count[i], i);
                }
            }

            // Handle C64's 3+1 color limitation
            std::array<int, 4> block_colors;
            if (c64_constraints) {
                block_colors[0] = default_bg_index; // Fixed background

                // Sort by frequency and take top 3
                std::sort(freq.begin(), freq.end(), std::greater<>());
                for (int i = 0; i < 3; ++i) {
                    block_colors[i + 1] = (i < freq.size()) ? freq[i].second :
                        (i == 0) ? 1 : (block_colors[i] + 1) % palette_size;
                }
            }
            else {
                // Non-C64 mode: take top 4 colors
                std::sort(freq.begin(), freq.end(), std::greater<>());
                for (int i = 0; i < 4; ++i) {
                    block_colors[i] = (i < freq.size()) ? freq[i].second :
                        (i == 0) ? 0 : (block_colors[i - 1] + 1) % palette_size;
                }
            }

            // Remap pixels
            for (int y = 0; y < block_h && (by + y) < height; ++y) {
                for (int x = 0; x < block_w && (bx + x) < width; ++x) {
                    int idx = ((by + y) * width + (bx + x)) * channels;

                    // Find closest from our selected colors
                    float min_dist = std::numeric_limits<float>::max();
                    int best_idx = 0;
                    for (int i = 0; i < 4; ++i) {
                        float dist = color_distance(&image[idx], c64_palette[block_colors[i]].data());
                        if (dist < min_dist) {
                            min_dist = dist;
                            best_idx = i;
                        }
                    }

                    // Apply color
                    const auto& color = c64_palette[block_colors[best_idx]];
                    std::copy(color.begin(), color.end(), &image[idx]);
                }
            }
        }
    }
}

void reduce_colors_per_block(uint8_t* image, int width, int height, int channels,
    int default_bg_index, bool c64_constraints)
{
    if (!image || width <= 0 || height <= 0 || channels < 3 || c64_palette.empty()) return;

    const int block_size = 8; // C64 character size
    const int palette_size = static_cast<int>(c64_palette.size());

    for (int by = 0; by < height; by += block_size) {
        for (int bx = 0; bx < width; bx += block_size) {
            std::vector<int> color_count(palette_size, 0);

            // Count colors in block
            for (int y = 0; y < block_size && (by + y) < height; ++y) {
                for (int x = 0; x < block_size && (bx + x) < width; ++x) {
                    int idx = ((by + y) * width + (bx + x)) * channels;
                    color_count[find_closest_color(&image[idx], c64_palette)]++;
                }
            }

            // Find top colors (excluding background if constrained)
            int first = 0, second = 0;
            for (int i = 0; i < palette_size; ++i) {
                if (c64_constraints && i == default_bg_index) continue;

                if (color_count[i] > color_count[first]) {
                    second = first;
                    first = i;
                }
                else if (color_count[i] > color_count[second]) {
                    second = i;
                }
            }

            // Ensure we have two distinct colors
            if (first == second) {
                second = (first + palette_size / 2) % palette_size;
            }

            // Remap pixels
            for (int y = 0; y < block_size && (by + y) < height; ++y) {
                for (int x = 0; x < block_size && (bx + x) < width; ++x) {
                    int idx = ((by + y) * width + (bx + x)) * channels;

                    // Compare to both colors and background
                    float dists[3] = {
                        color_distance(&image[idx], c64_palette[first].data()),
                        color_distance(&image[idx], c64_palette[second].data()),
                        c64_constraints ? color_distance(&image[idx], c64_palette[default_bg_index].data())
                                        : std::numeric_limits<float>::max()
                    };

                    int best = (dists[2] < dists[0] && dists[2] < dists[1]) ? default_bg_index :
                        (dists[0] < dists[1]) ? first : second;

                    // Apply color
                    const auto& color = c64_palette[best];
                    std::copy(color.begin(), color.end(), &image[idx]);
                }
            }
        }
    }
}