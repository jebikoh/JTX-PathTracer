#include "scene_loader.hpp"

#include "image.hpp"

#include <rapidobj.hpp>

JtxResult jtx::LoadScene(const std::filesystem::path &path, Scene &scene) {
    LOG_INFO(LOADER,"Loading scene: {}", path.string());
    auto fileExt = path.extension().string();
    std::ranges::transform(fileExt, fileExt.begin(), [](const unsigned char c) { return std::tolower(c); });
    if (!JTX_SCENE_SUPPORTED_FORMATS.contains(fileExt)) {
        LOG_ERROR(LOADER,"File extension not supported: {}", fileExt);
        return JTX_ERROR_INVALID_FILE_EXTENSION;
    }

    if (fileExt == ".obj") {
        return detail::LoadObj(path, scene);
    }

    if (fileExt == ".gltf") {
        return detail::LoadGltf(path, scene);
    }

    // This should ideally never be called
    LOG_ERROR(LOADER, "Supported file extension missing loader");
    return JTX_FAILURE;
}

/**
 * A few things to note about the OBJ loader:
 *  - The y texture coordinate is flipped in OBJ files
 *  - OBJ files allow different materials per face, but we only support one material per mesh;
 *    so we just grab the first material ID per mesh and use that for all faces
 */
JtxResult jtx::detail::LoadObj(const std::filesystem::path &path, jtx::Scene &scene) {
    LOG_DEBUG(LOADER,"Loading OBJ file: {}", path.string());
    rapidobj::Result result = rapidobj::ParseFile(path.string());
    if (result.error) {
        LOG_ERROR(LOADER,"Error loading OBJ file: {}", result.error.code.message());
        return JTX_ERROR_FILE_LOADING;
    }

    std::string baseDir = path.parent_path().string() + '/';

    // We only support triangles
    const bool bTriangulateSuccess = rapidobj::Triangulate(result);
    if (!bTriangulateSuccess) {
        LOG_ERROR(LOADER,"Error triangulating OBJ file: {}", result.error.code.message());
        return JTX_ERROR_FILE_INVALID_DATA;
    }

    scene.name = path.string();

    // Hashmap to keep track of textures we've already loaded
    std::unordered_map<std::string, int> textureMap;

    // Load all materials
    for (const auto & material : result.materials) {
        Material mat{};

        // Diffuse material loading
        mat.parameters.diffuse = vec3(material.diffuse.data());
        if (!material.diffuse_texname.empty()) {
            auto texturePath = material.diffuse_texname;
            if (textureMap.contains(texturePath)) {
                mat.textureIndices.albedo = textureMap[texturePath];
            } else {
                scene.textures.emplace_back();
                Image8u::Load((baseDir + texturePath), scene.textures.back());
                const size_t textureIndex = scene.textures.size() - 1;
                textureMap[texturePath] = textureIndex;
                mat.textureIndices.albedo = textureIndex;
            }
        } else {
            mat.textureIndices.albedo = JTX_MATERIAL_TEXTURE_INDEX_NONE;
        }

        mat.parameters.emission = vec3(material.emission.data());
        scene.materials.push_back(mat);
    }

    if (scene.materials.empty()) {
        LOG_DEBUG(LOADER, "No materials found in OBJ file, adding default material");
        Material mat{};
        mat.mType = Material::DIFFUSE;
        mat.parameters.diffuse = vec3(1.0f, 1.0f, 1.0f);
        scene.materials.push_back(mat);
    }

    const auto &positions = result.attributes.positions;
    const auto &normals   = result.attributes.normals;
    const auto &uvs       = result.attributes.texcoords;

    // Process meshes
    // All mesh data is stored into buffers on the scene struct
    for (const auto &shape : result.shapes) {
        const auto &mesh = shape.mesh;
        Mesh newMesh{};

        newMesh.name = shape.name.empty() ? std::string("mesh_") + std::to_string(scene.meshes.size()) : shape.name;
        LOG_DEBUG(LOADER,"Loading mesh: {}", newMesh.name);

        const auto numVertices = mesh.indices.size();
        const auto numIndices = numVertices / 3;

        newMesh.startIndex = scene.indices.size();
        newMesh.numIndices = numIndices;

        // We don't support meshes having multiple materials, so we just take the first material ID
        if (mesh.material_ids.empty()) {
            newMesh.materialIndex = 0;
        } else {
            newMesh.materialIndex = mesh.material_ids[0];
        }

        scene.meshes.push_back(newMesh);

        // We keep track of the current vertex index ourselves
        auto vertexIndex = static_cast<uint32_t>(scene.positions.size());
        for (size_t i = 0; i < mesh.indices.size(); i += 3) {
            const auto i0 = mesh.indices[i];
            const auto i1 = mesh.indices[i + 1];
            const auto i2 = mesh.indices[i + 2];
            scene.indices.emplace_back(vertexIndex, vertexIndex + 1, vertexIndex + 2);
            scene.materialIndices.push_back(newMesh.materialIndex);

            // Positions
            {
                // FYI, jtx vector classes have an unsafe constructor that take in a T pointer
                scene.positions.emplace_back(&positions[i0.position_index * 3]);
                scene.positions.emplace_back(&positions[i1.position_index * 3]);
                scene.positions.emplace_back(&positions[i2.position_index * 3]);
            }

            // Normals
            {
                if (i0.normal_index < 0 || i1.normal_index < 0 || i2.normal_index < 0) {
                    LOG_ERROR(LOADER,"Mesh {} has no normals", newMesh.name);
                    return JTX_ERROR_FILE_INVALID_DATA;
                }

                scene.normals.emplace_back(&normals[i0.normal_index * 3]);
                scene.normals.emplace_back(&normals[i1.normal_index * 3]);
                scene.normals.emplace_back(&normals[i2.normal_index * 3]);
            }

            // UVs
            {
                if (i0.texcoord_index < 0 || i1.texcoord_index < 0 || i2.texcoord_index < 0) {
                    scene.texCoords.emplace_back(0, 0);
                    scene.texCoords.emplace_back(0, 0);
                    scene.texCoords.emplace_back(0, 0);
                } else {
                    // V coordinates are flipped in OBJ files
                    scene.texCoords.emplace_back(uvs[i0.texcoord_index * 2 + 0], 1-uvs[i0.texcoord_index * 2 + 1]);
                    scene.texCoords.emplace_back(uvs[i1.texcoord_index * 2 + 0], 1-uvs[i1.texcoord_index * 2 + 1]);
                    scene.texCoords.emplace_back(uvs[i2.texcoord_index * 2 + 0], 1-uvs[i2.texcoord_index * 2 + 1]);
                }
            }

            vertexIndex += 3;
        }
    }

    LOG_INFO(LOADER,"OBJ file loaded with {} meshes, {} vertices, {} indices", scene.meshes.size(), scene.positions.size(), scene.indices.size());
    return JTX_SUCCESS;
}

JtxResult jtx::detail::LoadGltf(const std::filesystem::path &path, jtx::Scene &scene) {
    LOG_ERROR(LOADER,"GLTF loading not implemented yet");
    return JTX_FAILURE;
}
