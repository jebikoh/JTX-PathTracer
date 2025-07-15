#include <image.hpp>
#include <scene/scene_loader.hpp>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/rapidjson.h>
#include <rapidobj.hpp>

#include <fastgltf/core.hpp>

#include <unordered_set>

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

    if (fileExt == ".jtx") {
        return detail::LoadJtx(path, scene);
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
    rapidobj::Result result = rapidobj::ParseFile(path.string(), rapidobj::MaterialLibrary::Default(rapidobj::Load::Optional));
    if (result.error) {
        LOG_ERROR(LOADER,"Error loading OBJ file: {}", result.error.code.message());
        return JTX_ERROR_FILE_LOADING;
    }

    bool bHasMaterials = result.materials.size() > 0;

    std::string baseDir = path.parent_path().string() + '/';

    // We only support triangles
    const bool bTriangulateSuccess = rapidobj::Triangulate(result);
    if (!bTriangulateSuccess) {
        LOG_ERROR(LOADER,"Error triangulating OBJ file: {}", result.error.code.message());
        return JTX_ERROR_FILE_INVALID_DATA;
    }

    scene.name = path.stem().string();

    // Hashmap to keep track of textures we've already loaded
    std::unordered_map<std::string, int> textureMap;

    const auto LoadTexture = [&](const std::string &texPath) {\
        if (texPath.empty()) return JTX_MATERIAL_TEXTURE_INDEX_NONE;

        if (textureMap.contains(texPath)) {
            return textureMap[texPath];
        }

        Image8u texture;
        if (Image8u::Load((baseDir + texPath), texture) < 0) {
            textureMap[texPath] = JTX_MATERIAL_TEXTURE_MISSING;
            return JTX_MATERIAL_TEXTURE_MISSING;
        }

        scene.textures.push_back(std::move(texture));
        const int32_t textureIndex = scene.textures.size() - 1;
        textureMap[texPath]       = textureIndex;
        return textureIndex;
    };

    // Load all materials
    for (const auto & material : result.materials) {
        Material mat{};

        mat.parameters.diffuse = vec3(material.diffuse.data());
        mat.textureIndices.diffuse = LoadTexture(material.diffuse_texname);

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
    const auto &texCoords       = result.attributes.texcoords;

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
        if (!bHasMaterials || mesh.material_ids[0] == -1) {
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

            // Check if the triangle is emissive
            if (scene.materials[newMesh.materialIndex].IsEmissive()) {
                scene.emissiveTriangles.push_back(scene.indices.size() - 1);
            }

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
                    scene.texCoords.emplace_back(texCoords[i0.texcoord_index * 2 + 0], 1-texCoords[i0.texcoord_index * 2 + 1]);
                    scene.texCoords.emplace_back(texCoords[i1.texcoord_index * 2 + 0], 1-texCoords[i1.texcoord_index * 2 + 1]);
                    scene.texCoords.emplace_back(texCoords[i2.texcoord_index * 2 + 0], 1-texCoords[i2.texcoord_index * 2 + 1]);
                }
            }

            vertexIndex += 3;
        }
    }

    LOG_INFO(LOADER, "OBJ file loaded with {} meshes, {} triangles, {} materials, {} textures", scene.meshes.size(), scene.indices.size(), scene.materials.size(), scene.textures.size());
    return JTX_SUCCESS;
}

JtxResult jtx::detail::LoadGltf(const std::filesystem::path &path, jtx::Scene &scene) {
    LOG_ERROR(LOADER, "GLTF loading not implemented yet");
    return JTX_FAILURE;
}

namespace jtx::detail {
namespace {
    using namespace rapidjson;

    template<typename T>
    void FromJson(const rapidjson::Value &parent, const char *field, T& value, const T& defaultValue = T()) {
        if (parent.HasMember(field)) {
            if constexpr(std::is_same_v<T, vec2>) {
                const auto &arr = parent[field];
                assert(arr.IsArray() && arr.Size() == 2);
                value = {arr[0].GetFloat(), arr[1].GetFloat()};
            } else if constexpr(std::is_same_v<T, vec3>) {
                const auto &arr = parent[field];
                assert(arr.IsArray() && arr.Size() == 3);
                value = {arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat()};
            } else if constexpr(std::is_same_v<T, vec4>) {
                const auto &arr = parent[field];
                assert(arr.IsArray() && arr.Size() == 4);
                value = {arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat(), arr[3].GetFloat()};
            } else if constexpr(std::is_same_v<T, float>) {
                value = parent[field].GetFloat();
            } else if constexpr(std::is_same_v<T, double>) {
                value = parent[field].GetDouble();
            } else if constexpr(std::is_same_v<T, bool>) {
                value = parent[field].GetBool();
            } else if constexpr(std::is_same_v<T, uint32_t>) {
                value = parent[field].GetUint();
            } else if constexpr(std::is_same_v<T, int32_t>) {
                value = parent[field].GetInt();
            } else if constexpr(std::is_same_v<T, std::string>) {
                value = parent[field].GetString();
            } else {
                value = defaultValue;
            }
        } else {
            value = defaultValue;
        }
    }

    // void FromJson(const Value &arr, vec2 &v) {
    //     assert(arr.IsArray() && arr.Size() == 2);
    //     v.x = arr[0].GetFloat();
    //     v.y = arr[1].GetFloat();
    // }
    //
    // void FromJson(const Value &arr, vec3 &v) {
    //     assert(arr.IsArray() && arr.Size() == 3);
    //     v.x = arr[0].GetFloat();
    //     v.y = arr[1].GetFloat();
    //     v.z = arr[2].GetFloat();
    // }
    //
    // void FromJson(const Value &arr, vec3u &v) {
    //     assert(arr.IsArray() && arr.Size() == 3);
    //     v.x = arr[0].GetUint();
    //     v.y = arr[1].GetUint();
    //     v.z = arr[2].GetUint();
    // }
}
}

JtxResult jtx::detail::LoadJtx(const std::filesystem::path &path, Scene &scene) {
    LOG_DEBUG(LOADER,"Loading JTX file: {}", path.string());

    using namespace rapidjson;

    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR(LOADER, "Failed to open JTX file: {}", path.string());
        return JTX_ERROR_FILE_LOADING;
    }

    IStreamWrapper isw(file);
    Document d;
    d.ParseStream(isw);

    if (d.HasParseError()) {
        LOG_ERROR(LOADER, "Failed to parse JTX JSON: {} (offset {})",
           GetParseError_En(d.GetParseError()), d.GetErrorOffset());
        return JTX_ERROR_FILE_INVALID_DATA;
    }

    scene.name = d["name"].GetString();

    // -- Camera settings --
    const Value& cs = d["camera"];
    FromJson(cs, "position", scene.cameraSettings.position);
    FromJson(cs, "target", scene.cameraSettings.target);
    FromJson(cs, "up", scene.cameraSettings.up);
    FromJson(cs, "focalLength", scene.cameraSettings.focalLength, 0.05f);
    FromJson(cs, "sensorWidth", scene.cameraSettings.sensorWidth, 0.036f);
    FromJson(cs, "focalDistance", scene.cameraSettings.focalDistance, 10.0f);
    FromJson(cs, "enableDOF", scene.cameraSettings.bEnableDof, false);
    FromJson(cs, "fStop", scene.cameraSettings.fStop, 2.8f);

    // -- Envmap --
    if (d.HasMember("Envmap")) {
        const Value &envmap = d["Envmap"];
        int type;
        FromJson(envmap, "type", type, 0);
        scene.envmap.type = (EnvMap::kType)type;
        FromJson(envmap, "uniform", scene.envmap.uniform);
        FromJson(envmap, "intensity", scene.envmap.intensity, 1.0f);

        std::string hdriPath;
        FromJson(envmap, "hdri", hdriPath);
        if (!hdriPath.empty()) {
            Image32f::Load(hdriPath, scene.envmap.image);
        }

        FromJson(envmap, "horizontalOffset", scene.envmap.horizontalOffset, 0.0f);
        FromJson(envmap, "verticalOffset", scene.envmap.verticalOffset, 0.0f);
    } else {
        scene.envmap.type      = EnvMap::UNIFORM;
        scene.envmap.uniform   = vec3(0.0f);
        scene.envmap.intensity = 1.0f;
    }

    // -- Textures --
    std::unordered_set<uint32_t> failedTexLoads;
    if (d.HasMember("textures")) {
        const Value& textures = d["textures"];
        const uint32_t numTextures = textures["count"].GetUint();

        scene.textures.resize(numTextures);

        uint32_t index = 0;
        for (const auto& tex_json : textures["paths"].GetArray()) {
            const std::string filename = tex_json.GetString();
            if (filename.empty()) {
                LOG_INFO(LOADER, "Texture at index {} is empty, skipping", index);
                failedTexLoads.insert(index++);
                continue;
            }

            const std::filesystem::path texPath = path.parent_path() / filename;
            Image8u texture{};
            if (Image8u::Load(texPath, texture) < 1) {
                failedTexLoads.insert(index++);
                continue;
            }

            scene.textures[index++] = std::move(texture);
        }
    } else {
        LOG_DEBUG(LOADER, "No textures found");
    }

    // -- Materials --
    const Value& materials = d["materials"];
    scene.materials.reserve(materials.Size());
    for (const auto& m_json : materials.GetArray()) {
        Material mat{};
        mat.mType = (Material::Type)m_json["type"].GetInt();

        const Value& params = m_json["parameters"];
        FromJson(params, "diffuse", mat.parameters.diffuse);
        FromJson(params, "ior", mat.parameters.ior);
        FromJson(params, "k", mat.parameters.k);
        FromJson(params, "f0", mat.parameters.f0);
        FromJson(params, "emission", mat.parameters.emission);
        FromJson(params, "emissionStrength", mat.parameters.emissionStrength, 1.0f);
        FromJson(params, "roughness", mat.parameters.roughness);

        const Value& tex = m_json["textureIndices"];
        mat.textureIndices.diffuse = tex["diffuse"].GetInt();
        if (failedTexLoads.contains(mat.textureIndices.diffuse)) mat.textureIndices.diffuse = JTX_MATERIAL_TEXTURE_MISSING;
        mat.textureIndices.metallicRoughness = tex["metallicRoughness"].GetInt();
        if (failedTexLoads.contains(mat.textureIndices.metallicRoughness)) mat.textureIndices.metallicRoughness = JTX_MATERIAL_TEXTURE_MISSING;

        scene.materials.push_back(mat);
    }

    // -- Meshes --
    const Value& meshes = d["meshes"];
    scene.meshes.reserve(meshes.Size());
    for (const auto& m_json : meshes.GetArray()) {
        Mesh mesh{};
        mesh.name = m_json["name"].GetString();
        mesh.materialIndex = m_json["materialIndex"].GetInt();
        mesh.startIndex = m_json["startIndex"].GetUint();
        mesh.numIndices = m_json["numIndices"].GetUint();
        scene.meshes.push_back(mesh);
    }

    // -- Vertex data --
    const Value& vertexData = d["vertexData"];
    std::string binFilename = vertexData["binary"].GetString();
    std::filesystem::path binPath = path.parent_path() / binFilename;

    std::ifstream binStream(binPath, std::ios::binary);
    if (!binStream.is_open()) {
        LOG_ERROR(LOADER, "Failed to open binary data file: {}", binPath.string());
        return JTX_ERROR_FILE_LOADING;
    }

    const Value& layout = vertexData["layout"];
    auto LoadDataBlock = [&]<typename T0>(const char* name, T0& targetVector) -> bool {
        if (!layout.HasMember(name)) {
            LOG_ERROR(LOADER, "Block missing from layout: {}", name);
            return false;
        }

        const Value &blockInfo = layout[name];
        const uint64_t offset = blockInfo["offset"].GetUint64();
        uint64_t count = blockInfo["count"].GetUint64();

        if (count > 0) {
            targetVector.resize(count);
            binStream.seekg(offset);
            binStream.read(reinterpret_cast<char*>(targetVector.data()), count * sizeof(typename std::remove_reference_t<T0>::value_type));
            if (!binStream.good()) {
                LOG_ERROR(LOADER, "Error reading block '{}' from binary file: {}", name, binPath.string());
                return false;
            }
        }
        return true;
    };

    if (!LoadDataBlock("indices", scene.indices)) return JTX_ERROR_FILE_LOADING;
    if (!LoadDataBlock("positions", scene.positions)) return JTX_ERROR_FILE_LOADING;
    if (!LoadDataBlock("normals", scene.normals)) return JTX_ERROR_FILE_LOADING;
    if (!LoadDataBlock("texCoords", scene.texCoords)) return JTX_ERROR_FILE_LOADING;
    if (!LoadDataBlock("colors", scene.colors)) return JTX_ERROR_FILE_LOADING;

    // -- Material Indices & Emissive Triangles --
    scene.materialIndices.resize(scene.indices.size());
    for (const auto &mesh : scene.meshes) {
        for (uint32_t i = 0; i < mesh.numIndices; ++i) {
            scene.materialIndices[mesh.startIndex + i] = mesh.materialIndex;
            if (scene.materials[mesh.materialIndex].IsEmissive()) {
                scene.emissiveTriangles.push_back(mesh.startIndex + i);
            }
        }
    }

    LOG_INFO(LOADER, "JTX file loaded with {} meshes, {} triangles, {} materials, {} textures", scene.meshes.size(), scene.indices.size(), scene.materials.size(), scene.textures.size());
    return JTX_SUCCESS;
}
