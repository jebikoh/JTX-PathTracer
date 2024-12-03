#include "color.hpp"
#include "rt.hpp"

Color rayColor(const Ray &r) {
    const Float a = 0.5 * (r.dir.y + 1.0);
    return jtx::lerp(Color(1.0, 1.0, 1.0), Color(0.5, 0.7, 1.0), a);
}

int main() {
    // ReSharper disable once CppTooWideScope
    constexpr int IMAGE_WIDTH    = 1280;
    constexpr int IMAGE_HEIGHT   = 720;
    constexpr Float ASPECT_RATIO = static_cast<Float>(IMAGE_WIDTH) / static_cast<Float>(IMAGE_HEIGHT);

    // Viewport
    constexpr Float viewportHeight = 2.0;
    constexpr Float viewportWidth  = viewportHeight * ASPECT_RATIO;
    const auto cameraCenter        = Vec3(0, 0, 0);
    const Float focalLength        = 1.0;

    // Viewport offsets
    const auto u = Vec3(viewportWidth, 0, 0);
    const auto v = Vec3(0, -viewportHeight, 0);
    auto du      = u / IMAGE_WIDTH;
    auto dv      = v / IMAGE_HEIGHT;

    // Viewport anchors
    const auto vpUpperLeft = cameraCenter - Vec3(0, 0, focalLength) - u / 2 - v / 2;
    const auto vp00        = vpUpperLeft + 0.5 * (du + dv);

    for (int j = 0; j < IMAGE_HEIGHT; ++j) {
        for (int i = 0; i < IMAGE_WIDTH; ++i) {
            auto center = vp00 + (i * du) + (j * dv);
            auto rayDirection = (center - cameraCenter).normalize();
            Ray r(cameraCenter, rayDirection);

            Color pixelColor = rayColor(r);
            writeColor(std::cout, pixelColor);
        }
    }
}
