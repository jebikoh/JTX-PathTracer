#define TINYOBJLOADER_IMPLEMENTATION
#include "scene.hpp"

#include "mesh.hpp"
#include "tiny_obj_loader.h"

static constexpr int SCENE_MATERIAL_LIMIT = 64;

static Material DEFAULT_MAT = {.type = Material::DIFFUSE, .albedo = Color(1, 0.3, 0.5)};

bool Scene::closestHit(const Ray &r, Interval t, HitRecord &record) const {
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

bool Scene::anyHit(const Ray &r, Interval t) const {
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

void Scene::loadMesh(const std::string &path) {
    if (materials.capacity() < SCENE_MATERIAL_LIMIT) {
        materials.reserve(SCENE_MATERIAL_LIMIT);
    }
    materials.push_back(DEFAULT_MAT);

    const tinyobj::ObjReaderConfig reader_config;
    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(path, reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader Error: " << reader.Error() << std::endl;
        }
        return;
    }

    if (!reader.Warning().empty()) {
        std::cerr << "TinyObjReader Warning: " << reader.Warning() << std::endl;
    }

    const auto &attrib = reader.GetAttrib();
    const auto &shapes = reader.GetShapes();

    for (size_t s = 0; s < shapes.size(); s++) {
        const auto &meshIndices = shapes[s].mesh.indices;
        size_t numIndices       = meshIndices.size();

        if (numIndices % 3 != 0) {
            std::cerr << "Warning: shape " << s << " has a face that isn't a triangle.\n";
            continue;
        }

        // Prepare CPU-side arrays (vectors first for ease)
        std::vector<Vec3> shapeVerts(numIndices);
        std::vector<Vec3> shapeNormals(numIndices);
        std::vector<Vec3i> shapeTriIndices(numIndices / 3);

        // Fill vertices and normals
        // We'll store each index_t as a unique entry in shapeVerts / shapeNormals
        for (size_t i = 0; i < numIndices; i++) {
            const tinyobj::index_t &idx = meshIndices[i];

            const float vx = attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 0];
            const float vy = attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 1];
            const float vz = attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 2];

            shapeVerts[i] = Vec3(vx, vy, vz);

            // If a normal index is available, read it; otherwise set a default normal
            if (idx.normal_index >= 0) {
                float nx        = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 0];
                float ny        = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 1];
                float nz        = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 2];
                shapeNormals[i] = Vec3(nx, ny, nz);
            } else {
                shapeNormals[i] = Vec3(0.0f, 1.0f, 0.0f);// fallback normal
            }
        }

        // Build the index array, grouping by 3 for each triangle
        for (size_t f = 0; f < numIndices / 3; f++) {
            shapeTriIndices[f] = Vec3i(
                    static_cast<int>(3 * f + 0),
                    static_cast<int>(3 * f + 1),
                    static_cast<int>(3 * f + 2));
        }

        // Allocate dynamic arrays for Mesh constructor
        auto *finalVerts   = new Vec3[numIndices];
        auto *finalNormals = new Vec3[numIndices];
        auto *finalIndices = new Vec3i[numIndices / 3];

        // Copy data from std::vector to raw arrays
        std::memcpy(finalVerts, shapeVerts.data(), numIndices * sizeof(Vec3));
        std::memcpy(finalNormals, shapeNormals.data(), numIndices * sizeof(Vec3));
        std::memcpy(finalIndices, shapeTriIndices.data(), (numIndices / 3) * sizeof(Vec3i));

        std::string mName = shapes[s].name;

        // Create the Mesh and store it
        // Use the default material we pushed earlier: materials.back()
        meshes.emplace_back(mName, finalIndices, shapeTriIndices.size(), finalVerts, shapeVerts.size(), finalNormals, &materials.back());

        // Now register all triangles from this mesh in the scene
        const int meshIndex     = static_cast<int>(meshes.size()) - 1;
        const int triangleCount = static_cast<int>(numIndices / 3);

        for (int t = 0; t < triangleCount; t++) {
            Triangle tri;
            tri.index     = t;
            tri.meshIndex = meshIndex;
            triangles.push_back(tri);
        }
    }
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
    scene.cameraProperties.background    = Color(0.7, 0.8, 1.0);

    scene.materials.reserve(10);
    scene.spheres.reserve(10);

    // Objects & Materials
    scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Color(0.659, 0.659, 0.749)});
    scene.spheres.emplace_back(Vec3(0, -100.5, -1), 100, &scene.materials.back());

    const Vec3 GOLD_IOR = {0.15557,0.42415,1.3831};
    const Vec3 GOLD_K   = {-3.6024,-2.4721,-1.9155};

    scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Color(0.1, 0.2, 0.5)});
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
    scene.cameraProperties.background    = Color(0.7, 0.8, 1.0);

    scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Color(1, 0.3, 0.5)});

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

Scene createObjScene(const std::string &path, const Mat4 &t, const Color &background) {
    Scene scene;
    scene.name = "OBJ Scene";
    scene.loadMesh(path);

    scene.cameraProperties.center        = Vec3(0, 0, 8);
    scene.cameraProperties.target        = Vec3(0, 0, 0);
    scene.cameraProperties.up            = Vec3(0, 1, 0);
    scene.cameraProperties.yfov          = 20;
    scene.cameraProperties.defocusAngle  = 0;
    scene.cameraProperties.focusDistance = 1;
    scene.cameraProperties.background    = background;

    std::cout << "Scene loaded with:" << std::endl;
    std::cout << " - " << scene.meshes.size() << " meshes" << std::endl;
    std::cout << " - " << scene.triangles.size() << " triangles" << std::endl;

    int numVertices = 0;
    for (auto &mesh: scene.meshes) {
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

Scene createShaderBallScene() {
    const auto t                 = Mat4::identity();
    const std::string path = "../src/assets/shaderball.obj";
    auto scene       = createObjScene(path, t);

    scene.cameraProperties.center     = Vec3(2.5, 16, 12);
    scene.cameraProperties.target     = Vec3(0, 3, 0);
    scene.cameraProperties.background = Color(0.7, 0.8, 1.0);
    scene.cameraProperties.yfov       = 40;

    const Vec3 GOLD_IOR = {0.15557, 0.42415, 1.3831};
    const Vec3 GOLD_K   = {-3.6024, -2.4721, -1.9155};

    // scene.materials.push_back({.type = Material::CONDUCTOR, .IOR = GOLD_IOR, .k = GOLD_K, .alphaX = 0.05, .alphaY = 0.05});
    scene.materials.push_back({.type = Material::DIELECTRIC, .IOR = Vec3(1.5), .alphaX = 0.3, .alphaY = 0.3});
    scene.meshes[3].material = &scene.materials.back();

    // scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Color(0.8, 0.8, 0.8)});
    // scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Color(1, 0.431, 0.431)});
    scene.materials.push_back({.type = Material::CONDUCTOR, .IOR = GOLD_IOR, .k = GOLD_K, .alphaX = 0.05, .alphaY = 0.05});
    scene.meshes[0].material = &scene.materials.back();
    scene.meshes[1].material = &scene.materials.back();
    scene.meshes[4].material = &scene.materials.back();

    scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Color(0.3, 0.3, 0.3)});
    scene.meshes[2].material = &scene.materials.back();

    return scene;
}

Scene createShaderBallSceneWithLight() {
    const auto t                 = Mat4::identity();
    const std::string path = "../src/assets/shaderball.obj";
    auto scene       = createObjScene(path, t);

    scene.cameraProperties.center     = Vec3(2.5, 16, 12);
    scene.cameraProperties.target     = Vec3(0, 3, 0);
    scene.cameraProperties.background = Color(0.1, 0.1, 0.1);
    scene.cameraProperties.yfov       = 40;

    // Single point light
    const Light point = {
            .type = Light::POINT,
            .position = Vec3(2.5, 16, 12),
            .intensity = WHITE,
            .scale = 50
    };
    scene.lights.push_back(point);

    const Vec3 GOLD_IOR = {0.15557, 0.42415, 1.3831};
    const Vec3 GOLD_K   = {-3.6024, -2.4721, -1.9155};

    // scene.materials.push_back({.type = Material::CONDUCTOR, .IOR = GOLD_IOR, .k = GOLD_K, .alphaX = 0.05, .alphaY = 0.05});
    scene.materials.push_back({.type = Material::DIELECTRIC, .IOR = Vec3(1.5), .alphaX = 0.3, .alphaY = 0.3});
    scene.meshes[3].material = &scene.materials.back();

    // scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Color(0.8, 0.8, 0.8)});
    // scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Color(1, 0.431, 0.431)});
    scene.materials.push_back({.type = Material::CONDUCTOR, .IOR = GOLD_IOR, .k = GOLD_K, .alphaX = 0.05, .alphaY = 0.05});
    scene.meshes[0].material = &scene.materials.back();
    scene.meshes[1].material = &scene.materials.back();
    scene.meshes[4].material = &scene.materials.back();

    scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Color(0.3, 0.3, 0.3)});
    scene.meshes[2].material = &scene.materials.back();

    return scene;
}

Scene createKnobScene() {
    auto t           = Mat4::identity();
    std::string path = "../src/assets/knob.obj";
    auto scene       = createObjScene(path, t);

    scene.cameraProperties.center     = Vec3(0, 3, 8);
    scene.cameraProperties.target     = Vec3(0, 0, 0);
    scene.cameraProperties.background = Color(0.7, 0.8, 1.0);
    scene.cameraProperties.yfov       = 15;

    scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Color(0.3, 0.3, 0.)});
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

// Scene createCornellBox() {
//     std::string path = "../src/assets/cornell_box.obj";
//     Scene scene      = createObjScene(path, Mat4::identity(), Color(0.7, 0.8, 1.0));
//
//     scene.materials.push_back({.type = Material::DIFFUSE_LIGHT, .emission = 15 * WHITE});
//     scene.meshes.back().material = &scene.materials.back();
//
//     scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Color(.73, .73, .73)});
//     scene.meshes[0].material = &scene.materials.back();
//     scene.meshes[1].material = &scene.materials.back();
//     scene.meshes[2].material = &scene.materials.back();
//     scene.meshes[5].material = &scene.materials.back();
//     scene.meshes[6].material = &scene.materials.back();
//
//     scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Color(.12, .45, .15)});
//     scene.meshes[3].material = &scene.materials.back();
//
//     scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Color(.65, .05, .05)});
//     scene.meshes[4].material = &scene.materials.back();
//
//     scene.cameraProperties.center       = Vec3(278, 273, -800);
//     scene.cameraProperties.target       = Vec3(278, 273, 0);
//     scene.cameraProperties.up           = Vec3(0, 1, 0);
//     scene.cameraProperties.defocusAngle = 0;
//     scene.cameraProperties.yfov         = 40;
//
//     scene.cameraProperties.background = BLACK;
//
//     return scene;
// }

// Scene createF22Scene(bool isDielectric) {
//     std::string path = "../src/assets/f22_box.obj";
//     Scene scene      = createObjScene(path, Mat4::identity(), Color(0.2, 0.2, 0.2));
//
//     scene.materials.push_back({.type = Material::DIFFUSE_LIGHT, .emission = 15 * WHITE});
//     scene.meshes.back().material = &scene.materials.back();
//
//     scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Color(.73, .73, .73)});
//     scene.meshes[1].material = &scene.materials.back();
//     scene.meshes[2].material = &scene.materials.back();
//     scene.meshes[3].material = &scene.materials.back();
//
//     scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Color(.12, .45, .15)});
//     scene.meshes[4].material = &scene.materials.back();
//
//     scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Color(.65, .05, .05)});
//     scene.meshes[5].material = &scene.materials.back();
//
//     if (!isDielectric) {
//         *scene.meshes[0].material = {.type = Material::METAL, .albedo = Color(0.8, 0.8, 0.8)};
//     } else {
//         *scene.meshes[0].material = {.type = Material::DIELECTRIC, .refractionIndex = 1.5f};
//     }
//
//     scene.cameraProperties.center = Vec3(0, 1, 3.5);
//     scene.cameraProperties.target = Vec3(0, 1, 0);
//     scene.cameraProperties.up           = Vec3(0, 1, 0);
//     scene.cameraProperties.defocusAngle = 0;
//     scene.cameraProperties.yfov         = 40;
//
//     scene.cameraProperties.background = BLACK;
//
//     return scene;
// }
