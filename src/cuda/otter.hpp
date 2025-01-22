#pragma once

#include <jtxlib/math/vec3.hpp>
#include <jtxlib/math/mat4.hpp>
#include <jtxlib/math/vec2.hpp>
#include <jtxlib/jstd/jstd.hpp>
#include <jtxlib.hpp>

#define CHECK_CUDA(val) checkCudaError( (val), #val, __FILE__, __LINE__ )
void checkCudaError(cudaError_t result, char const *const func, const char *const file, int const line) {
    if (result) {
        std::cerr << "CUDA error = " << static_cast<unsigned int>(result) << " at " <<
                file << ":" << line << " '" << func << "' \n";
        // Make sure we call CUDA Device Reset before exiting
        cudaDeviceReset();
        exit(99);
    }
}

using Vec3  = jtx::Vec3f;
using Vec2f = jtx::Vec2f;
using Ray   = jtx::Rayf;

#ifdef __CUDA_ARCH__
constexpr float INF = cuda::std::numeric_limits<float>::max();
constexpr float PI  = 3.14159265358979323846f;
constexpr float INV_PI = 1 / PI;
#else
constexpr float INF = jtx::INFINITY_F;
constexpr float PI  = jtx::PI_F;
constexpr float INV_PI = 1 / jtx::PI_F;
#endif

#ifdef ENABLE_CUDA_BACKEND
#define HOSTDEV __host__ __device__
#define DEV __device__
#define HOST __host__
#define INLINE __forceinline__
#else
#define HOSTDEV
#define DEV
#define HOST
#define INLINE inline
#endif

using Vec2i = jtx::Vec2i;
using Vec3i = jtx::Vec3i;

using Mat4 = jtx::Mat4;