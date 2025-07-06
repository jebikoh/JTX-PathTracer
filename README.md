# JTX Path Tracer

**Update June 2025**:

I am currently in the process of rewriting the original project. The code for this can be found in JTX2.

My original version (for which everything below this point is for) was becoming too unwieldy to work with as it was very much a learning project. I also wanted to incorporate my Vulkan path tracer projects (and eventually, my CUDA OptiX projects) into one project.

Here a rough overview of what is currently implemented:
- Unidirectional (backwards) Monte-Carlo Path Tracing
- Multiple-Importance Sampling (MIS) with uniform light sampling
- Russian Roulette
- Diffuse, Dielectric, Conductor (Schlick), Complex Conductor BxDFs on the CPU
- Vulkan/SDL2 interactive display
- Forward rasterization pipeline for scene preview
- SSE4.2/NEON QBVH Trees: [Shallow Bounding Volume Hierarchies for Fast SIMD Ray Tracing of Incoherent Rays](https://www.uni-ulm.de/fileadmin/website_uni_ulm/iui.inst.100/institut/Papers/QBVH.pdf)
- Working hardware RT pipeline on compatible hardware

Here is what I am currently working on/researching:
 - Interactive progressive rending RT pipeline (Vulkan)
 - Ashikhmin-Shirley BRDF: [An anisotrphic phong BRDF model](https://www.researchgate.net/publication/2523875_An_anisotropic_phong_BRDF_model)
 - Layered BSDFs: [Position-free monte carlo simulation for arbitrary layered BSDFs](https://dl.acm.org/doi/10.1145/3272127.3275053#supplementary-materials)
 - d'Eon Diffuse BRDF: [An analytic BRDF for materials with spherical Lambertian scatterers](https://research.nvidia.com/publication/2021-06_analytic-brdf-materials-spherical-lambertian-scatterers)
 - Multiple Scattering:
     - [Multiple-scattering microfacet BSDFs with the Smith model](https://jo.dreggn.org/home/2016_microfacets.pdf)
     - [Practical multiple scattering compensation for microfacet models](https://blog.selfshadow.com/publications/turquin/ms_comp_final.pdf)
 - [Disney BSDF](https://blog.selfshadow.com/publications/s2015-shading-course/burley/s2015_pbs_disney_bsdf_slides.pdf)

Here is what I plan on integrating in the near future:
 - Single/multiple scattering homogeneous participating media
 - Radiance caching
 - BSSRDF

And the far future:
 - CPU bidirectional path tracing
 - [Vectorized Production Path Tracing](https://stg-research.dreamworks.com/wp-content/uploads/2018/07/Vectorized_Production_Path_Tracing_DWA_2017.pdf) on the CPU
 - Metropolis Light Transport
 - CUDA OptiX backend

![ajax](https://github.com/user-attachments/assets/6b44f17f-84d6-46f5-9214-547d9cb30931)

![knobs](https://github.com/user-attachments/assets/8299adf3-f817-4477-82d3-45d28a46ed80)

![knob](https://github.com/user-attachments/assets/b756c618-eca3-493a-a609-a1003155a4a6)

![knob](https://github.com/user-attachments/assets/86913d61-663d-4254-91e1-03062a4fb8b1)

---

This is my physically-based Monte Carlo path tracer, based on PBRTv4, built with multithreaded and vectorized C++ 20 and a interactive UI made with SDL2, OpenGL, and ImGui.

![image](https://github.com/user-attachments/assets/f6499dc1-893a-4249-bdaa-b36749b3c80f)

Old animation sample (incorrect lighting):

https://github.com/user-attachments/assets/4b746f1b-bf9e-4343-ab8d-3f99a952089c


## Features

**Sampling**:
- Unidirectional (backwards) Monte-Carlo Path Tracing
- Multiple-Importance Sampling (MIS) with uniform light sampling
- Path regularization (WIP)
- Russian Roulette (WIP)

**BxDF**:
- Diffuse, Dielectric, Conductor BxDFs
- glTF 2.0 PBR Metallic-Roughness BxDF (WIP)

**UI/UX**:
 - SDL2/OpenGL/ImGui interactive display
 - Scene and material editor
 - Debug views (normal, uv, depth)
 - Interactive camera (WIP)

**Engine**:
 - Multithreaded, vectorized CPU backend
 - Multithreaded LBVH construction
 - [SSE/NEON QBVH (4-wide) tree](https://github.com/jebikoh/simd-bvh) (WIP)
 - Optional CUDA/OptiX backend (WIP)

**Other**:
 - Texture support (albedo, roughness)
 - Basic animation (WIP)
 - Support for `.gltf`, `.glb`, and `.obj`
 - `.jtx` binary file format (WIP)
 - Command-line utility for final renders (WIP)

## Future Features:
**BxDF**:
 - [ ] Disney BxDF
 - [ ] Layered BxDF
 - [ ] Mediums

**Sampling**:
 - [ ] IBL

**CPU Optimizations**
 - [ ] Embree Integration
 - [ ] [Vectorized path tracing](https://www.tabellion.org/et/paper17/MoonRay.pdf)

**GPU**:
 - [ ] Vulkan backend

**UI**:
 - [ ] Interactive display

## Dependencies

JTX uses OpenGL and the following external libraries:
 - [fastgltf](https://github.com/spnda/fastgltf)
 - [glad](https://github.com/Dav1dde/glad)
 - [IconsLucide](https://github.com/juliettef/IconFontCppHeaders)
 - [imfilebrowser](https://github.com/AirGuanZ/imgui-filebrowser)
 - [ImGui](https://github.com/ocornut/imgui)
 - [JTXLib](https://github.com/jebikoh/jtxlib)
 - [rapidobj](https://github.com/guybrush77/rapidobj)
 - [SDL2](https://github.com/libsdl-org/SDL)
 - [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h)
 - [tinyexr](https://github.com/syoyo/tinyexr)

These are all vendored in the repository or via git submodules.

## Setup

To build the project, you will need to have the OpenGL installed (>3.3). All other dependencies are vendored in the repository or via git submodules. After cloning the repository, you can download the submodules with:

```bash
git submodule update --init --recursive
```

The project is built with CMake. Here are the available build options:
 - `JTX_ENABLE_CUDA_BACKEND`: enables and compiles the CUDA backend. Requires `nvcc`. OFF by default.
 - `JTX_ENABLE_PROFILING`: enables profiling tools. OFF by default.
 - `JTX_ENABLE_PERF_FLAGS`: applies compiler specific optimization flags. ON by default.
 - `JTX_ENABLE_MULTITHREADING`: enables multithreaded integration and BVH construction. ON by default.
 - `JTX_DISABLE_UI`: compile without the UI. Useful for avoiding UI overhead in final renders. OFF by default.
