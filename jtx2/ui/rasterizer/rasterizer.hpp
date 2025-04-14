#pragma once

#include "ui/jvk/buffer.hpp"
#include "ui/jvk/descriptor.hpp"
#include "ui/jvk/jvk.hpp"
#include "ui/jvk/pipeline.hpp"
#include "ui/rasterizer/render_data.hpp"

namespace jtx {

class Display;

/**
 * This class is responsible for rasterizing the scene, including managing
 * GPU resources, pipelines, and rendering commands.
 *
 * A quick overview of the GPU data. We pass data through push constants and
 * two UBOs:
 *  - Push Constants: per-object data
 *      - World matrix (ma4)
 *      - Normal matrix (mat4)
 *  - UBO (layout 0, binding 0): scene data
 *      - View matrix (mat4)
 *      - Proj matrix (mat4)
 *      - View * Proj matrix (mat4)
 *      - Camera position (vec4)
 *      - Vertex buffer address (uint64_t)
 *      - Normal buffer address (uint64_t)
 *      - UV buffer address (uint64_t)
 *      - Color buffer address (uint64_t)
 *  - UBO (layout 1, binding 0): material data
*       - Ambient value (vec4)
 *      - Diffuse value (vec4)
 *      - Specular value (vec4)
 *      - Shininess value (float)
 *  - Image/Sampler (layout 1, binding 1-3): material textures
 *      - Ambient texture/sampler
 *      - Diffuse texture/sampler
 *      - Specular texture/sampler
 *
 * Push constants are modified per-object, per-frame
 *
 * The first layout is initialized in init() and is updated at the start
 * of every frame.
 *
 * The second layout is initialized in loadScene() and is only updated
 * if a material has been updated.
 */
class Rasterizer {
public:
    Rasterizer(Display *pDisplay)
        : m_pDisplay(pDisplay) {}

    void init();

    void destroy();

    void draw(VkCommandBuffer cmd);

    /**
     * This will (re)load the scene from the display and setup GPU resources.
     *
     * The things that occur when load a new scene are as follows:
     *  - Previously loaded scene's data will be cleared (mesh buffers, material buffers, textures, etc)
     *  - New GPU buffers for index, position, normal, uv, and color data will be created
     *  - Data is uploaded via a staging buffer
     *  - Textures are uploaded
     *  - A new UBO is created to hold material data
     *  - Material instances are written for each material and constants are written to UBO
     */
    void loadScene();

private:
    Display *m_pDisplay = nullptr;

    // Scene
    bool m_bSceneLoaded = false;

    // The rasterizer also needs to keep track of some per-frame data
    struct FrameData {
        jvk::Buffer sceneDataBuffer;
        VkDescriptorSet sceneDataDescriptorSet;
    } m_frameData[JTX_MAX_FRAMES_IN_FLIGHT];

    VkDescriptorSetLayout m_sceneDataDescriptorSetLayout = VK_NULL_HANDLE;
    void initFrameData();
    void destroyFrameData() const;

    struct GPUSceneBuffers {
        jvk::Buffer index;
        jvk::Buffer position;
        jvk::Buffer normal;
        jvk::Buffer uv;
        jvk::Buffer color;

        VkDeviceAddress positionAddress;
        VkDeviceAddress normalAddress;
        VkDeviceAddress uvAddress;
        VkDeviceAddress colorAddress;
    } m_gpuSceneBuffers;

    void destroyGPUSceneData();

    // Materials
    struct GPUMaterials {
        jvk::Pipeline opaquePipeline;
        jvk::Pipeline transparentPipeline;
        VkDescriptorSetLayout descriptorSetLayout;
    } m_gpuMaterials;

    jvk::DescriptorWriter writer;
    std::vector<GPUMaterialInstance> m_materialInstances;
    jvk::Buffer m_materialBufferUBO;

    // Draw
    GPUDrawContext m_drawContext;
    GPUDrawSceneData m_sceneData;

    void updateContext();

    /**
     * Builds material pipelines and descriptor set layouts
     */
    void initMaterialResources();

    /**
     * Destroys material pipelines and descriptor layouts
     */
    void destroyMaterialResources() const;

    /**
     * Utility function to write a single material instance given a pass type and resources.
     * @param pass material pass type
     * @param resources material resources
     * @param descriptorAllocator descriptor allocator to use
     * @return material instance
     */
    GPUMaterialInstance writeMaterial(GPUMaterialPass pass, const GPUMaterialResources &resources);
};

}// namespace jtx
