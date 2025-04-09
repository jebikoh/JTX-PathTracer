#include "scene.hpp"
#include "bsdf/conductor.hpp"
#include "loader.hpp"
#include "logger.hpp"
#include "mesh.hpp"

static const Vec3 GOLD_IOR = {0.15557, 0.42415, 1.3831};
static const Vec3 GOLD_K   = {-3.6024, -2.4721, -1.9155};

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
                    const Triangle &tri = primitives_[node->primitivesOffset + i];
                    float u, v;
                    if (meshes[tri.meshIndex].tClosestHit(r, t, record, tri.index, u, v)) {
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
                    const Triangle &tri = primitives_[node->primitivesOffset + i];
                    if (meshes[tri.meshIndex].tAnyHit(r, t, tri.index)) {
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
    LOG_INFO("Building BVH");
    maxPrimsInNode_ = maxPrimsInNode;
    primitives_.resize(triangles.size());

    std::vector<Triangle> bvhPrimitives(triangles.size());
    for (size_t i = 0; i < triangles.size(); ++i) {
        primitives_[i] = Triangle{triangles[i].index, triangles[i].meshIndex, meshes[triangles[i].meshIndex].tBounds(triangles[i].index)};
    }
    std::vector<Triangle> orderedPrimitives(primitives_.size());

    int totalNodes             = 1;
    int orderedPrimitiveOffset = 0;

    const BVHNode *root = buildTree(primitives_, &totalNodes, &orderedPrimitiveOffset, orderedPrimitives, maxPrimsInNode);
    primitives_.swap(orderedPrimitives);

    nodes_     = new LinearBVHNode[totalNodes];
    int offset = 0;
    LOG_INFO("Flattening BVH");
    flattenBVH(root, nodes_, &offset);
    LOG_INFO("Finished flattening BVH");

    // Clean-up the tree
    root->destroy();
    delete root;

    bvhBuilt_ = true;
    LOG_INFO("Finished building BVH");

    // Pre-process lights that need the scene radius
    LOG_INFO("Pre-processing lights");
    const float sceneRadius = getSceneRadius();
    for (auto &light: lights) {
        if (light.type == Light::DISTANT) {
            light.sceneRadius = sceneRadius;
        }
    }
    LOG_INFO("Lights processed");
}

Scene createScene(const std::string &path, const Mat4 &t, const Vec3 &background) {
    Scene scene;
    scene.name = "File scene";
    loadScene(path, scene);

    scene.cameraProperties.position      = Vec3(0, 0, 8);
    scene.cameraProperties.target        = Vec3(0, 0, 0);
    scene.cameraProperties.up            = Vec3(0, 1, 0);
    scene.cameraProperties.yfov          = 20;
    scene.cameraProperties.defocusAngle  = 0;
    scene.cameraProperties.focusDistance = 1;

    scene.skyColor = background;

    // Transform all verts and norms
    for (int i = 0; i < scene.meshes[0].vertices.size(); ++i) {
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


    scene.cameraProperties.position = Vec3(2.5, 16, 12);
    scene.cameraProperties.target   = Vec3(0, 3, 0);
    scene.cameraProperties.yfov     = 40;

    scene.skyColor = Vec3(0.7, 0.8, 1.0);

    scene.materials.push_back({.mType      = Material::CONDUCTOR,
                               .parameters = {
                                       .ior    = GOLD_IOR,
                                       .k      = GOLD_K,
                                       .alphaY = 0.05,
                                       .alphaX = 0.05}});
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

    scene.cameraProperties.position = Vec3(2.5, 16, 12);
    scene.cameraProperties.target   = Vec3(0, 3, 0);
    scene.cameraProperties.yfov     = 40;

    // scene.skyColor = Vec3(0.1, 0.1, 0.1);
    // scene.skyColor    = BLACK;
    scene.skyColor    = Color::SKY_BLUE;
    const Light point = {
            .type      = Light::POINT,
            .position  = Vec3(0, 20, 0),
            .intensity = Color::WHITE,
            .scale     = 1000};
    // const Light distant = {
    //         .type      = Light::DISTANT,
    //         .position  = {0, -1, 0},
    //         .intensity = Color::WHITE,
    //         .scale     = 10,
    // };
    scene.lights.push_back(point);
    // scene.lights.push_back(distant);

    // Base
    // scene.materials.push_back({.type = Material::CONDUCTOR, .IOR = GOLD_IOR, .k = GOLD_K, .alphaX = 0.05, .alphaY = 0.05});

    // Material matDielectric{};
    // matDielectric.mType = Material::DIELECTRIC;
    // matDielectric.parameters.ior = Vec3(1.5);
    // matDielectric.parameters.alphaX = 0.01;
    // matDielectric.parameters.alphaY = 0.01;
    // matDielectric.textureIndices.albedo = -1;

    Material matSS{};
    matSS.mType                 = Material::CONDUCTOR;
    matSS.parameters            = JTX_BXDF_PRESET_STAINLESS_STEEL;
    matSS.parameters.alphaX     = 0.01;
    matSS.parameters.alphaY     = 0.01;
    matSS.textureIndices.albedo = -1;

    scene.materials.push_back(matSS);
    scene.meshes[3].material = &scene.materials.back();

    return scene;
}

Scene createKnobScene() {
    const auto t           = Mat4::identity();
    const std::string path = "assets/scenes/knob.obj";
    auto scene             = createScene(path, t);

    scene.cameraProperties.position = Vec3(0, 3, 8);
    scene.cameraProperties.target   = Vec3(0, 0, 0);
    scene.cameraProperties.yfov     = 15;

    scene.materials.push_back({
            .mType      = Material::DIFFUSE,
            .parameters = {.albedo = Vec3(0.3, 0.3, 0.0)},
    });
    scene.meshes[0].material = &scene.materials.back();

    scene.materials.push_back({.mType      = Material::CONDUCTOR,
                               .parameters = {
                                       .ior    = GOLD_IOR,
                                       .k      = GOLD_K,
                                       .alphaY = 0.05,
                                       .alphaX = 0.05,
                               }});
    scene.meshes[1].material = &scene.materials.back();
    scene.meshes[2].material = &scene.materials.back();

    scene.materials.push_back({.mType      = Material::DIELECTRIC,
                               .parameters = {
                                       .ior    = Vec3(1.5),
                                       .alphaY = 0.3,
                                       .alphaX = 0.3,
                               }});
    scene.meshes[3].material = &scene.materials.back();

    return scene;
}
