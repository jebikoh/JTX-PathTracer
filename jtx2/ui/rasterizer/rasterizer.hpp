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
 */
class Rasterizer {
public:
    Rasterizer(Display *pDisplay)
        : m_pDisplay(pDisplay) {}

    void init();

    void destroy();

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

    VkDescriptorSet m_sceneDataDescriptorSet             = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_sceneDataDescriptorSetLayout = VK_NULL_HANDLE;
    void initSceneDataDescriptorSetLayout();
    void destroySceneDataDescriptorSetLayout() const;

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

    void populateContext();

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
