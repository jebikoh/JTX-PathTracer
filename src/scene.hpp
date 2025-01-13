#pragma once
#include "camera.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "primitives.hpp"

// Very basic scene struct
// Will change if this starts running into performance issues

struct Scene {
    std::string name;

    std::vector<Material> materials;

    std::vector<Sphere> spheres;

    std::vector<Triangle> triangles;
    std::vector<Mesh> meshes;

    Camera::Properties cameraProperties;

    void destroy() const {
        for (auto &mesh : meshes) {
            mesh.destroy();
        }
    }

    bool hit(const Primitive &primitive, const Ray &r, const Interval t, HitRecord &record) const {
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

    [[nodiscard]]
    int numPrimitives() const {
        return spheres.size() + triangles.size();
    }

    void loadMesh(const std::string &path);
};

void createDefaultScene(Scene &scene);
void createTestScene(Scene &scene);
void createCoverScene(Scene &scene);
void createMeshScene(Scene &scene);
void createObjScene(Scene &scene, std::string &path, const Mat4 &t, const Material &material, const Color &background = Color(0.7, 0.8, 1.0));