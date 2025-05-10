#include <backends/cpu/bvh.hpp>
#include <backends/cpu/bvh_test.hpp>
#include <bitset>
#include <scene/scene_loader.hpp>
#include <util/simd.hpp>

int main(int argc, char *argv[]) {
    // Load scene
    jtx::Scene scene;
    jtx::loadScene("assets/f22.obj", scene);

    jtx::BVH4 bvh;
    bvh.build(scene);
    bvh.destroy();

    return 0;
}
