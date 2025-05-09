#include <backends/cpu/bvh.hpp>
// #include <scene/scene_loader.hpp>
// #include <backends/cpu/bvh_test.hpp>

int main(int argc, char *argv[]) {
    // Load scene
    // jtx::Scene scene;
    // jtx::loadScene("assets/f22.obj", scene);
    //
    // auto result = jtx::validateBVH2(scene);
    // result.logResults();

    uint32_t a = 0b1111;
    uint32_t b = a << 27;

    uint32_t c = ~b;

    std::bitset<32> x(c);

    std::cout << x << std::endl;
    std::cout << a << std::endl;

    return 0;
}
