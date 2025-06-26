#pragma once

#include "rt.hpp"


#include <jvk/buffer.hpp>
#include <jvk/context.hpp>
#include <jvk/descriptor.hpp>
#include <jvk/jvk.hpp>
#include <jvk/pipeline.hpp>
#include <jvk/util.hpp>

#include <engine/wing/camera.hpp>
#include <engine/wing/render_data.hpp>
#include <interface/gfx_context.hpp>

#include <fastgltf/types.hpp>


namespace jtx {
struct UiDrawContext;
struct Scene;

/**
 * GPU data overview:
 * Layout 0, Binding 0: Global scene data (UBO)
 *  - view proj matrix (mat4)
 *  - inverse view proj matrix (mat4)
 *  - camera position (vec4)
 *  - sun direction (vec3)
 *  - sun intensity (float)
 * Layout 1, Binding 0: Bindless Object Data (SSBO) []
 *  - world matrix (mat4)
 *  - normal matrix (mat4)
 *  - material handle (uint32_t)
 *  - geometry handle (uint32_t)
 * Layout 1, Binding 1: Bindless Material Data (SSBO) []
 *  - diffuse color (vec4)
 *  - ior (vec4)
 *  - k (vec4)
 *  - f0 (vec4)
 *  - emission (vec4)
 *  - roughness (vec4)
 *  - diffuse texture handle (int32_t)
 * Layout 1, Binding 2: Bindless Texture Sampler Array (Combined Image Sampler) []
 * Layout 1, Binding 3: Bindless Geometry Data (SSBO) []
 *  - vertex buffer address (VkDeviceAddress)
 *  - normal buffer address (VkDeviceAddress)
 *  - uv buffer address (VkDeviceAddress)
 *  - color buffer address (VkDeviceAddress)
 *  - index buffer address (VkDeviceAddress)
 */
class VkEngine {
public:
    explicit VkEngine(const GfxContext &gfx) : m_gfx(gfx), m_ASManager(gfx) {}

    void Init(bool bEnableRayTracing = false);

    void Destroy();

    void Draw(RenderContext &ctx, ResolveRegion &region);

    void ProcessEvent(const SDL_Event &event);

    void SkipEvent();

    void SetViewportRectangle(const jvk::ViewRectangle &viewRectangle) { m_viewRectangle = viewRectangle; }

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
    void LoadScene(const Scene *pScene);

    void DrawSettingsPanel(UiDrawContext &ctx);
private:
    const GfxContext &m_gfx;
    jvk::DynamicDescriptorAllocator m_descriptorAllocator;

    jvk::ViewRectangle m_viewRectangle{};

    // Scene
    bool m_bSceneLoaded = false;
    const Scene *m_pScene = nullptr;

    OrbitCamera m_camera{};

    void InitDescriptorSets();

    // Frame data
    struct FrameData {
        jvk::Buffer gpuSceneDataUBO;
        GPUSceneUBOData *gpuSceneDataUBOMapping = nullptr;
        VkDescriptorSet gpuSceneDataUboDescriptorSet = VK_NULL_HANDLE;
    } m_frameData[JTX_MAX_FRAMES_IN_FLIGHT];
    VkDescriptorSetLayout m_gpuSceneDataUboDescriptorLayout = VK_NULL_HANDLE;
    GPUSceneUBOData m_gpuSceneUboData{};

    void InitFrameSceneData();
    void DestroyFrameSceneData() const;

    // GPU scene mesh data buffers
    struct GPUSceneMeshData {
        jvk::Buffer index{};
        jvk::Buffer position{};
        jvk::Buffer normal{};
        jvk::Buffer uv{};
        jvk::Buffer color{};

        VkDeviceAddress indexAddress = 0;
        VkDeviceAddress positionAddress = 0;
        VkDeviceAddress normalAddress = 0;
        VkDeviceAddress uvAddress = 0;
        VkDeviceAddress colorAddress = 0;
    } m_gpuSceneMeshData;

    // GPU material pipelines and descriptor layouts
    struct GPUMaterials {
        jvk::Pipeline opaquePipeline;
        jvk::Pipeline transparentPipeline;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    } m_gpuMaterials;

    // A scene's specific material data
    std::vector<GPUMaterialInstance> m_gpuMaterialInstances;
    jvk::Buffer m_materialBufferUBO;

    // Scene's loaded textures
    std::vector<jvk::Image> m_sceneTextures;

    /**
     * Builds material pipelines and descriptor set layouts
     */
    void InitMaterialResources();

    /**
     * Destroys material pipelines and descriptor layouts
     */
    void DestroyMaterialResources() const;

    /**
     * Utility function to write a single material instance given a pass type and resources.
     * @param pass material pass type
     * @param resources material resources
     * @return material instance
     */
    GPUMaterialInstance WriteMaterial(GPUMaterialPass pass, const GPUMaterialResources &resources);

    // Draw
    GPUDrawContext m_drawContext;
    void Rasterize(RenderContext &ctx, const VkRect2D &renderArea);

    void PopulateContext();
    void UpdateGlobalUBOData();

    jvk::DescriptorWriter m_descriptorWriter;

    void DestroyGPUScene();

    // Grid
    bool m_bDrawGrid = true;
    void InitGridPipeline();
    void DestroyGridPipeline() const;
    jvk::Pipeline m_gridPipeline;

    // Billboards
    bool m_bDrawBillboards = false;
    void InitBillboardPipeline();
    void DestroyBillboardPipeline();

    // Ray tracing
    bool m_bRayTracingAvailable = false;
    bool m_bRayTracingEnabled   = false;
    ASManager m_ASManager;
    void BuildBLAS();
    void BuildTLAS();

    void InitRayTracingDescriptors();
    void DestroyRayTracingDescriptors() const;
    VkDescriptorSetLayout m_rtDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_rtDescriptorSet = VK_NULL_HANDLE;

    void InitRayTracingPipeline();
    void DestroyRayTracingPipeline() const;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_rtShaderGroups;
    VkPipelineLayout m_rtPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_rtPipeline = VK_NULL_HANDLE;

    void InitRayTracingSBT();
    void DestroyRayTracingSBT() const;
    jvk::Buffer m_rtSBTBuffer;
    VkStridedDeviceAddressRegionKHR m_rayGenRegion{};
    VkStridedDeviceAddressRegionKHR m_missRegion{};
    VkStridedDeviceAddressRegionKHR m_hitRegion{};
    VkStridedDeviceAddressRegionKHR m_callableRegion{};

    void RayTrace(RenderContext &ctx, const glm::vec4 &clearColor) const;
};

}// namespace jtx
