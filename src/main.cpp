#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>
#include <string>
#include <limits>

// STB headers (implementation defines are now in CMake)
#include "stb_image.h"
#include "stb_image_write.h"

#ifdef USE_SFML
#include <SFML/Graphics.hpp>
#endif

// Commodore 64 color palette (RGB values)
const std::array<std::array<uint8_t, 3>, 16> c64_palette = {{
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
uint8_t find_closest_color(const uint8_t* color) {
    uint8_t closest = 0;
    float min_dist = std::numeric_limits<float>::max();
    
    for (uint8_t i = 0; i < c64_palette.size(); ++i) {
        float dist = color_distance(color, c64_palette[i].data());
        if (dist < min_dist) {
            min_dist = dist;
            closest = i;
        }
    }
    
    return closest;
}

// Apply Floyd-Steinberg dithering
void apply_dithering(uint8_t* image, int width, int height, int channels) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * channels;
            
            uint8_t palette_idx = find_closest_color(&image[idx]);
            int error_r = image[idx] - c64_palette[palette_idx][0];
            int error_g = image[idx+1] - c64_palette[palette_idx][1];
            int error_b = image[idx+2] - c64_palette[palette_idx][2];
            
            image[idx] = c64_palette[palette_idx][0];
            image[idx+1] = c64_palette[palette_idx][1];
            image[idx+2] = c64_palette[palette_idx][2];
            
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

// Scale image down to fit within C64 resolution while maintaining aspect ratio
void scale_to_c64(const uint8_t* input, int in_width, int in_height, 
                 uint8_t* output, int out_width, int out_height, int channels) {
    float scale_x = static_cast<float>(in_width) / out_width;
    float scale_y = static_cast<float>(in_height) / out_height;
    
    for (int y = 0; y < out_height; ++y) {
        for (int x = 0; x < out_width; ++x) {
            int src_x = static_cast<int>(x * scale_x);
            int src_y = static_cast<int>(y * scale_y);
            int src_idx = (src_y * in_width + src_x) * channels;
            int dst_idx = (y * out_width + x) * channels;
            
            std::copy_n(&input[src_idx], channels, &output[dst_idx]);
        }
    }
}

#ifdef USE_SFML
void show_preview(const uint8_t* image, int width, int height) {
    sf::RenderWindow window(sf::VideoMode(width, height), "C64 Image Preview");
    sf::Texture texture;
    texture.create(width, height);
    sf::Sprite sprite(texture);
    
    std::vector<uint8_t> pixels(width * height * 4);
    for (int i = 0; i < width * height; ++i) {
        pixels[i*4]   = image[i*3];     // R
        pixels[i*4+1] = image[i*3+1];   // G
        pixels[i*4+2] = image[i*3+2];   // B
        pixels[i*4+3] = 255;            // A
    }
    
    texture.update(pixels.data());
    
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        
        window.clear();
        window.draw(sprite);
        window.display();
    }
}
#else
void show_preview(const uint8_t* image, int width, int height) {
    std::cout << "Preview not available (SFML not enabled)" << std::endl;
}
#endif

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_image> <output_image> [--dither]\n"
                  << "Example: " << argv[0] << " input.png output.png --dither" << std::endl;
        return 1;
    }
    
    bool use_dithering = (argc > 3 && std::string(argv[3]) == "--dither");
    
    // Load input image (force 3 channels RGB)
    int width, height, channels;
    uint8_t* image = stbi_load(argv[1], &width, &height, &channels, 3);
    if (!image) {
        std::cerr << "Error loading image: " << argv[1] << "\n"
                  << "Reason: " << stbi_failure_reason() << std::endl;
        return 1;
    }
    
    // Determine target dimensions (maintain aspect ratio)
    const int c64_width = 320;
    const int c64_height = 200;
    
    int target_width, target_height;
    float aspect = static_cast<float>(width) / height;
    
    if (aspect > (static_cast<float>(c64_width) / c64_height)) {
        target_width = c64_width;
        target_height = static_cast<int>(c64_width / aspect);
    } else {
        target_height = c64_height;
        target_width = static_cast<int>(c64_height * aspect);
    }
    
    // Scale image down
    std::vector<uint8_t> scaled_image(target_width * target_height * 3);
    scale_to_c64(image, width, height, scaled_image.data(), target_width, target_height, 3);
    
    // Apply dithering if requested
    if (use_dithering) {
        apply_dithering(scaled_image.data(), target_width, target_height, 3);
    } else {
        // Simple color quantization
        for (size_t i = 0; i < scaled_image.size(); i += 3) {
            uint8_t palette_idx = find_closest_color(&scaled_image[i]);
            scaled_image[i]   = c64_palette[palette_idx][0];
            scaled_image[i+1] = c64_palette[palette_idx][1];
            scaled_image[i+2] = c64_palette[palette_idx][2];
        }
    }
    
    // Save output image
    std::string output_path = argv[2];
    std::string extension = output_path.substr(output_path.find_last_of(".") + 1);
    
    bool save_result = false;
    if (extension == "png") {
        save_result = stbi_write_png(output_path.c_str(), target_width, target_height, 3, 
                                   scaled_image.data(), target_width * 3);
    } else if (extension == "jpg" || extension == "jpeg") {
        save_result = stbi_write_jpg(output_path.c_str(), target_width, target_height, 3, 
                                   scaled_image.data(), 90);
    } else if (extension == "bmp") {
        save_result = stbi_write_bmp(output_path.c_str(), target_width, target_height, 3, 
                                   scaled_image.data());
    } else {
        std::cerr << "Unsupported output format. Using PNG." << std::endl;
        output_path += ".png";
        save_result = stbi_write_png(output_path.c_str(), target_width, target_height, 3, 
                                   scaled_image.data(), target_width * 3);
    }
    
    if (!save_result) {
        std::cerr << "Failed to save output image" << std::endl;
    }
    
    // Show preview if enabled
    #ifdef USE_SFML
    show_preview(scaled_image.data(), target_width, target_height);
    #endif
    
    // Cleanup
    stbi_image_free(image);
    
    if (save_result) {
        std::cout << "Successfully converted image to " << target_width << "x" << target_height 
                  << " with C64 colors.\nSaved to: " << output_path << std::endl;
        return 0;
    } else {
        return 1;
    }
}