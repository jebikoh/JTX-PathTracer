#pragma once

#include "rt.hpp"


#include <jvk/buffer.hpp>
#include <jvk/context.hpp>
#include <jvk/descriptor.hpp>
#include <jvk/jvk.hpp>
#include <jvk/pipeline.hpp>
#include <jvk/util.hpp>

#include <engine/vulkan/camera.hpp>
#include <engine/vulkan/render_data.hpp>
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
    explicit VkEngine(const GfxContext &gfx)
        : m_gfx(gfx),
          m_ASManager(gfx) {}

    void Init(bool bEnableRayTracing = false);

    void Destroy();

    void Draw(RenderContext &ctx, ResolveRegion &region);

    void ProcessEvent(const SDL_Event &event);

    void SkipEvent();

    void SetViewportRectangle(const jvk::ViewRectangle &viewRectangle) { m_viewRectangle = viewRectangle; }

    void LoadScene(const Scene *pScene);

    void DrawSettingsPanel(UiDrawContext &ctx);

private:
    const GfxContext &m_gfx;
    jvk::ViewRectangle m_viewRectangle{};


    const Scene *m_pScene = nullptr;
    OrbitCamera m_camera{};

    // == State ==
    bool m_bSceneLoaded                    = false;
    bool m_bDrawGrid                       = true;
    bool m_bRayTracingAvailable            = false;
    bool m_bRayTracingEnabled              = false;
    bool m_bRayTracingEnabledPreviousFrame = false;

    // == Descriptor Sets ==
    void InitDescriptors();
    void DestroyDescriptors() const;

    // -- Bindless data --
    jvk::DescriptorAllocator m_bindlessAllocator;
    VkDescriptorSetLayout m_bindlessDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_bindlessDescriptorSet             = VK_NULL_HANDLE;

    // -- Global uniform data --
    jvk::DescriptorAllocator m_descriptorAllocator;
    struct FrameData {
        jvk::Buffer gpuGlobalUniformData;
        GPUGlobalUniformData *gpuGlobalUniformDataMapping = nullptr;
        VkDescriptorSet gpuGlobalUniformDataDescriptorSet = VK_NULL_HANDLE;
    } m_frameData[JTX_MAX_FRAMES_IN_FLIGHT];
    VkDescriptorSetLayout m_gpuGlobalUniformDataDescriptorLayout = VK_NULL_HANDLE;

    void UpdateGlobalUniformData();
    GPUGlobalUniformData m_gpuGlobalUniformData{};

    // == Pipelines ==
    void InitPipelines();
    void DestroyPipelines() const;

    // -- Material pipelines --
    void InitMaterialPipelines();
    void DestroyMaterialPipelines() const;
    struct MaterialPipelines {
        VkPipelineLayout layout = VK_NULL_HANDLE;

        VkPipeline diffuse = VK_NULL_HANDLE;
    } m_materialPipelines;

    // -- Grid pipeline --
    void InitGridPipeline();
    void DestroyGridPipeline() const;
    jvk::Pipeline m_gridPipeline;

    // -- RT pipelines --
    void InitRayTracingPipeline();
    void DestroyRayTracingPipeline() const;
    jvk::Pipeline m_rayTracingPipeline;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_rtShaderGroups;

    // == Scene data ==
    // LoadScene is public
    void DestroyScene();

    struct GPUSceneData {
        jvk::Buffer index{};
        jvk::Buffer position{};
        jvk::Buffer normal{};
        jvk::Buffer uv{};
        jvk::Buffer color{};

        VkDeviceAddress indexAddress    = 0;
        VkDeviceAddress positionAddress = 0;
        VkDeviceAddress normalAddress   = 0;
        VkDeviceAddress uvAddress       = 0;
        VkDeviceAddress colorAddress    = 0;

        std::vector<jvk::Image> textures;
        jvk::Buffer materialBuffer;
        jvk::Buffer objectBuffer;
    } m_gpuSceneData;

    // == Rasterization ==
    GPUDrawContext m_drawContext;
    void PopulateContext();
    void Rasterize(RenderContext &ctx, const VkRect2D &renderArea);

    // == Ray Tracing ==
    // -- Acceleration Structures --
    ASManager m_ASManager;
    void BuildBLAS();
    void BuildTLAS();

    struct ShaderBindingTable {
        jvk::Buffer buffer{};
        VkStridedDeviceAddressRegionKHR rayGenRegion{};
        VkStridedDeviceAddressRegionKHR missRegion{};
        VkStridedDeviceAddressRegionKHR hitRegion{};
        VkStridedDeviceAddressRegionKHR callableRegion{};
    } m_SBT;
    void InitRayTracingSBT();
    void DestroyRayTracingSBT();

    void RayTrace(RenderContext &ctx, const glm::vec4 &clearColor) const;

    uint32_t m_rtFrameNumber     = 0;
    uint32_t m_rtSamplesPerFrame = 16;
    uint32_t m_rtMaxFrames       = 128; // TODO: parameterize by samples

    // == Cache ==
    struct Cache {
        glm::mat4 view;
        glm::mat4 proj;
    } m_cache{};
};

}// namespace jtx
