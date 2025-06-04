#include <map>
#include <vector>
#include <array>
#include <algorithm>
#include <stdexcept>
#include <iostream>

#include "pallet.h"
#include "asmgenerator.h"

C64ImageData convert_to_c64_memory(uint8_t* image, int width, int height, bool multicolor)
{
    const int screen_bitmap_size = 8000;
    const int bytes_per_line = 320;
    const int color_ram_size = 1000;
    const int screenWidth = multicolor ? 160 : 320;
    const int screenHeight = 200;

    std::map<std::pair<int, int>, std::array<int, 3>> colordict;
    C64ImageData result;
 
    // center image on screen
    auto startX = 0; //  (screenWidth - width) / 2;
    auto startY = 0; //  (screenHeight - height) / 2;

    result.bitmap_data.resize(screen_bitmap_size);
    for (auto image_y = 0; image_y < height; image_y++) {
        auto y = startY + image_y;
        auto row = y / 8;
        auto line = y & 7;
        for (auto image_x = 0; image_x < width; ++image_x) {
            auto x = startX + image_x;
            auto ch = x / 8;
            auto bit = 7 - (x & 7);
            auto byte = row * bytes_per_line + ch * 8 + line;

            auto idx = (image_y * width + image_x) * 3;
            uint8_t color_idx = find_closest_color(&image[idx], c64_palette);
            if (color_idx < 0) {
                throw std::runtime_error("Color not in c64 pallette");
            }
            if (multicolor) {
                throw std::invalid_argument("Multi color not supported yet");
            }
            else {
                if (colordict.find({ row, ch }) == colordict.end()) {
                    colordict[{row, ch}] = { 0, 0, 0 };
                }
                auto& dict = colordict[{row, ch}];
                auto n = dict[0];
                switch (n) {
                    case 0:
                        // set as background color
                        // do not set bitmap
                        dict[0] = 1;
                        dict[1] = color_idx;
                        break;

                    case 1:
                        // the color is not the background color
                        // add foreground color and
                        // set bitmap
                        if (dict[1] != color_idx) {
                            dict[0] = 2;
                            dict[2] = color_idx;
                            result.bitmap_data[byte] |= 1 << bit;
                        }
                        break;

                    case 2:
                        if (dict[2] == color_idx) {
                            result.bitmap_data[byte] |= 1 << bit;
                        }
                        else if (dict[1] != color_idx) {
                            dict[0] &= 0xFF;
                            throw std::runtime_error(std::string("More than 2 colors in block " + std::to_string(row) + ", " + std::to_string(ch)));
                        }
                        break;

                    default:
                        throw std::runtime_error("internal error");
                        break;
                }
            }
        }
    }
 
    result.color_ram.resize(color_ram_size);
    auto b = 0;
    for (auto r = 0; r < 25; ++r) {
        for (auto c = 0; c < 40; ++c) {

            uint8_t c0 = 0;
            uint8_t c1 = 0;

            if (colordict.find({r, c}) != colordict.end()) {
                auto& cc = colordict[{r, c}];
                auto n = cc[0];
                if (n > 0)
                    c0 = (cc[1] & 0x0f);       // background
                if (n > 1)
                    c1 = (cc[2] & 0x0f) << 4;  // foreground
                if (n > 2)
                    throw std::runtime_error(std::string("More than 2 colors in block " + std::to_string(r) + ", " + std::to_string(c)));

            }
            result.color_ram[b] = c0 | c1;
            ++b;
        }
    }
    return result;
}

bool generate_6502_image(const C64ImageData& img, std::ofstream& bitmap, std::ofstream& color)
{
    auto bitmapLoadAddr = 0x2000;
    auto colorLoadAddr = 0x0400;

    bitmap << (unsigned char)(bitmapLoadAddr & 0xFF);
    bitmap << (unsigned char)((bitmapLoadAddr >> 8) & 0xFF);
    for (auto& byte : img.bitmap_data)
        bitmap << (unsigned char)byte;

    color << (unsigned char)(colorLoadAddr & 0xFF);
    color << (unsigned char)((colorLoadAddr >> 8) & 0xFF);
    for (auto& byte : img.color_ram)
        color << (unsigned char)byte;

    return true;
}

std::string generate_6502_asm(const std::string& fname)
{
    std::string tempname;
    size_t last_dot = fname.find_last_of(".");
    if (last_dot == std::string::npos) {
        tempname = fname;
    }
    else tempname = fname.substr(0, last_dot);
    std::string name;
    for (auto& ch : tempname)
        name += toupper(ch);

    std::ostringstream oss;
    oss <<
        "        MSGFLG = $009D\n" <<
        "        FA = $00BA\n" <<
        "        STROUT = $AB1E\n" <<
        "\n" <<
        "        CIAICR = $DC0D\n" <<
        "        CIACRA = $DC0E\n" <<
        "        RESTOR = $FF8A\n" <<
        "        SETLFS = $FFBA\n" <<
        "        SETNAM = $FFBD\n" <<
        "        LOAD = $FFD5\n" <<
        "         \n" <<
        "        .org $102               ; this MUST BE the autostart address\n" <<
        "\n" <<
        "        lda #$7F                ;   suppress any irq&nmi\n" <<
        "        sta CIAICR              ;   to disallow abort load by R/S\n" <<
        "        sta CIACRA\n" <<
        "        jsr RESTOR              ;   restore i/o-vex (just for sure...)\n" <<
        "         \n" <<
        "        lda $D018\n" <<
        "        ora #%00001000\n" <<
        "        sta $D018\n" <<
        "        lda $D011\n" <<
        "        ora #%00100000\n" <<
        "        sta $D011\n" <<
        "\n" <<
        "\n" <<
        "        ;;;;;;;;; load BITMAP\n" <<
        "        lda #NAMELEN\n" <<
        "        ldx #<NAME\n" <<
        "        ldy #>NAME\n" <<
        "        jsr SETNAM              ;   set name of file to load\n" <<
        "\n" <<
        "        lda FA                  ;   last device number - should be 8\n" <<
        "        tax\n" <<
        "        tay\n" <<
        "        jsr SETLFS              ;   open 8,8,8\n" <<
        "\n" <<
        "        lda #0                  ;   load... (lda #1 would verify)\n" <<
        "        sta MSGFLG              ;   flag progam mode (to suppress 'searching for...' msg)\n" <<
        "        jsr LOAD                ;   ...filename,8,8\n" <<
        "  \n" <<
        "\n" <<
        "        ;;;;;;;;; load COLOR\n" <<
        "        lda #CNAMELEN\n" <<
        "        ldx #<CNAME\n" <<
        "        ldy #>CNAME\n" <<
        "        jsr SETNAM              ;   set name of file to load\n" <<
        "\n" <<
        "        lda FA                  ;   last device number - should be 8\n" <<
        "        tax\n" <<
        "        tay\n" <<
        "        jsr SETLFS              ;   open 8,8,8\n" <<
        "\n" <<
        "        lda #0                  ;   load... (lda #1 would verify)\n" <<
        "        sta MSGFLG              ;   flag progam mode (to suppress 'searching for...' msg)\n" <<
        "        jsr LOAD                ;   ...filename,8,8\n" <<
        "\n" <<
        "        lda #$81                ;  restore any irq&nmi\n" <<
        "        sta CIAICR              ;\n" <<
        "        sta CIACRA\n" <<
        "\n" <<
        "LOOPFOREVER\n" <<
        "        jmp LOOPFOREVER         ; loop\n" <<
        "\n" <<
        "NAME    .str "; 
        
    oss <<
        "\"" << name << "IMAGE\",0        ;   enter name of prg-to-load here\n" <<
        "        NAMELEN = * - NAME - 1\n" <<
        "\n" <<
        "CNAME    .str ";    
    oss <<
        "\"" << name << "COLOR\",0        ;   enter name of prg-to-load here\n" <<
        "        CNAMELEN = * - CNAME - 1\n" <<
        "\n" <<
        "                                ;   now comes the smart part:\n" <<
        "        .fill $01, $200-*\n";

    return oss.str();
}
