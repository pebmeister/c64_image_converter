#pragma once
#include <stdint.h>
#include <array>
#include <vector>
#include "pallet.h"

extern void convert_to_c64_hires(uint8_t* image, int width, int height, int bg_color = C64_BLACK);
extern void convert_to_c64_multicolor(uint8_t* image, int width, int height, int bg_color = C64_BLACK);

