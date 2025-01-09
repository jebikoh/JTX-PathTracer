#pragma once
#include "engine.hpp"
#include "material.hpp"
#include "primitives.hpp"

// Very basic scene struct
// Will change if this starts running into performance issues

struct Scene {
    std::string name;

    std::vector<Material> materials;
    std::vector<Sphere> spheres;

    Camera::Properties cameraProperties;

    bool hit(const Primitive &primitive, const Ray &r, const Interval t, HitRecord &record) const {
        switch(primitive.type) {
            case Primitive::SPHERE: {
                return spheres[primitive.index].hit(r, t, record);
            }
            default:
                break;
        }

        return false;
    }

    [[nodiscard]]
    int numPrimitives() const {
        return spheres.size();
    }
};

class PrimitiveList {
public:
    explicit PrimitiveList(const Scene &scene) : scene_(scene) {
        for (size_t i = 0; i < scene.spheres.size(); ++i) {
            primitives_.push_back({Primitive::SPHERE, i, scene.spheres[i].bounds()});
        }
    }

    bool hit(const Ray &r, const Interval t, HitRecord &record) const {
        HitRecord tmpRecord{};
        bool hitAnything   = false;
        float closestSoFar = t.max;

        for (const auto &prim : primitives_) {
            switch(prim.type) {
                case Primitive::SPHERE: {
                    if (const auto &sphere = scene_.spheres[prim.index]; sphere.hit(r, Interval(t.min, closestSoFar), tmpRecord)) {
                        hitAnything  = true;
                        closestSoFar = tmpRecord.t;
                        record       = tmpRecord;
                    }
                    break;
                }
                case Primitive::RECTANGLE:
                    break;
                default:
                    break;
            }
        }

        return hitAnything;
    }
private:
    const Scene &scene_;
    std::vector<Primitive> primitives_;
};

Scene createDefaultScene(Scene &scene);
Scene createTestScene(Scene &scene);
Scene createCoverScene(Scene &scene);
