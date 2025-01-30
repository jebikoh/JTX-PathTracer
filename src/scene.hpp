#pragma once
#include "bvh.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "primitives.hpp"
#include "lights/lights.hpp"
#include "util/rand.hpp"

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
    Color background;
};

class Scene {
public:
    std::string name;

    std::vector<Material> materials;

    std::vector<Light> lights;

    std::vector<Sphere> spheres;
    std::vector<Triangle> triangles;
    std::vector<Mesh> meshes;

    CameraProperties cameraProperties;

    void destroy() {
        for (auto &mesh : meshes) {
            mesh.destroy();
        }
        if (bvhBuilt_) destroyBVH();
    }

    bool hit(const Ray &r, Interval t, HitRecord &record) const;

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
        }
    }

    AABB bounds() const {
        return nodes_[0].bbox;
    }

    int sampleLightIdx(RNG &rng) const {
        const int idx = rng.sampleRange(lights.size());
        return idx;
    }

private:
    bool hitPrimitive(const Primitive &primitive, const Ray &r, const Interval t, HitRecord &record) const {
        switch(primitive.type) {
            case Primitive::SPHERE: {
                return spheres[primitive.index].hit(r, t, record);
            }
            case Primitive::TRIANGLE: {
                const Triangle &triangle = triangles[primitive.index];
                float u, v;
                return meshes[triangle.meshIndex].tHit(r, t, record, triangle.index, u, v);
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
Scene createTestScene();
Scene createCoverScene();
Scene createMeshScene();
Scene createObjScene(const std::string &path, const Mat4 &t, const Color &background = Color(0.7, 0.8, 1.0));
Scene createCornellBox();
Scene createF22Scene(bool isDielectric = false);
Scene createShaderBallScene();
Scene createShaderBallSceneWithLight();
Scene createKnobScene();