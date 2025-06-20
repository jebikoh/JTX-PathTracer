#include <bvh/bvh.hpp>
#include <engine/cpu/backend_cpu.hpp>
#include <interface/display.hpp>
#include <scene/scene_loader.hpp>

// #define JTX_ENABLE_UI

int main(int argc, char *argv[]) {
    Logger::AddDefaultSink();
#ifdef JTX_ENABLE_UI
    jtx::Scene scene;
    CHECK_JTX(jtx::LoadScene("assets/duck/duck.obj", scene));

    jtx::Display display;
    display.Init();
    display.SetScene(&scene);
    display.Run();
    display.Destroy();
#else
    jtx::Scene scene;
    jtx::LoadScene("assets/cornell_box/small/cb_small.obj", scene);

    scene.skyColor = vec3(0.0f);
    scene.cameraSettings.position = vec3{278, 273, -800} * 0.2f;
    scene.cameraSettings.target = vec3{278, 273, 0} * 0.2f;
    scene.cameraSettings.up = {0, 1, 0};
    scene.cameraSettings.yfov = 40.0f;
    scene.cameraSettings.focalDistance = 1.0f;

    // Specular gold material on short box
    // jtx::Material conductor;
    // conductor.mType = jtx::Material::COMPLEX_CONDUCTOR;
    // conductor.parameters = {
    //     .ior = vec3(0.18299,0.42108,1.37340),
    //     .k = vec3(3.42420,2.34590,1.77040),
    // };
    // jtx::Material conductor;
    // conductor.mType = jtx::Material::CONDUCTOR;
    // conductor.parameters = {
    //     .f0 = vec3(0.384, 0.58, 1)
    // };
    //
    // const uint32_t materialIndex = scene.AddMaterial(conductor);
    // scene.UpdateMeshMaterial(7, materialIndex);

    jtx::RenderSettings rs;
    rs.maxDepth = 32;
    rs.sppRow = 16;
    rs.sppCol = 16;
    rs.tileSize = 32;
    rs.numThreads = 16;
    rs.seed = 419;

    jtx::BackendCPU backend;
    backend.Init(400, 400, rs);
    backend.LoadScene(&scene);
    backend.StartOfflineRender();
    CHECK_JTX(backend.SaveRenderOutput("nee_400x400_256spp.png"));
    backend.Destroy();

    // const float mse = jtx::CalculateMSE("../renders/diffuse-cornell-box/nee_400x400_256spp.png",
    //                   "../renders/diffuse-cornell-box/ref_400x400_16384spp.png");
    // if (mse > 0.0f) {
    //     LOG_INFO(GENERAL, "MSE: {:.6f}", mse);
    // }
#endif

    return 0;
}
