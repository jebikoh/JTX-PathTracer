# JTX Path Tracer

**Update June 2025**:

(The previous version of the project can be found in the `main` branch under the `src/` directory)

Currently implemented:
- Unidirectional (backwards) Monte-Carlo Path Tracing
- Multiple-Importance Sampling (MIS) with uniform light sampling
- Russian Roulette
- Diffuse, Dielectric, Conductor (Schlick), Complex Conductor BxDFs on the CPU
- Vulkan/SDL2 interactive display
- Forward rasterization pipeline for scene preview
- SSE4.2/NEON QBVH Trees: [Shallow Bounding Volume Hierarchies for Fast SIMD Ray Tracing of Incoherent Rays](https://www.uni-ulm.de/fileadmin/website_uni_ulm/iui.inst.100/institut/Papers/QBVH.pdf)
- Interactive Vulkan RT pipeline

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

https://github.com/user-attachments/assets/69ecdfaf-d2df-4946-abe9-26715a2f2ede

![ajax](https://github.com/user-attachments/assets/6b44f17f-84d6-46f5-9214-547d9cb30931)

![knobs](https://github.com/user-attachments/assets/8299adf3-f817-4477-82d3-45d28a46ed80)

![knob](https://github.com/user-attachments/assets/b756c618-eca3-493a-a609-a1003155a4a6)

![knob](https://github.com/user-attachments/assets/86913d61-663d-4254-91e1-03062a4fb8b1)
