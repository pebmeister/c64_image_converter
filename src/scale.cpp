#include <algorithm>
#include "scale.h"

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
