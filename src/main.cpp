#include "rt.hpp"
#include "color.hpp"

int main() {
    constexpr int IMAGE_WIDTH  = 256;
    constexpr int IMAGE_HEIGHT = 256;

    for (int j = 0; j < IMAGE_HEIGHT; ++j) {
        for (int i = 0;i < IMAGE_WIDTH; ++i) {
            writeColor(std::cout,
                {static_cast<Float>(i) / (IMAGE_WIDTH - 1),
                static_cast<Float>(j) / (IMAGE_HEIGHT -1), 0.0});
        }
    }
}
