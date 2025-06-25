#include <scene/scene.hpp>
#include <scene/scene_exporter.hpp>

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include <unordered_set>

using namespace rapidjson;

namespace jtx {

Value ToJson(const vec2 &v, Document::AllocatorType &allocator) {
    Value arr(kArrayType);
    arr.PushBack(v.x, allocator);
    arr.PushBack(v.y, allocator);
    return arr;
}

Value ToJson(const vec3 &v, Document::AllocatorType &allocator) {
    Value arr(kArrayType);
    arr.PushBack(v.x, allocator);
    arr.PushBack(v.y, allocator);
    arr.PushBack(v.z, allocator);
    return arr;
}

Value ToJson(const vec3u &v, Document::AllocatorType &allocator) {
    Value arr(kArrayType);
    arr.PushBack(v.x, allocator);
    arr.PushBack(v.y, allocator);
    arr.PushBack(v.z, allocator);
    return arr;
}

JtxResult ExportScene(const Scene &scene, const std::filesystem::path &path) {
    LOG_INFO(LOADER, "Exporting scene: {}", scene.name);

    // Setup JSON document
    Document d;
    d.SetObject();
    Document::AllocatorType &allocator = d.GetAllocator();

    // Scene name & file type version
    d.AddMember("name", Value(scene.name.c_str(), allocator).Move(), allocator);
    d.AddMember("version", Value("1.0", allocator).Move(), allocator);

    // Camera settings
    Value cs(kObjectType);
    cs.AddMember("position", ToJson(scene.cameraSettings.position, allocator), allocator);
    cs.AddMember("target", ToJson(scene.cameraSettings.target, allocator), allocator);
    cs.AddMember("up", ToJson(scene.cameraSettings.up, allocator), allocator);
    cs.AddMember("focalLength", scene.cameraSettings.focalLength, allocator);
    cs.AddMember("sensorWidth", scene.cameraSettings.sensorWidth, allocator);
    cs.AddMember("focalDistance", scene.cameraSettings.focalDistance, allocator);
    cs.AddMember("enableDOF", scene.cameraSettings.bEnableDof, allocator);
    cs.AddMember("fStop", scene.cameraSettings.fStop, allocator);
    d.AddMember("camera", cs, allocator);

    // Materials
    Value materials(kArrayType);
    for (const auto &material : scene.materials) {
        Value m(kObjectType);
        m.AddMember("type", material.mType, allocator);

        Value params(kObjectType);
        params.AddMember("diffuse", ToJson(material.parameters.diffuse, allocator), allocator);
        params.AddMember("ior", ToJson(material.parameters.ior, allocator), allocator);
        params.AddMember("k", ToJson(material.parameters.k, allocator), allocator);
        params.AddMember("f0", ToJson(material.parameters.f0, allocator), allocator);
        params.AddMember("emission", ToJson(material.parameters.emission, allocator), allocator);
        params.AddMember("roughness", ToJson(material.parameters.roughness, allocator), allocator);
        m.AddMember("parameters", params, allocator);

        Value tex(kObjectType);
        tex.AddMember("diffuse", material.textureIndices.diffuse, allocator);
        tex.AddMember("metallicRoughness", material.textureIndices.metallicRoughness, allocator);
        m.AddMember("textureIndices", tex, allocator);

        materials.PushBack(m, allocator);
    }
    d.AddMember("materials", materials, allocator);

    // Textures
    Value textures(kObjectType);
    textures.AddMember("count", static_cast<uint32_t>(scene.textures.size()), allocator);
    Value texturePaths(kArrayType);

    std::string baseDir = path.parent_path().string() + '/';
    uint32_t nTex = 0;
    for (const auto &texture : scene.textures) {
        std::string texName = "tex_" + std::to_string(nTex++) + ".png";
        std::string texPath = baseDir + texName;
        const auto texResult = texture.Save(texPath, false);
        if (texResult < 1) {
            LOG_INFO(LOADER, "Failed to save texture to disk: {}", texPath);
            texName = "";
        }
        texturePaths.PushBack(Value(texName.c_str(), allocator).Move(), allocator);
        nTex++;
    }
    textures.AddMember("paths", texturePaths, allocator);
    d.AddMember("textures", textures, allocator);

    // Meshes
    Value meshes(kArrayType);
    for (const auto &mesh : scene.meshes) {
        Value m(kObjectType);
        m.AddMember("name", Value(mesh.name.c_str(), allocator).Move(), allocator);
        m.AddMember("materialIndex", mesh.materialIndex, allocator);
        m.AddMember("startIndex", mesh.startIndex, allocator);
        m.AddMember("numIndices", mesh.numIndices, allocator);

        meshes.PushBack(m, allocator);
    }
    d.AddMember("meshes", meshes, allocator);

    // Vertex data
    // We save all vertex data to a single .bin file
    // In this file, we just save the counts and offsets
    // Order goes: index, positions, normals, texCoords, colors
    std::filesystem::path binPath = path;
    binPath.replace_extension(".bin");
    std::string binFilename = binPath.filename().string();

    uint64_t offset = 0;
    std::ofstream binStream(binPath, std::ios::binary | std::ios::trunc);
    if (!binStream.is_open()) {
        LOG_ERROR(LOADER, "Failed to open binary file for writing: {}", binPath.string());
        return JTX_ERROR_FILE_LOADING;
    }

    Value vertexData(kObjectType);
    vertexData.AddMember("binary", Value(binFilename.c_str(), allocator).Move(), allocator);

    const auto AddDataBlack = [&](const char *name, auto &data, Value &obj) {
        Value blockInfo(kObjectType);
        blockInfo.AddMember("offset", offset, allocator);
        blockInfo.AddMember("count", static_cast<uint64_t>(data.size()), allocator);
        obj.AddMember(Value(name, allocator).Move(), blockInfo, allocator);

        if (!data.empty()) {
            binStream.write(reinterpret_cast<const char *>(data.data()), data.size() * sizeof(data[0]));
        }
        if (!binStream.good()) {
            LOG_ERROR(LOADER, "Error while writing to binary file: {}", binPath.string());
            binStream.close();
            return false;
        }
        offset += data.size() * sizeof(data[0]);
        return true;
    };

    Value layout(kObjectType);
    if (!AddDataBlack("indices", scene.indices, layout)) return JTX_ERROR_FILE_WRITE;
    if (!AddDataBlack("positions", scene.positions, layout)) return JTX_ERROR_FILE_WRITE;
    if (!AddDataBlack("normals", scene.normals, layout)) return JTX_ERROR_FILE_WRITE;
    if (!AddDataBlack("texCoords", scene.texCoords, layout)) return JTX_ERROR_FILE_WRITE;
    if (!AddDataBlack("colors", scene.colors, layout)) return JTX_ERROR_FILE_WRITE;
    vertexData.AddMember("layout", layout, allocator);
    d.AddMember("vertexData", vertexData, allocator);

    StringBuffer buffer;
    PrettyWriter writer(buffer);
    d.Accept(writer);

    std::ofstream file(path);
    file << buffer.GetString();
    file.close();

    LOG_INFO(LOADER, "Scene exported to: {}", path.string());

    return JTX_SUCCESS;
}

}// namespace jtx