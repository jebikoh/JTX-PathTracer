#pragma once
#include "bvh.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "primitives.hpp"
#include "lights/lights.hpp"
#include "util/rand.hpp"

constexpr float RAY_EPSILON = 1e-4f;

// Very basic scene struct
// Will change if this starts running into performance issues
// 01/29: I've moved the BVH logic here. The BVH tree regularly needs to access
//        scene geometry, doing this will help eliminate a pointer dereference.
struct CameraProperties {
    Vec3 center;
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

    std::vector<Sphere> spheres;
    std::vector<Triangle> triangles;
    std::vector<Mesh> meshes;

    std::vector<TextureImage> textures;

    CameraProperties cameraProperties;

    void destroy() {
        for (auto &mesh : meshes) {
            mesh.destroy();
        }
        if (bvhBuilt_) destroyBVH();
    }

    bool closestHit(const Ray &r, Interval t, SurfaceIntersection &record) const;
    bool anyHit(const Ray &r, Interval t) const;

    [[nodiscard]]
    int numPrimitives() const {
        return spheres.size() + triangles.size();
    }

    void loadMesh(const std::string &path);

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
    bool closestHitPrimitive(const Primitive &primitive, const Ray &r, const Interval t, SurfaceIntersection &record) const {
        switch(primitive.type) {
            case Primitive::SPHERE: {
                return spheres[primitive.index].closestHit(r, t, record);
            }
            case Primitive::TRIANGLE: {
                const Triangle &triangle = triangles[primitive.index];
                float u, v;
                return meshes[triangle.meshIndex].tClosestHit(r, t, record, triangle.index, u, v);
            }
            default:
                break;
        }
        return false;
    }

    bool anyHitPrimitive(const Primitive &primitive, const Ray &r, const Interval t) const {
        switch(primitive.type) {
            case Primitive::SPHERE: {
                return spheres[primitive.index].anyHit(r, t);
            }
            case Primitive::TRIANGLE: {
                const Triangle &triangle = triangles[primitive.index];
                return meshes[triangle.meshIndex].tAnyHit(r, t, triangle.index);
            }
            default:
                break;
        }
        return false;
    }

    bool bvhBuilt_ = false;
    int maxPrimsInNode_ = 0;
    std::vector<Primitive> primitives_;
    LinearBVHNode *nodes_ = nullptr;
};

Scene createDefaultScene();
Scene createMeshScene();
Scene createObjScene(const std::string &path, const Mat4 &t, const Color &background = Color(0.7, 0.8, 1.0));
Scene createShaderBallScene(bool highSubdivision = false);
Scene createShaderBallSceneWithLight(bool highSubdivision = false);
Scene createKnobScene();