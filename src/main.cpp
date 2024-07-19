#include <iostream>
#include <jtxlib/math/vec3.hpp>
#include "color.hpp"


int main() {
    const int image_width = 256;
    const int image_height = 256;

    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = 0; j < image_height; ++j) {
        std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
        for (int i = 0; i < image_width; ++i) {
            auto color = Color(double(i)/(image_width-1), double(j)/(image_height-1), 0);
            write_color(std::cout, color);
        }
    }

    std::clog << "\rDone.                 \n";
}
