#define TINYOBJLOADER_IMPLEMENTATION
#include "scene.hpp"

#include "mesh.hpp"
#include "tiny_obj_loader.h"

static Material DEFAULT_MAT = {.type = Material::LAMBERTIAN, .albedo = Color(1, 0.3, 0.5)};

void Scene::loadMesh(const std::string &path) {
    materials.push_back(DEFAULT_MAT);

    tinyobj::ObjReaderConfig reader_config;
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

        // Create the Mesh and store it
        // Use the default material we pushed earlier: materials.back()
        meshes.emplace_back(finalIndices, shapeTriIndices.size(), finalVerts, shapeVerts.size(), finalNormals, materials.back());

        // Now register all triangles from this mesh in the scene
        int meshIndex     = static_cast<int>(meshes.size()) - 1;
        int triangleCount = static_cast<int>(numIndices / 3);

        for (int t = 0; t < triangleCount; t++) {
            Triangle tri;
            tri.index     = t;
            tri.meshIndex = meshIndex;
            triangles.push_back(tri);
        }
    }
}

void createDefaultScene(Scene &scene) {
    scene.name = "Default Scene";

    // Camera
    scene.cameraProperties.center        = Vec3(-2, 2, 1);
    scene.cameraProperties.target        = Vec3(0, 0, -1);
    scene.cameraProperties.up            = Vec3(0, 1, 0);
    scene.cameraProperties.yfov          = 20;
    scene.cameraProperties.defocusAngle  = 10.0;
    scene.cameraProperties.focusDistance = 3.4;
    scene.cameraProperties.background    = Color(0.7, 0.8, 1.0);

    scene.materials.reserve(10);
    scene.spheres.reserve(10);

    // Objects & Materials
    scene.materials.push_back({.type = Material::LAMBERTIAN, .albedo = Color(0.8, 0.8, 0.0)});
    scene.spheres.emplace_back(Vec3(0, -100.5, -1), 100, scene.materials.back());

    scene.materials.push_back({.type = Material::LAMBERTIAN, .albedo = Color(0.1, 0.2, 0.5)});
    scene.spheres.emplace_back(Vec3(0, 0, -1.2), 0.5, scene.materials.back());

    scene.materials.push_back({.type = Material::METAL, .albedo = Color(0.8, 0.6, 0.2), .fuzz = 1.0});
    scene.spheres.emplace_back(Vec3(1, 0, -1), 0.5, scene.materials.back());

    scene.materials.push_back({.type = Material::DIELECTRIC, .refractionIndex = 1.5});
    scene.spheres.emplace_back(Vec3(-1, 0, -1), 0.5, scene.materials.back());

    scene.materials.push_back({.type = Material::DIELECTRIC, .refractionIndex = 1.00 / 1.50});
    scene.spheres.emplace_back(Vec3(-1, 0, -1), 0.4, scene.materials.back());
}

void createTestScene(Scene &scene) {
    scene.cameraProperties.center        = Vec3(0, 3, 8);
    scene.cameraProperties.target        = Vec3(0, 2, -1);
    scene.cameraProperties.up            = Vec3(0, 1, 0);
    scene.cameraProperties.yfov          = 20;
    scene.cameraProperties.defocusAngle  = 0;
    scene.cameraProperties.focusDistance = 3.4;
    scene.cameraProperties.background    = Color(0.7, 0.8, 1.0);

    scene.materials.reserve(20);
    scene.spheres.reserve(20);

    // Glass bubbles
    scene.materials.push_back({.type = Material::DIELECTRIC, .refractionIndex = 1.5f});
    scene.materials.push_back({.type = Material::DIELECTRIC, .refractionIndex = 1.0f / 1.5f});

    // Ground
    scene.materials.push_back({.type = Material::LAMBERTIAN, .albedo = Color(0.8, 0.8, 0.0)});
    scene.spheres.push_back({Vec3(0, -100.5, -1), 100, scene.materials.back()});

    // Row one
    scene.spheres.push_back({Vec3(-1, 1, -1), 0.4, scene.materials[0]});
    scene.spheres.push_back({Vec3(-1, 1, -1), 0.3, scene.materials[1]});

    scene.materials.push_back({.type = Material::LAMBERTIAN, .albedo = Color(0.1, 0.2, 0.5)});
    scene.spheres.push_back({Vec3(0, 1, -1), 0.4, scene.materials.back()});

    scene.materials.push_back({.type = Material::METAL, .albedo = Color(0.8, 0.6, 0.2), .fuzz = 1.0});
    scene.spheres.emplace_back(Vec3(1, 1, -1), 0.4, scene.materials.back());

    // Row two
    scene.spheres.push_back({Vec3(1, 2, -1), 0.4, scene.materials[0]});
    scene.spheres.push_back({Vec3(1, 2, -1), 0.3, scene.materials[1]});

    scene.materials.push_back({.type = Material::LAMBERTIAN, .albedo = Color(1, 0.3, 0.5)});
    scene.spheres.push_back({Vec3(-1, 2, -1), 0.4, scene.materials.back()});

    scene.materials.push_back({.type = Material::METAL, .albedo = Color(0.8, 0.8, 0.8), .fuzz = 0.5});
    scene.spheres.emplace_back(Vec3(0, 2, -1), 0.4, scene.materials.back());

    // Row 3
    scene.spheres.push_back({Vec3(0, 3, -1), 0.4, scene.materials[0]});
    scene.spheres.push_back({Vec3(0, 3, -1), 0.3, scene.materials[1]});

    scene.materials.push_back({.type = Material::LAMBERTIAN, .albedo = Color(0.5, 0.3, 0.5)});
    scene.spheres.push_back({Vec3(1, 3, -1), 0.4, scene.materials.back()});

    scene.materials.push_back({.type = Material::METAL, .albedo = Color(0.8, 0.6, 0.2), .fuzz = 0});
    scene.spheres.emplace_back(Vec3(-1, 3, -1), 0.4, scene.materials.back());
}

void createCoverScene(Scene &scene) {
    constexpr float DIFFUSE_PROBABILITY = 0.8;
    constexpr float METAL_PROBABILITY   = 0.15;

    constexpr float METAL_CUTOFF = DIFFUSE_PROBABILITY + METAL_PROBABILITY;

    // There are around 500 spheres in this scene
    scene.spheres.reserve(500);
    scene.materials.reserve(500);

    // Glass
    scene.materials.push_back({.type = Material::DIELECTRIC, .refractionIndex = 1.5});

    // Ground
    scene.materials.push_back({.type = Material::LAMBERTIAN, .albedo = Color(0.5, 0.5, 0.5)});
    scene.spheres.emplace_back(Vec3(0, -1000, 0), 1000, scene.materials.back());


    for (int a = -11; a < 11; ++a) {
        for (int b = -11; b < 11; ++b) {
            const Vec3 center(a + 0.9 * randomFloat(), 0.2, b + 0.9 * randomFloat());

            const auto matIdx = randomFloat();
            if ((center - Vec3(4, 0.2, 0)).len() > 0.9) {
                if (matIdx < DIFFUSE_PROBABILITY) {
                    const auto albedo = randomVec3() * randomVec3();
                    scene.materials.push_back({.type = Material::LAMBERTIAN, .albedo = albedo});
                    scene.spheres.emplace_back(center, 0.2, scene.materials.back());
                } else if (matIdx < METAL_CUTOFF) {
                    const auto albedo = randomVec3(0.5, 1);
                    const auto fuzz   = randomFloat(0, 0.5);
                    scene.materials.push_back({.type = Material::METAL, .albedo = albedo, .fuzz = fuzz});
                    scene.spheres.emplace_back(center, 0.2, scene.materials.back());
                } else {
                    // Dielectric glass always at the front
                    scene.spheres.emplace_back(center, 0.2, scene.materials.front());
                }
            }
        }
    }

    scene.spheres.emplace_back(Vec3(0, 1, 0), 1.0, scene.materials.front());

    scene.materials.push_back({.type = Material::LAMBERTIAN, .albedo = Color(0.4, 0.2, 0.1)});
    scene.spheres.emplace_back(Vec3(-4, 1, 0), 1.0, scene.materials.back());

    scene.materials.push_back({.type = Material::METAL, .albedo = Color(0.7, 0.6, 0.5), .fuzz = 0.0});
    scene.spheres.emplace_back(Vec3(4, 1, 0), 1.0, scene.materials.back());

    scene.cameraProperties.yfov          = 20;
    scene.cameraProperties.center        = {13, 2, 3};
    scene.cameraProperties.target        = {0, 0, 0};
    scene.cameraProperties.up            = {0, 1, 0};
    scene.cameraProperties.defocusAngle  = 0.6;
    scene.cameraProperties.focusDistance = 10.0;
    scene.cameraProperties.background    = Color(0.7, 0.8, 1.0);
}

void createMeshScene(Scene &scene) {
    scene.cameraProperties.center        = Vec3(0, 0, 8);
    scene.cameraProperties.target        = Vec3(0, 0, -1);
    scene.cameraProperties.up            = Vec3(0, 1, 0);
    scene.cameraProperties.yfov          = 20;
    scene.cameraProperties.defocusAngle  = 0;
    scene.cameraProperties.focusDistance = 3.4;
    scene.cameraProperties.background    = Color(0.7, 0.8, 1.0);

    scene.materials.push_back({.type = Material::LAMBERTIAN, .albedo = Color(1, 0.3, 0.5)});

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

    scene.meshes.push_back({indices, 2, vertices, 4, normals, scene.materials.back()});

    scene.triangles.push_back({0, 0});
    scene.triangles.push_back({1, 0});
}

void createObjScene(Scene &scene, std::string &path, const Mat4 &t, const Material &material, const Color &background) {
    scene.loadMesh(path);

    scene.cameraProperties.center        = Vec3(0, 0, 8);
    scene.cameraProperties.target        = Vec3(0, 0, 0);
    scene.cameraProperties.up            = Vec3(0, 1, 0);
    scene.cameraProperties.yfov          = 20;
    scene.cameraProperties.defocusAngle  = 0;
    scene.cameraProperties.focusDistance = 1;
    scene.cameraProperties.background    = background;

    std::cout << scene.meshes[0].numVertices << std::endl;
    std::cout << scene.meshes[0].numIndices << std::endl;

    scene.meshes[0].material = material;

    // Transform all verts and norms
    for (int i = 0; i < scene.meshes[0].numVertices; ++i) {
        scene.meshes[0].vertices[i] = t.applyToPoint(scene.meshes[0].vertices[i]);
        scene.meshes[0].normals[i]  = t.applyToNormal(scene.meshes[0].normals[i]);
    }
}
