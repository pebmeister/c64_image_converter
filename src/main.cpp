#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>
#include <string>
#include <limits>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "dither.h"
#include "pallet.h"
#include "scale.h"
#include "blockreducer.h"
#include "preview.h"
#include "c64converter.h"
#include "asmgenerator.h"  // New header for ASM generation

// STB headers
#include "stb_image.h"
#include "stb_image_write.h"

const int RGBChannels = 3;

typedef std::array<uint8_t, 3>Color;

Color getpixel(std::vector<uint8_t>& input_buffer, int width, int height, int x, int y)
{
    auto idx = (y * width + x) * 3;
    Color color{ input_buffer[idx], input_buffer[idx + 1], input_buffer[idx + 2] };
    return color;
}

void setpixel(std::vector<uint8_t>& output_buffer, int width, int height, int x, int y, Color color)
{
    int idx = (y * width + x) * 3;
    output_buffer[idx + 0] = color[0];
    output_buffer[idx + 1] = color[1];
    output_buffer[idx + 2] = color[2];
}


void copy_multi_color_bitmap(
    std::vector<uint8_t>& input_buffer, int input_width, int input_height,
    std::vector<uint8_t>& output_buffer, int output_width, int output_height)
{

    for (int y = 0; y < input_height; ++y) {
        for (int x = 0; x < input_width; ++x) {
            Color color = getpixel(input_buffer, input_width, input_height, x, y); // lookup from bitmap/ram

            setpixel(output_buffer, output_width, output_height, 2 * x + 0, y, color); // double-width
            setpixel(output_buffer, output_width, output_height, 2 * x + 1, y, color);
        }
    }
}

// Function to generate ASM filename based on output path
std::string get_filename(const std::string& output_path, const std::string& ext)
{
    size_t last_dot = output_path.find_last_of(".");
    if (last_dot == std::string::npos) {
        return output_path + ext;
    }
    return output_path.substr(0, last_dot) + ext;
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_image> <output_image> [options]\n"
            << "Options:\n"
            << "  --dither       Apply Floyd-Steinberg dithering\n"
            << "  --hires        Convert to C64 hires mode\n"
            << "  --multicolor   Convert to C64 multicolor mode\n"
            << "  --preview      Show SFML preview window\n"
            << "  --width N      Set output width\n"
            << "  --height N     Set output height\n"
            << "  --asm          Generate 6502 assembly file\n"
            << "Example: " << argv[0] << " input.png output.png --dither --multicolor --asm" << std::endl;
        return 1;
    }

    bool use_dithering = false, use_hires = false, use_multicolor = false;
    bool preview = false, generate_asm = false;

    int output_width = 320;
    int output_height = 200;
    
    // Parse command line arguments
    auto skipArg = false;
    for (auto arg = 3; arg < argc; ++arg) {
        if (skipArg) {
            skipArg = false;
            continue;
        }

        std::string arg_str = argv[arg];
        if (arg_str == "--dither") {
            use_dithering = true;
        }
        else if (arg_str == "--hires") {
            use_hires = true;
        }
        else if (arg_str == "--multicolor") {
            use_multicolor = true;
        }
        else if (arg_str == "--preview") {
            preview = true;
        }
        else if (arg_str == "--asm") {
            generate_asm = true;
        }
        else if (arg_str == "--width") {
            if (arg + 1 < argc) {
                output_width = std::stoi(argv[arg + 1]);
                skipArg = true;
            }
            else {
                std::cerr << "No value specified for width" << std::endl;
                return 1;
            }
        }
        else if (arg_str == "--height") {
            if (arg + 1 < argc) {
                output_height = std::stoi(argv[arg + 1]);
                skipArg = true;
            }
            else {
                std::cerr << "No value specified for height" << std::endl;
                return 1;
            }
        }
        else {
            std::cerr << "Unknown argument: " << arg_str << std::endl;
            return 1;
        }
    }

    // Validate mode selection
    if (use_hires && use_multicolor) {
        std::cerr << "Cannot specify both multicolor and hires" << std::endl;
        return 1;
    }
    if (!use_hires && !use_multicolor) {
        std::cerr << "Must specify either --hires or --multicolor" << std::endl;
        return 1;
    }

    // Load input image
    int width, height, channels;
    uint8_t* image = stbi_load(argv[1], &width, &height, &channels, RGBChannels);
    if (!image) {
        std::cerr << "Error loading image: " << argv[1] << "\n"
            << "Reason: " << stbi_failure_reason() << std::endl;
        return 1;
    }

    // Calculate target dimensions maintaining aspect ratio
    int target_width, target_height;
    float aspect = static_cast<float>(width) / height;

    if (aspect > (static_cast<float>(output_width) / output_height)) {
        target_width = output_width;
        target_height = static_cast<int>(output_width / aspect);
    }
    else {
        target_height = output_height;
        target_width = static_cast<int>(output_height * aspect);
    }

    // Scale image down
    std::vector<uint8_t> scaled_image(target_width * target_height * 3);

    if (use_multicolor) {
        auto multi_color_width = target_width / 2;
        std::vector<uint8_t> multi_color_scaled_image(multi_color_width * target_height * 3);
        scale_to_c64(image, width, height, multi_color_scaled_image.data(), multi_color_width, target_height, 3);
        scale_to_c64(multi_color_scaled_image.data(), multi_color_width, target_height, scaled_image.data(), target_width, target_height, 3);
    }
    else {
        scale_to_c64(image, width, height, scaled_image.data(), target_width, target_height, 3);
    }

    // Apply color conversion
    if (use_hires) {
        convert_to_c64_hires(scaled_image.data(), target_width, target_height);
    }
    else if (use_multicolor) {
        convert_to_c64_multicolor(scaled_image.data(), target_width, target_height);
    }

    // Apply dithering if requested
    if (use_dithering) {
        apply_floyd_steinberg(scaled_image.data(), target_width, target_height);
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

    // Generate ASM if requested
    if (generate_asm) {
        std::string asm_code = generate_6502_asm(argv[2]);
        std::string asm_filename = get_filename(argv[2], ".asm");
        std::ofstream asm_file(asm_filename);
        if (asm_file) {
            asm_file << asm_code;
            std::cout << "Assembly code generated: " << asm_filename << std::endl;

            C64ImageData c64_data = convert_to_c64_memory(scaled_image.data(), target_width, target_height, use_multicolor);
            std::string bitmap_filename = get_filename(argv[2], "image") + ".prg";
            std::ofstream bitmap_file(bitmap_filename, std::ios::binary);

            std::string color_filename = get_filename(argv[2], "color") + ".prg";
            std::ofstream color_file(color_filename, std::ios::binary);

            std::cout << "bitmap generated: " << bitmap_filename << std::endl;
            std::cout << "color generated: " << color_filename << std::endl;

            generate_6502_image(c64_data, bitmap_file, color_file);
        }
        else {
            std::cerr << "Failed to create ASM file: " << asm_filename << std::endl;
        }
    }

    // Save output image
    std::string output_path = argv[2];
    std::string extension = output_path.substr(output_path.find_last_of(".") + 1);

    bool save_result = false;
    if (extension == "png") {
        save_result = stbi_write_png(output_path.c_str(), target_width, target_height, RGBChannels,
            scaled_image.data(), target_width * RGBChannels);
    }
    else if (extension == "jpg" || extension == "jpeg") {
        save_result = stbi_write_jpg(output_path.c_str(), target_width, target_height, RGBChannels,
            scaled_image.data(), 90);
    }
    else if (extension == "bmp") {
        save_result = stbi_write_bmp(output_path.c_str(), target_width, target_height, RGBChannels,
            scaled_image.data());
    }
    else {
        std::cerr << "Unsupported output format. Using PNG." << std::endl;
        output_path += ".png";
        save_result = stbi_write_png(output_path.c_str(), target_width, target_height, RGBChannels,
            scaled_image.data(), target_width * RGBChannels);
    }

    if (!save_result) {
        std::cerr << "Failed to save output image" << std::endl;
    }

    // Show preview if enabled
    if (preview) {
#ifdef USE_SFML
        show_preview(scaled_image.data(), target_width, target_height);
#else
        std::cerr << "Preview not available - SFML support not compiled in" << std::endl;
#endif
    }

    // Cleanup
    stbi_image_free(image);

    if (save_result) {
        std::cout << "Successfully converted image to " << target_width << "x" << target_height
            << " with C64 colors.\nSaved to: " << output_path << std::endl;
        return 0;
    }
    else {
        return 1;
    }
}