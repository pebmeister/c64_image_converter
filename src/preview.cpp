#include "preview.h"

#ifdef USE_SFML
#include <SFML/Graphics.hpp>
#endif

#ifdef USE_SFML
void show_preview(const uint8_t* image, int width, int height) 
{
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
    auto sz = window.getSize();
    sz.x *= 5;
    sz.y *= 5;
    window.setSize(sz);
    window.setPosition({ 50,50 });
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
