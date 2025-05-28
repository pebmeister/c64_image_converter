#include <algorithm>

#include "dither.h"
#include "pallet.h"

void apply_floyd_steinberg(uint8_t* image, int width, int height)
{
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * 3;
            uint8_t old_r = image[idx];
            uint8_t old_g = image[idx + 1];
            uint8_t old_b = image[idx + 2];

            uint8_t new_r = (old_r > 128) ? 255 : 0;
            uint8_t new_g = (old_g > 128) ? 255 : 0;
            uint8_t new_b = (old_b > 128) ? 255 : 0;

            image[idx] = new_r;
            image[idx + 1] = new_g;
            image[idx + 2] = new_b;

            int err_r = old_r - new_r;
            int err_g = old_g - new_g;
            int err_b = old_b - new_b;

            // Distribute error
            if (x + 1 < width) {
                for (int c = 0; c < 3; c++) {
                    image[idx + 3 + c] = std::clamp(
                        image[idx + 3 + c] + err_r * 7 / 16, 0, 255);
                }
            }
            if (y + 1 < height) {
                if (x > 0) {
                    for (int c = 0; c < 3; c++) {
                        image[idx + width * 3 - 3 + c] = std::clamp(
                            image[idx + width * 3 - 3 + c] + err_r * 3 / 16, 0, 255);
                    }
                }
                for (int c = 0; c < 3; c++) {
                    image[idx + width * 3 + c] = std::clamp(
                        image[idx + width * 3 + c] + err_r * 5 / 16, 0, 255);
                }
                if (x + 1 < width) {
                    for (int c = 0; c < 3; c++) {
                        image[idx + width * 3 + 3 + c] = std::clamp(
                            image[idx + width * 3 + 3 + c] + err_r * 1 / 16, 0, 255);
                    }
                }
            }
        }
    }
}

// Apply Floyd-Steinberg dithering
void apply_dithering(uint8_t* image, int width, int height, int channels, 
    const std::vector<std::array<uint8_t, 3>>& palette)
{
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * channels;

            uint8_t palette_idx = find_closest_color(&image[idx], palette);
            int error_r = image[idx] - palette[palette_idx][0];
            int error_g = image[idx + 1] - palette[palette_idx][1];
            int error_b = image[idx + 2] - palette[palette_idx][2];

            image[idx] = palette[palette_idx][0];
            image[idx + 1] = palette[palette_idx][1];
            image[idx + 2] = palette[palette_idx][2];

            // Distribute error to neighboring pixels
            if (x + 1 < width) {
                for (int c = 0; c < 3; ++c) {
                    int neighbor_idx = idx + channels + c;
                    image[neighbor_idx] = std::clamp(image[neighbor_idx] + error_r * 7 / 16, 0, 255);
                }
            }
            if (y + 1 < height) {
                if (x > 0) {
                    for (int c = 0; c < 3; ++c) {
                        int neighbor_idx = idx + (width - 1) * channels + c;
                        image[neighbor_idx] = std::clamp(image[neighbor_idx] + error_r * 3 / 16, 0, 255);
                    }
                }
                for (int c = 0; c < 3; ++c) {
                    int neighbor_idx = idx + width * channels + c;
                    image[neighbor_idx] = std::clamp(image[neighbor_idx] + error_r * 5 / 16, 0, 255);
                }
                if (x + 1 < width) {
                    for (int c = 0; c < 3; ++c) {
                        int neighbor_idx = idx + (width + 1) * channels + c;
                        image[neighbor_idx] = std::clamp(image[neighbor_idx] + error_r * 1 / 16, 0, 255);
                    }
                }
            }
        }
    }
}
