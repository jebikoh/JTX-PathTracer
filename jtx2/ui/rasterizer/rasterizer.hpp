#pragma once

#include "camera.hpp"
#include "fastgltf/types.hpp"
#include "ui/jvk/buffer.hpp"
#include "ui/jvk/descriptor.hpp"
#include "ui/jvk/jvk.hpp"
#include "ui/jvk/pipeline.hpp"
#include "ui/jvk/util.hpp"
#include "ui/rasterizer/render_data.hpp"

#include <ui/jvk/context.hpp>

namespace jtx {

class Display;

/**
 * This class is responsible for rasterizing the scene, including managing
 * GPU resources, pipelines, and rendering commands.
 *
 * GPU data overview:
 *
 * Push Constants (per-object):
 *  - world matrix (mat4)
 *  - normal matrix (mat4)
 * Layout 0, Binding 0: Scene-Data (per-frame):
 *  - view matrix (mat4)
 *  - proj matrix (mat4)
 *  - view proj matrix (mat4)
 *  - camera position (vec4)
 *  - vertex buffer address (VkDeviceAddress)
 *  - normal buffer address (VkDeviceAddress)
 *  - UV buffer address (VkDeviceAddress)
 *  - color buffer address (VkDeviceAddress)
 * Layout 1, Binding 0: Material Data UBO (per-object)
 *  - ambient value (vec4)
 *  - diffuse value (vec4)
 *  - specular value (vec4)
 *  - shininess value (float)
 * Layout 1, Binding 1: Ambient image/sampler (per-object)
 * Layout 1, Binding 2: Diffuse image/sampler (per-object)
 * Layout 1, Binding 3: Specular image/sampler (per-object)
 */
class Rasterizer {
public:
    explicit Rasterizer(Display *pDisplay)
        : m_pDisplay(pDisplay) {}

    void init();

    void destroy();

    void draw(VkCommandBuffer cmd);

    void processSDLEvent(const SDL_Event &event);

    void skipEvent();

    void setViewportRectangle(const jvk::ViewRectangle &viewRectangle) { m_viewRectangle = viewRectangle; }

    /**
     * This will (re)load the scene from the display and setup GPU resources.
     *
     * The things that occur when load a new scene are as follows:
     *  - Previously loaded scene's data will be cleared (mesh buffers, material buffers, textures, etc.)
     *  - New GPU buffers for index, position, normal, uv, and color data will be created
     *  - Data is uploaded via a staging buffer
     *  - Textures are uploaded
     *  - A new UBO is created to hold material data
     *  - Material instances are written for each material and constants are written to UBO
     */
    void loadScene();

private:
    // It might be worth to store some commonly used members (as copies or pointers) within this class
    // during initialization to avoid some pointer chasing
    Display *m_pDisplay = nullptr;
    jvk::Context m_ctx;
    VmaAllocator m_allocator{};

    jvk::ViewRectangle m_viewRectangle{};

    // Scene
    bool m_bSceneLoaded = false;
    // FPSCamera m_camera{.position = {0.0f, 0.0f, 5.0f}, .speed = 0.1};
    OrbitCamera m_camera{};

    // Frame data (scene UBO buffers)
    struct FrameData {
        jvk::Buffer gpuSceneDataUBO;
        GPUSceneUBOData *gpuSceneDataUBOMapping;
        VkDescriptorSet gpuSceneDataUboDescriptorSet;
    } m_frameData[JTX_MAX_FRAMES_IN_FLIGHT];
    VkDescriptorSetLayout m_gpuSceneDataUboDescriptorLayout = VK_NULL_HANDLE;
    GPUSceneUBOData m_gpuSceneUboData;

    void initFrameData();
    void destroyFrameData() const;

    // GPU scene mesh data buffers
    struct GPUSceneMeshData {
        jvk::Buffer index{};
        jvk::Buffer position{};
        jvk::Buffer normal{};
        jvk::Buffer uv{};
        jvk::Buffer color{};

        VkDeviceAddress positionAddress;
        VkDeviceAddress normalAddress;
        VkDeviceAddress uvAddress;
        VkDeviceAddress colorAddress;
    } m_gpuSceneMeshData;
    void destroyGPUSceneMeshData();

    // GPU material pipelines and descriptor layouts
    struct GPUMaterials {
        jvk::Pipeline opaquePipeline;
        jvk::Pipeline transparentPipeline;
        VkDescriptorSetLayout descriptorSetLayout;
    } m_gpuMaterials;

    // A scene's specific material data
    std::vector<GPUMaterialInstance> m_gpuMaterialInstances;
    jvk::Buffer m_materialBufferUBO;

    // Scene's loaded textures
    std::vector<jvk::Image> m_sceneTextures;

    /**
     * Builds material pipelines and descriptor set layouts
     */
    void initMaterialResources();

    /**
     * Destroys material pipelines and descriptor layouts
     */
    void destroyMaterialResources() const;

    /**
     * Destroys all material instances and the material UBO if a scene is loaded
     */
    void destroyGPUSceneMaterials() const;

    /**
     * Destroys all loaded textures on the GPU
     */
    void destroyGPUSceneTextures() const;

    /**
     * Utility function to write a single material instance given a pass type and resources.
     * @param pass material pass type
     * @param resources material resources
     * @return material instance
     */
    GPUMaterialInstance writeMaterial(GPUMaterialPass pass, const GPUMaterialResources &resources);

    // Draw
    GPUDrawContext m_drawContext;

    void populateContext();
    void updateSceneData();

    jvk::DescriptorWriter m_descriptorWriter;

    void destroyGPUScene();
};

}// namespace jtx
