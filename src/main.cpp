#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>
#include <string>
#include <limits>

#include "dither.h"
#include "pallet.h"
#include "scale.h"
#include "blockreducer.h"
#include "preview.h"

// STB headers (implementation defines are now in CMake)
#include "stb_image.h"
#include "stb_image_write.h"

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

    bool use_block_reduction = (argc > 3 && std::string(argv[3]) == "--hires");
    bool use_block_multi_color_reduction = (argc > 3 && std::string(argv[3]) == "--multicolor");

    if (use_block_reduction) {
        reduce_colors_per_block(scaled_image.data(), target_width, target_height, 3);
    }
    if (use_block_multi_color_reduction) {
        reduce_colors_per_multicolor_block(scaled_image.data(), target_width, target_height, 3);
    }
    else if (use_dithering) {
        apply_dithering(scaled_image.data(), target_width, target_height, 3, c64_palette);
    }
    else {
        // Simple color quantization
        for (size_t i = 0; i < scaled_image.size(); i += 3) {
            uint8_t palette_idx = find_closest_color(&scaled_image[i], c64_palette);
            scaled_image[i] = c64_palette[palette_idx][0];
            scaled_image[i + 1] = c64_palette[palette_idx][1];
            scaled_image[i + 2] = c64_palette[palette_idx][2];
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
        std::cout << "Successfully converted image to " << target_width << "x" << target_height  <<
            " with C64 colors.\nSaved to: " << output_path << std::endl;
        return 0;
    } else {
        return 1;
    }
}
