#pragma once
#include "bvh.hpp"
#include "lights/lights.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "primitives.hpp"
#include "util/rand.hpp"
#include "image.hpp"


constexpr float RAY_EPSILON = 1e-4f;

struct CameraProperties {
    Vec3 position;
    Vec3 target;
    Vec3 up;
    Float yfov;
    Float defocusAngle;
    Float focusDistance;
};

class Scene {
public:
    std::string name;

    std::vector<Material> materials;

    std::vector<Light> lights;
    Vec3 skyColor;

    std::vector<Triangle> triangles;
    std::vector<Mesh> meshes;

    std::vector<TextureImage> textures;

    CameraProperties cameraProperties;

    void destroy() {
        if (bvhBuilt_) destroyBVH();
    }

    bool closestHit(const Ray &r, Interval t, SurfaceIntersection &record) const;
    bool anyHit(const Ray &r, Interval t) const;

    [[nodiscard]]
    int numPrimitives() const {
        return triangles.size();
    }

    void buildBVH(int maxPrimsInNode = 1);

    void destroyBVH() {
        if (bvhBuilt_) {
            delete[] nodes_;
            nodes_ = nullptr;
            bvhBuilt_ = false;
            primitives_.clear();
        }
    }

    void rebuildBVH(const int maxPrimsInNode = 1) {
        destroyBVH();
        buildBVH(maxPrimsInNode);
    }

    AABB bounds() const {
        if (!bvhBuilt_) return AABB();
        return nodes_[0].bbox;
    }

    int sampleLightIdx(RNG &rng) const {
        const int idx = rng.sampleRange(lights.size());
        return idx;
    }

    float getSceneRadius() const {
        if (!bvhBuilt_) return 0;
        return bounds().diagonal().len() / 2;
    }

private:
    bool bvhBuilt_ = false;
    int maxPrimsInNode_ = 0;
    std::vector<Triangle> primitives_;
    LinearBVHNode *nodes_ = nullptr;
};

Scene createScene(const std::string &path, const Mat4 &t, const Vec3 &background = Vec3(0.7, 0.8, 1.0));
Scene createShaderBallScene(bool highSubdivision = false);
Scene createShaderBallSceneWithLight(bool highSubdivision = false);
Scene createKnobScene();