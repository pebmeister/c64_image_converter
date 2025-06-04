#pragma once
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>

struct C64ImageData {
    std::vector<uint8_t> color_ram;
    std::vector<uint8_t> bitmap_data;
    bool multicolor;
};

extern std::string generate_6502_asm(const std::string& name);
extern C64ImageData convert_to_c64_memory(uint8_t* image, int width, int height, bool multicolor);
extern bool generate_6502_image(const C64ImageData& img, std::ofstream& bitmap, std::ofstream& color);
