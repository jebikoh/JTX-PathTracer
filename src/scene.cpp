#include "scene.hpp"
#include "assimp/GltfMaterial.h"
#include "mesh.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <unordered_map>
#include "loader.hpp"

static constexpr int SCENE_MATERIAL_LIMIT = 64;
static const Vec3 GOLD_IOR                = {0.15557, 0.42415, 1.3831};
static const Vec3 GOLD_K                  = {-3.6024, -2.4721, -1.9155};

bool Scene::closestHit(const Ray &r, Interval t, SurfaceIntersection &record) const {
    const auto invDir     = 1 / r.dir;
    const int dirIsNeg[3] = {static_cast<int>(invDir.x < 0), static_cast<int>(invDir.y < 0), static_cast<int>(invDir.z < 0)};

    int toVisitOffset    = 0;
    int currentNodeIndex = 0;
    int stack[64];
    bool hitAnything = false;

    while (true) {
        const LinearBVHNode *node = &nodes_[currentNodeIndex];
        // 1. Check the ray intersects the current node
        //    If it doesn't, pop the stack and continue
        if (node->bbox.hit(r.origin, r.dir, t)) {
            // 2. If we are at a leaf node, loop through all primitives
            //    Otherwise, push the children onto the stack
            if (node->numPrimitives > 0) {
                // Leaf node
                for (int i = 0; i < node->numPrimitives; ++i) {
                    if (closestHitPrimitive(primitives_[node->primitivesOffset + i], r, t, record)) {
                        hitAnything = true;
                        t.max       = record.t;
                    }
                }
                if (toVisitOffset == 0) break;
                currentNodeIndex = stack[--toVisitOffset];
            } else {
                // Interior node
                if (dirIsNeg[node->axis]) {
                    stack[toVisitOffset++] = currentNodeIndex + 1;
                    currentNodeIndex       = node->secondChildOffset;
                } else {
                    stack[toVisitOffset++] = node->secondChildOffset;
                    currentNodeIndex       = currentNodeIndex + 1;
                }
            }
        } else {
            if (toVisitOffset == 0) break;
            currentNodeIndex = stack[--toVisitOffset];
        }
    }

    return hitAnything;
}

bool Scene::anyHit(const Ray &r, const Interval t) const {
    const auto invDir     = 1 / r.dir;
    const int dirIsNeg[3] = {static_cast<int>(invDir.x < 0), static_cast<int>(invDir.y < 0), static_cast<int>(invDir.z < 0)};

    int toVisitOffset    = 0;
    int currentNodeIndex = 0;
    int stack[64];

    while (true) {
        const LinearBVHNode *node = &nodes_[currentNodeIndex];
        if (node->bbox.hit(r.origin, r.dir, t)) {
            if (node->numPrimitives > 0) {
                for (int i = 0; i < node->numPrimitives; ++i) {
                    if (anyHitPrimitive(primitives_[node->primitivesOffset + i], r, t)) {
                        return true;
                    }
                }
                if (toVisitOffset == 0) break;
                currentNodeIndex = stack[--toVisitOffset];
            } else {
                // Interior node
                if (dirIsNeg[node->axis]) {
                    stack[toVisitOffset++] = currentNodeIndex + 1;
                    currentNodeIndex       = node->secondChildOffset;
                } else {
                    stack[toVisitOffset++] = node->secondChildOffset;
                    currentNodeIndex       = currentNodeIndex + 1;
                }
            }
        } else {
            if (toVisitOffset == 0) break;
            currentNodeIndex = stack[--toVisitOffset];
        }
    }

    return false;
}

void Scene::buildBVH(const int maxPrimsInNode) {
    maxPrimsInNode_ = maxPrimsInNode;
    primitives_.resize(numPrimitives());
    // std::vector<Primitive> primitives(scene.numPrimitives());

    // BVH Primitives is our working span of primitives
    // This will start out as all of them
    std::vector<Primitive> bvhPrimitives(primitives_.size());
    for (size_t i = 0; i < spheres.size(); ++i) {
        primitives_[i]   = Primitive{Primitive::SPHERE, i, spheres[i].bounds()};
        bvhPrimitives[i] = Primitive{Primitive::SPHERE, i, spheres[i].bounds()};
    }

    const size_t tOffset = spheres.size();
    for (size_t i = 0; i < triangles.size(); ++i) {
        primitives_[tOffset + i]   = Primitive{Primitive::TRIANGLE, i, meshes[triangles[i].meshIndex].tBounds(triangles[i].index)};
        bvhPrimitives[tOffset + i] = Primitive{Primitive::TRIANGLE, i, meshes[triangles[i].meshIndex].tBounds(triangles[i].index)};
    }
    // Add rest of types when we get them
    // We will order as we build
    std::vector<Primitive> orderedPrimitives(primitives_.size());

    int totalNodes             = 1;
    int orderedPrimitiveOffset = 0;

    const BVHNode *root = buildTree(bvhPrimitives, &totalNodes, &orderedPrimitiveOffset, orderedPrimitives, maxPrimsInNode_);
    primitives_.swap(orderedPrimitives);

    bvhPrimitives.resize(0);
    bvhPrimitives.shrink_to_fit();

    nodes_     = new LinearBVHNode[totalNodes];
    int offset = 0;
    flattenBVH(root, nodes_, &offset);

    // Clean-up the tree
    root->destroy();
    delete root;

    bvhBuilt_ = true;

    // Pre-process lights that need the scene radius
    const float sceneRadius = getSceneRadius();
    for (auto &light: lights) {
        if (light.type == Light::DISTANT) {
            light.sceneRadius = sceneRadius;
        }
    }
}

Scene createDefaultScene() {
    Scene scene;
    scene.name = "Default Scene";

    // Camera
    scene.cameraProperties.center        = Vec3(0, 1, 8);
    scene.cameraProperties.target        = Vec3(0, 0, -1);
    scene.cameraProperties.up            = Vec3(0, 1, 0);
    scene.cameraProperties.yfov          = 20;
    scene.cameraProperties.defocusAngle  = 0;
    scene.cameraProperties.focusDistance = 3.4;

    scene.skyColor = Vec3(0.7, 0.8, 1.0);

    scene.materials.reserve(10);
    scene.spheres.reserve(10);

    // Objects & Materials
    scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Vec3(0.659, 0.659, 0.749)});
    scene.spheres.emplace_back(Vec3(0, -100.5, -1), 100, &scene.materials.back());

    const Vec3 GOLD_IOR = {0.15557, 0.42415, 1.3831};
    const Vec3 GOLD_K   = {-3.6024, -2.4721, -1.9155};

    scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Vec3(0.1, 0.2, 0.5)});
    scene.spheres.emplace_back(Vec3(0, 0, -1.2), 0.5, &scene.materials.back());

    scene.materials.push_back({.type = Material::CONDUCTOR, .IOR = GOLD_IOR, .k = GOLD_K, .alphaX = 0.3, .alphaY = 0.3});
    scene.spheres.emplace_back(Vec3(1, 0, -1), 0.5, &scene.materials.back());

    scene.materials.push_back({.type = Material::DIELECTRIC, .IOR = Vec3(1.5 / 1.0)});
    scene.spheres.emplace_back(Vec3(-1, 0, -1), 0.5, &scene.materials.back());

    return scene;
}

Scene createMeshScene() {
    Scene scene;
    scene.name = "Mesh Scene";

    scene.cameraProperties.center        = Vec3(0, 0, 8);
    scene.cameraProperties.target        = Vec3(0, 0, -1);
    scene.cameraProperties.up            = Vec3(0, 1, 0);
    scene.cameraProperties.yfov          = 20;
    scene.cameraProperties.defocusAngle  = 0;
    scene.cameraProperties.focusDistance = 3.4;

    scene.skyColor = Vec3(0.7, 0.8, 1.0);

    scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Vec3(1, 0.3, 0.5)});

    const auto vertices = new Vec3[4];
    vertices[0]         = Vec3(-1, -1, -1);
    vertices[1]         = Vec3(-1, 1, -1);
    vertices[2]         = Vec3(1, 1, -1);
    vertices[3]         = Vec3(1, -1, -1);

    const auto indices = new Vec3i[2];
    indices[0]         = Vec3i(0, 1, 2);
    indices[1]         = Vec3i(0, 2, 3);

    const auto normals = new Vec3[4];
    normals[0]         = Vec3(0, 0, 1);
    normals[1]         = Vec3(0, 0, 1);
    normals[2]         = Vec3(0, 0, 1);
    normals[3]         = Vec3(0, 0, 1);

    scene.meshes.push_back({indices, 2, vertices, 4, normals, &scene.materials.back()});

    scene.triangles.push_back({0, 0});
    scene.triangles.push_back({1, 0});

    return scene;
}

Scene createScene(const std::string &path, const Mat4 &t, const Vec3 &background) {
    Scene scene;
    scene.name = "File scene";
    loadScene(path, scene);

    scene.cameraProperties.center        = Vec3(0, 0, 8);
    scene.cameraProperties.target        = Vec3(0, 0, 0);
    scene.cameraProperties.up            = Vec3(0, 1, 0);
    scene.cameraProperties.yfov          = 20;
    scene.cameraProperties.defocusAngle  = 0;
    scene.cameraProperties.focusDistance = 1;

    scene.skyColor = background;

    std::cout << "Scene loaded with:" << std::endl;
    std::cout << " - " << scene.meshes.size() << " meshes" << std::endl;
    std::cout << " - " << scene.triangles.size() << " triangles" << std::endl;

    int numVertices = 0;
    for (const auto &mesh: scene.meshes) {
        numVertices += mesh.numVertices;
    }

    std::cout << " - " << numVertices << " vertices" << std::endl;

    // Transform all verts and norms
    for (int i = 0; i < scene.meshes[0].numVertices; ++i) {
        scene.meshes[0].vertices[i] = t.applyToPoint(scene.meshes[0].vertices[i]);
        scene.meshes[0].normals[i]  = t.applyToNormal(scene.meshes[0].normals[i]);
    }

    return scene;
}

Scene createShaderBallScene(const bool highSubdivision) {
    const auto t = Mat4::identity();
    Scene scene;
    if (highSubdivision) {
        const std::string path = "assets/scenes/shaderball/shaderball_hsd.obj";
        scene                  = createScene(path, t);
    } else {
        const std::string path = "assets/scenes/shaderball/shaderball.obj";
        scene                  = createScene(path, t);
    }


    scene.cameraProperties.center = Vec3(2.5, 16, 12);
    scene.cameraProperties.target = Vec3(0, 3, 0);
    scene.cameraProperties.yfov   = 40;

    scene.skyColor = Vec3(0.7, 0.8, 1.0);

    scene.materials.push_back({.type = Material::CONDUCTOR, .IOR = GOLD_IOR, .k = GOLD_K, .alphaX = 0.05, .alphaY = 0.05});
    // scene.materials.push_back({.type = Material::DIELECTRIC, .IOR = Vec3(1.5), .alphaX = 0.01, .alphaY = 0.01, .texId = scene.meshes[3].material->texId});
    scene.meshes[3].material = &scene.materials.back();

    return scene;
}

Scene createShaderBallSceneWithLight(const bool highSubdivision) {

    const auto t = Mat4::identity();
    Scene scene;
    if (highSubdivision) {
        const std::string path = "assets/scenes/shaderball/shaderball_hsd.obj";
        scene                  = createScene(path, t);
    } else {
        const std::string path = "assets/scenes/shaderball/shaderball.obj";
        scene                  = createScene(path, t);
    }

    scene.cameraProperties.center = Vec3(2.5, 16, 12);
    scene.cameraProperties.target = Vec3(0, 3, 0);
    scene.cameraProperties.yfov   = 40;

    // scene.skyColor = Vec3(0.1, 0.1, 0.1);
    // scene.skyColor    = BLACK;
    scene.skyColor    = Color::SKY_BLUE;
    const Light point = {
            .type      = Light::POINT,
            .position  = Vec3(0, 20, 0),
            .intensity = Color::WHITE,
            .scale     = 1000};
    const Light distant = {
            .type      = Light::DISTANT,
            .position  = {0, -1, 0},
            .intensity = Color::WHITE,
            .scale     = 10,
    };
    // scene.lights.push_back(point);
    scene.lights.push_back(distant);

    // Base
    scene.materials.push_back({.type = Material::CONDUCTOR, .IOR = GOLD_IOR, .k = GOLD_K, .alphaX = 0.05, .alphaY = 0.05});
    // scene.materials.push_back({.type = Material::DIELECTRIC, .IOR = Vec3(1.5), .alphaX = 0.01, .alphaY = 0.01, .texId = scene.meshes[3].material->texId});
    scene.meshes[3].material = &scene.materials.back();

    return scene;
}

Scene createKnobScene() {
    const auto t           = Mat4::identity();
    const std::string path = "assets/scenes/knob.obj";
    auto scene             = createScene(path, t);

    scene.cameraProperties.center = Vec3(0, 3, 8);
    scene.cameraProperties.target = Vec3(0, 0, 0);
    scene.cameraProperties.yfov   = 15;

    scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Vec3(0.3, 0.3, 0.)});
    scene.meshes[0].material = &scene.materials.back();

    const Vec3 GOLD_IOR = {0.15557, 0.42415, 1.3831};
    const Vec3 GOLD_K   = {-3.6024, -2.4721, -1.9155};
    scene.materials.push_back({.type = Material::CONDUCTOR, .IOR = GOLD_IOR, .k = GOLD_K, .alphaX = 0.05, .alphaY = 0.05});
    scene.meshes[1].material = &scene.materials.back();
    scene.meshes[2].material = &scene.materials.back();

    scene.materials.push_back({.type = Material::DIELECTRIC, .IOR = Vec3(1.5), .alphaX = 0.3, .alphaY = 0.3});
    scene.meshes[3].material = &scene.materials.back();

    return scene;
}
