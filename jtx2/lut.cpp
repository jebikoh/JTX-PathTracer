#include <bvh/bvh.hpp>
#include <editor/editor.hpp>
#include <engine/cpu/backend_cpu.hpp>
#include <scene/scene_loader.hpp>
#include <thread>
#include <engine/cpu/energy_compensation.hpp>

using namespace jtx;

int main(int argc, char *argv[]) {
    Logger::AddDefaultSink();

    std::filesystem::path path = "ggx.lut";
    CHECK_JTX(GenerateGGXReflectionCompensationLUT(path, 419));

    return 0;
}
