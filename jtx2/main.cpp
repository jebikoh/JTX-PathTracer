#include <backends/cpu/bvh.hpp>
#include <scene/scene_loader.hpp>
#include <backends/cpu/bvh_test.hpp>

int main(int argc, char *argv[]) {
    // Load scene
    jtx::Scene scene;
    jtx::loadScene("assets/f22.obj", scene);

    auto result = jtx::validateBVH2(scene);
    result.logResults();

    return 0;
}
