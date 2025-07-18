# JTX Path Tracer


(The previous version of the project can be found in the `main` branch under the `src/` directory)

## Overview

Currently implemented:
- Unidirectional (backwards) Monte-Carlo Path Tracing
- Multiple-Importance Sampling (MIS) with uniform light sampling
- Russian Roulette
- Diffuse, Dielectric, Conductor (Schlick), Complex Conductor BxDFs
- HDRI Envmap sampling
- Physically based camera
- Post-processing: manual/camera-based exposure, tonemapping (ACES, AgX, Hable)
- Vulkan/SDL2 interactive display
  - Forward rasterization pipeline for scene preview
  - Vulkan RT pipeline for realtime path tracing
- Offline Vulkan RT pipeline
- SSE4.2/NEON QBVH Trees: [Shallow Bounding Volume Hierarchies for Fast SIMD Ray Tracing of Incoherent Rays](https://www.uni-ulm.de/fileadmin/website_uni_ulm/iui.inst.100/institut/Papers/QBVH.pdf)
- Custom scene descriptor `.jtx`

Currently working on/researching:
 - Ashikhmin-Shirley BRDF: [An anisotrphic phong BRDF model](https://www.researchgate.net/publication/2523875_An_anisotropic_phong_BRDF_model)
 - Layered BSDFs: [Position-free monte carlo simulation for arbitrary layered BSDFs](https://dl.acm.org/doi/10.1145/3272127.3275053#supplementary-materials)
 - d'Eon Diffuse BRDF: [An analytic BRDF for materials with spherical Lambertian scatterers](https://research.nvidia.com/publication/2021-06_analytic-brdf-materials-spherical-lambertian-scatterers)
 - Multiple Scattering:
     - [Multiple-scattering microfacet BSDFs with the Smith model](https://jo.dreggn.org/home/2016_microfacets.pdf)
     - [Practical multiple scattering compensation for microfacet models](https://blog.selfshadow.com/publications/turquin/ms_comp_final.pdf)
 - [Disney BSDF](https://blog.selfshadow.com/publications/s2015-shading-course/burley/s2015_pbs_disney_bsdf_slides.pdf)

Future features:
 - Single/multiple scattering homogeneous participating media
 - Radiance caching
 - BSSRDF

Far future features:
 - CPU bidirectional path tracing
 - [Vectorized Production Path Tracing](https://stg-research.dreamworks.com/wp-content/uploads/2018/07/Vectorized_Production_Path_Tracing_DWA_2017.pdf) on the CPU
 - Metropolis Light Transport
 - CUDA OptiX backend

## Gallery

https://github.com/user-attachments/assets/75a9206b-2e38-4193-a30f-23a2879dcdbb

![ajax](https://github.com/user-attachments/assets/6b44f17f-84d6-46f5-9214-547d9cb30931)

![knobs](https://github.com/user-attachments/assets/8299adf3-f817-4477-82d3-45d28a46ed80)

<img width="1920" height="1080" alt="knob_e" src="https://github.com/user-attachments/assets/1c310f08-4890-4c2a-9531-9cd26da1ffdc" />

![knob](https://github.com/user-attachments/assets/86913d61-663d-4254-91e1-03062a4fb8b1)

## Setup

### Prerequisites

The following tools/dependencies must be installed:
 - CMake: Version 3.28 or newer.
 - C++20 Compiler: A compiler that supports the C++20 standard (project developed with MSVC and Apple Clang).
 - Vulkan SDK: 1.2 or newer. Ensure the VULKAN_SDK environment variable is set.
 - Slang Compiler: Comes with Vulkan 1.3.296.0 or newer, or can be downloaded separately from the [Slang GitHub](https://github.com/shader-slang/slang).

Optional dependencies
 - Embree: 4.0 or newer
   - Windows: included as a vendored dependency. No action required.
   - OSX/Linux: Install via package manager or build from source.
   - If you don't want to use Embree, set `JTX_USE_EMBREE` to `OFF` in the CMake configuration step.

Hardware requirements:
 - Minimum, a GPU that supports Vulkan 1.2+ and the following extensions:
   - `VK_KHR_SYNCHRONIZATION_2`
   - `VK_KHR_DYNAMIC_RENDERING`
   - `VK_KHR_COPY_COMMANDS_2`
 - To utilize hardware ray tracing, GPU that supports the following extensions is required:
   - `VK_KHR_ACCELERATION_STRUCTURE`
   - `VK_KHR_RAY_TRACING_PIPELINE`
   - `VK_KHR_DEFERRED_HOST_OPERATIONS`
 - For CPU, a processor with support for SSE4.2 or NEON is required for reasonable render times.

### Building

1. Clone the repository with the `--recursive` flag to include submodules:
   ```bash
   git clone --recursive https://github.com/jebikoh/JTX-PathTracer.git
   cd JTX-PathTracer
   ```
2. Create build directory and run CMake:
    ```bash
    cmake -S . -B build
    ```
3. Build the project:
    ```bash
    cmake --build build --config Release
    ```
    After a successful build, you can find the editor executable, `JTXEditor`, and CLI, `JTX`, inside the `build/bin/` directory.

### Configuration Options

You can customize the build using several CMake options. Pass them during the configuration step with the `-D` flag. For example: `cmake -S . -B build -DJTX_BUILD_TESTS=ON`.

|Option|Description|Default|
|---|---|---|
|`JTX_ENABLE_PROFILING`|Enables profiling code via compile definitions.|`ON`|
|`JTX_ENABLE_PERF_FLAGS`|Adds compiler optimization flags (`-O3` for GCC/Clang, `/O2` for MSVC).|`ON`|
|`JTX_ENABLE_MULTI_THREADING`|Enables multi-threading for the CPU backend.|`ON`|
|`JTX_ENABLE_SIMD`|Enables SIMD (SSE4.2/NEON) intrinsics for the BVH.|`ON`|
|`JTX_USE_EMBREE`|Builds with Embree for ray tracing acceleration structures.|`ON`|
|`JTX_BUILD_TESTS`|Builds the GTest unit tests.|`OFF`|
|`JTX_BUILD_BENCHMARKS`|Builds the Google Benchmark performance tests.|`OFF`|


## Third-Party Libraries

A list of all the third-party libraries used in this project:
 - [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/): Graphics API
 - [Volk](https://github.com/zeux/volk): Vulkan loader and extension management library
 - [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator): Vulkan Memory Allocator
 - [VkBootstrap](https://github.com/charles-lunarg/vk-bootstrap): Vulkan initialization helper library
 - [SDL3](https://github.com/libsdl-org/SDL/tree/release-3.2.x): Cross-platform windowing and input handling
 - [Dear ImGui](https://github.com/ocornut/imgui): UI library
 - [Native File Dialog Extended](https://github.com/btzy/nativefiledialog-extended): Cross-platform file dialog library
 - [stb_image](https://github.com/nothings/stb): Image loading library
 - [tinyEXR](https://github.com/syoyo/tinyexr): EXR image loading library
 - [rapidjson](https://github.com/Tencent/rapidjson/): Fast JSON parsing (for `.jtx` files)
 - [rapidobj](https://github.com/guybrush77/rapidobj): Wavefront OBJ file loading library
 - [fastgltf](https://github.com/spnda/fastgltf): GLTF file loading library
 - [JTXLib](https://github.com/jebikoh/jtxlib): Custom math library
 - [GLM](https://github.com/g-truc/glm): OpenGL Mathematics library
 - [IconFontCppHeaders](https://github.com/juliettef/IconFontCppHeaders): Icon font headers for Dear ImGui
 - [Embree](https://www.embree.org/): High-performance ray tracing kernels (optional, used for CPU backend)
