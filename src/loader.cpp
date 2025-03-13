#include "loader.hpp"

#include "image.hpp"
#include "material.hpp"
#include "scene.hpp"

#include <fmt/core.h>
#include <rapidobj.hpp>

const Material DEFAULT_MATERIAL = {.type = Material::DIFFUSE, .albedo = Vec3(1, 0.451, 0.969), .albedoTexId = -1};

bool loadScene(const std::string &path, Scene &scene) {
    const size_t lastDot = path.find_last_of(".");
    if (lastDot == std::string::npos) {
        fmt::println("File is missing extension.\n");
        return false;
    }
    const auto ext = path.substr(lastDot);

    if (ext == ".obj") { return loadObj(path, scene); }
    if (ext == ".gltf") { return loadGltf(path, scene); }

    fmt::print("Extension not supported: {}\n", ext);
    return false;
}

bool loadObj(const std::string &path, Scene &scene) {
    // LOAD
    rapidobj::Result result = rapidobj::ParseFile(path);
    if (result.error) {
        fmt::print("{}\n", result.error.code.message());
        return false;
    }

    const size_t lastSlash = path.find_last_of("/\\");
    std::string baseDir    = (lastSlash != std::string::npos) ? path.substr(0, lastSlash + 1) : "";

    // TRIANGULATE
    const bool success = rapidobj::Triangulate(result);
    if (!success) {
        fmt::print("{}\n", result.error.code.message());
        return false;
    }

    // PROCESS MATERIALS
    std::unordered_map<std::string, int> texMap;

    for (int i = 0; i < result.materials.size(); i++) {
        const auto &material = result.materials[i];
        Material mat;

        // Load diffuse
        if (material.diffuse_texname.length() > 0) {
            auto texPath = material.diffuse_texname;
            if (texMap.contains(texPath)) {
                mat.albedoTexId = texMap[texPath];
            } else {
                scene.textures.emplace_back((baseDir + texPath).c_str());
                texMap[texPath] = scene.textures.size() - 1;
                mat.albedoTexId = texMap[texPath];
            }
        } else {
            auto data  = material.diffuse.data();
            mat.albedo = Vec3(data[0], data[1], data[2]);
            mat.albedoTexId = -1;
        }

        scene.materials.push_back(mat);
    }
    scene.materials.push_back(DEFAULT_MATERIAL);
    const size_t defaultMaterialIdx = scene.materials.size() - 1;

    const auto &positions = result.attributes.positions;
    const auto &normals   = result.attributes.normals;
    const auto &texcoords = result.attributes.texcoords;

    // PROCESS MESHES
    for (const auto &shape: result.shapes) {
        Vec3 *p;
        Vec3 *n;
        Vec2f *uv;
        Vec3i *idx;

        const auto &mesh = shape.mesh;
        fmt::print("Loading Mesh: {}\n", shape.name);

        const int numIdx      = mesh.indices.size() / 3;
        const int numVertices = mesh.indices.size();

        p   = new Vec3[numVertices];
        n   = new Vec3[numVertices];
        uv  = new Vec2f[numVertices];
        idx = new Vec3i[numIdx];

        int vertexCount = 0;
        for (int i = 0; i < mesh.indices.size(); i += 3) {
            // INDICES
            const auto i0 = mesh.indices[i + 0];
            const auto i1 = mesh.indices[i + 1];
            const auto i2 = mesh.indices[i + 2];

            int faceIdx  = i / 3;
            idx[faceIdx] = Vec3i(vertexCount, vertexCount + 1, vertexCount + 2);

            // POSITIONS
            {
                p[vertexCount] = Vec3(
                        positions[i0.position_index * 3 + 0],
                        positions[i0.position_index * 3 + 1],
                        positions[i0.position_index * 3 + 2]);
                p[vertexCount + 1] = Vec3(
                        positions[i1.position_index * 3 + 0],
                        positions[i1.position_index * 3 + 1],
                        positions[i1.position_index * 3 + 2]);
                p[vertexCount + 2] = Vec3(
                        positions[i2.position_index * 3 + 0],
                        positions[i2.position_index * 3 + 1],
                        positions[i2.position_index * 3 + 2]);
            }

            if (i0.normal_index == -1 || i1.normal_index == -1 || i2.normal_index == -1) {
                fmt::println("Mesh is missing normals.");
                delete[] p;
                delete[] n;
                delete[] uv;
                delete[] idx;
                return false;
            }

            // NORMAL
            {
                n[vertexCount] = Vec3(
                        normals[i0.normal_index * 3 + 0],
                        normals[i0.normal_index * 3 + 1],
                        normals[i0.normal_index * 3 + 2]);
                n[vertexCount + 1] = Vec3(
                        normals[i1.normal_index * 3 + 0],
                        normals[i1.normal_index * 3 + 1],
                        normals[i1.normal_index * 3 + 2]);
                n[vertexCount + 2] = Vec3(
                        normals[i2.normal_index * 3 + 0],
                        normals[i2.normal_index * 3 + 1],
                        normals[i2.normal_index * 3 + 2]);
            }

            // UVs
            if (i0.texcoord_index != -1 && i1.texcoord_index != -1 && i2.texcoord_index != -1) {
                uv[vertexCount] = Vec2f(
                        texcoords[i0.texcoord_index * 2 + 0],
                        texcoords[i0.texcoord_index * 2 + 1]);
                uv[vertexCount + 1] = Vec2f(
                        texcoords[i1.texcoord_index * 2 + 0],
                        texcoords[i1.texcoord_index * 2 + 1]);
                uv[vertexCount + 2] = Vec2f(
                        texcoords[i2.texcoord_index * 2 + 0],
                        texcoords[i2.texcoord_index * 2 + 1]);
            } else {
                uv[vertexCount]     = Vec2f(0, 0);
                uv[vertexCount + 1] = Vec2f(0, 0);
                uv[vertexCount + 2] = Vec2f(0, 0);
            }

            vertexCount += 3;
        }

        Material *meshMaterial = &(mesh.material_ids.size() > 0 ? scene.materials[mesh.material_ids[0]] : scene.materials[defaultMaterialIdx]);
        auto meshName          = shape.name.empty() ? std::string("mesh_" + scene.meshes.size()) : shape.name;
        scene.meshes.emplace_back(meshName,
                                  idx,
                                  numIdx,
                                  p,
                                  numVertices,
                                  n,
                                  uv,
                                  meshMaterial);

        // Build triangles
        int meshIndex = scene.meshes.size() - 1;
        for (int i = 0; i < numIdx; i++) {
            Triangle tri;
            tri.index     = i;
            tri.meshIndex = meshIndex;
            scene.triangles.emplace_back(tri);
        }
    }

    return true;
}

bool loadGltf(const std::string &path, Scene &scene) {
    return false;
}