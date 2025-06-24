#include <bvh/bvh.hpp>
#include <engine/cpu/backend_cpu.hpp>
#include <interface/display.hpp>
#include <scene/scene_loader.hpp>
#include <thread>

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
    jtx::LoadScene("assets/ajax/ajax.obj", scene);
    // jtx::LoadScene("assets/cornell_box/small/cb_small.obj", scene);

    scene.skyColor = vec3(0.0f);
    scene.cameraSettings.position = vec3{278, 273, -800} * 0.2f;
    scene.cameraSettings.target = vec3{278, 273, 0} * 0.2f;
    scene.cameraSettings.up = {0, 1, 0};
    scene.cameraSettings.focalDistance = 10.0f;
    // scene.cameraSettings.focalLength = 0.028f; // 50mm
    scene.cameraSettings.focalLength = 0.05f;
    scene.cameraSettings.sensorWidth = 0.036f; // 36mm
    scene.cameraSettings.focalDistance = 200.0f;
    scene.cameraSettings.bEnableDof = true;
    scene.cameraSettings.fStop = 8.0f;

    // Specular gold material on short box
    // jtx::Material conductor;
    // conductor.mType = jtx::Material::COMPLEX_CONDUCTOR;
    // conductor.parameters = { // Perfectly specular
    //     .ior = vec3(0.18299,0.42108,1.37340),
    //     .k = vec3(3.42420,2.34590,1.77040),
    //     .roughness = vec2(0.1f, 0.1f),
    // };
    // const uint32_t materialIndex = scene.AddMaterial(conductor);
    // scene.UpdateMeshMaterial(6, materialIndex);

    // jtx::Material conductor;
    // conductor.mType = jtx::Material::CONDUCTOR;
    // conductor.parameters = {
    //     .f0 = vec3(0.384, 0.58, 1),
    //     .roughness = vec2(0.1f, 0.1f)
    // };
    // const uint32_t materialIndex = scene.AddMaterial(conductor);
    // scene.UpdateMeshMaterial(6, materialIndex);

    jtx::Material dielectric;
    dielectric.mType = jtx::Material::DIELECTRIC;
    dielectric.parameters = {
        .ior = vec3(1.5),
        .roughness = vec2(0.3, 0.3)
    };
    const uint32_t materialIndex = scene.AddMaterial(dielectric);
    scene.UpdateMeshMaterial(6, materialIndex);

    jtx::RenderSettings rs;
    rs.maxDepth = 32;
    rs.sppRow = 32;
    rs.sppCol = 32;
    rs.tileSize = 32;
    rs.numThreads = std::thread::hardware_concurrency() - 1;
    rs.seed = 419;
    // rs.samplesPerPass = 1;

    jtx::BackendCPU backend;
    backend.Init(400, 400, rs);
    backend.LoadScene(&scene);
    backend.StartOfflineRender();
    CHECK_JTX(backend.SaveRenderOutput("ajax_rough_dielectric.png"));
    backend.Destroy();
#endif

    return 0;
}
