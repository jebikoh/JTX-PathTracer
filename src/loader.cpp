#include "loader.hpp"

#include "logger.hpp"
#include "material.hpp"
#include "scene.hpp"

#include <fmt/core.h>
#include <rapidobj.hpp>

const Material DEFAULT_MATERIAL = {
        .mType      = Material::DIFFUSE,
        .parameters = {
                .albedo = Vec3(1, 0.451, 0.969)}};
constexpr int JTX_SCENE_MATERIAL_LIMIT = 128;

bool loadScene(const std::string &path, Scene &scene) {
    const size_t lastDot = path.find_last_of(".");
    if (lastDot == std::string::npos) {
        LOG_ERROR("File path is missing extension: ", path);
        return false;
    }
    const auto ext = path.substr(lastDot);

    if (ext == ".obj") { return loadObj(path, scene); }
    if (ext == ".gltf") { return loadGltf(path, scene); }

    LOG_ERROR("File extension not supported: ", ext);
    return false;
}

bool loadObj(const std::string &path, Scene &scene) {
    LOG_INFO("Loading OBJ file: ", path);
    scene.materials.reserve(JTX_SCENE_MATERIAL_LIMIT);
    // LOAD
    rapidobj::Result result = rapidobj::ParseFile(path);
    if (result.error) {
        LOG_ERROR("Failed to parse OBJ file: ", result.error.code.message());
        return false;
    }

    const size_t lastSlash = path.find_last_of("/\\");
    std::string baseDir    = (lastSlash != std::string::npos) ? path.substr(0, lastSlash + 1) : "";

    // TRIANGULATE
    const bool success = rapidobj::Triangulate(result);
    if (!success) {
        LOG_ERROR("Failed to triangulate OBJ file: ", result.error.code.message());
        return false;
    }

    // PROCESS MATERIALS
    std::unordered_map<std::string, int> texMap;

    for (int i = 0; i < result.materials.size(); i++) {
        const auto &material = result.materials[i];
        Material mat{};

        // Load diffuse
        if (material.diffuse_texname.length() > 0) {
            auto texPath = material.diffuse_texname;
            if (texMap.contains(texPath)) {
                mat.textureIndices.albedo = texMap[texPath];
            } else {
                LOG_INFO("Loading texture: {}", texPath);
                scene.textures.emplace_back((baseDir + texPath).c_str());
                texMap[texPath]           = scene.textures.size() - 1;
                mat.textureIndices.albedo = texMap[texPath];
            }
        } else {
            auto data                 = material.diffuse.data();
            mat.parameters.albedo     = Vec3(data[0], data[1], data[2]);
            mat.textureIndices.albedo = -1;
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
        Mesh newMesh{};

        const auto &mesh = shape.mesh;
        newMesh.name = shape.name.empty() ? std::string("mesh_") + std::to_string(scene.meshes.size()) : shape.name;
        LOG_INFO("Loading mesh: {}", newMesh.name);

        const int numIdx      = mesh.indices.size() / 3;
        const int numVertices = mesh.indices.size();

        newMesh.vertices.reserve(numVertices);
        newMesh.normals.reserve(numVertices);
        newMesh.uvs.reserve(numVertices);
        newMesh.indices.reserve(numIdx);

        int vertexCount = 0;
        for (int i = 0; i < mesh.indices.size(); i += 3) {
            // INDICES
            const auto i0 = mesh.indices[i + 0];
            const auto i1 = mesh.indices[i + 1];
            const auto i2 = mesh.indices[i + 2];
            newMesh.indices.emplace_back(vertexCount, vertexCount + 1, vertexCount + 2);

            // POSITIONS
            {
                newMesh.vertices.emplace_back(
                        positions[i0.position_index * 3 + 0],
                        positions[i0.position_index * 3 + 1],
                        positions[i0.position_index * 3 + 2]);
                newMesh.vertices.emplace_back(
                        positions[i1.position_index * 3 + 0],
                        positions[i1.position_index * 3 + 1],
                        positions[i1.position_index * 3 + 2]);
                newMesh.vertices.emplace_back(
                        positions[i2.position_index * 3 + 0],
                        positions[i2.position_index * 3 + 1],
                        positions[i2.position_index * 3 + 2]);
            }

            // NORMALS
            if (i0.normal_index == -1 || i1.normal_index == -1 || i2.normal_index == -1) {
                LOG_ERROR("Mesh is missing normals: {}", newMesh.name);
                return false;
            }
            {
                newMesh.normals.emplace_back(
                        normals[i0.normal_index * 3 + 0],
                        normals[i0.normal_index * 3 + 1],
                        normals[i0.normal_index * 3 + 2]);
                newMesh.normals.emplace_back(
                        normals[i1.normal_index * 3 + 0],
                        normals[i1.normal_index * 3 + 1],
                        normals[i1.normal_index * 3 + 2]);
                newMesh.normals.emplace_back(
                        normals[i2.normal_index * 3 + 0],
                        normals[i2.normal_index * 3 + 1],
                        normals[i2.normal_index * 3 + 2]);
            }

            // UVs
            if (i0.texcoord_index != -1 && i1.texcoord_index != -1 && i2.texcoord_index != -1) {
                newMesh.uvs.emplace_back(
                        texcoords[i0.texcoord_index * 2 + 0],
                        -texcoords[i0.texcoord_index * 2 + 1]);
                newMesh.uvs.emplace_back(
                        texcoords[i1.texcoord_index * 2 + 0],
                        -texcoords[i1.texcoord_index * 2 + 1]);
                newMesh.uvs.emplace_back(
                        texcoords[i2.texcoord_index * 2 + 0],
                        -texcoords[i2.texcoord_index * 2 + 1]);
            } else {
                newMesh.uvs.emplace_back(0, 0);
                newMesh.uvs.emplace_back(0, 0);
                newMesh.uvs.emplace_back(0, 0);
            }

            vertexCount += 3;
        }

        newMesh.material = &(mesh.material_ids.size() > 0 ? scene.materials[mesh.material_ids[0]] : scene.materials[defaultMaterialIdx]);
        scene.meshes.push_back(newMesh);

        // Build triangles
        LOG_INFO("Building mesh faces");
        int meshIndex = scene.meshes.size() - 1;
        for (int i = 0; i < numIdx; i++) {
            Triangle tri;
            tri.index     = i;
            tri.meshIndex = meshIndex;
            scene.triangles.emplace_back(tri);
        }
    }

    LOG_INFO("Loaded scene with {} meshes and {} triangles", scene.meshes.size(), scene.triangles.size());

    return true;
}

bool loadGltf(const std::string &path, Scene &scene) {
    return false;
}