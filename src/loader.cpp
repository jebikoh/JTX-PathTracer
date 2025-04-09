#include "loader.hpp"

#include "logger.hpp"
#include "material.hpp"
#include "scene.hpp"

#include <fmt/core.h>
#include <rapidobj.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

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
    const size_t lastSlash = path.find_last_of("/");
    scene.name = path.substr(lastSlash + 1, lastDot);

    if (ext == ".obj") { return loadObj(path, scene); }
    if (ext == ".gltf" || ext == ".glb") { return loadGltf(path, scene); }

    LOG_ERROR("File extension not supported: ", ext);
    return false;
}

bool loadObj(const std::string &path, Scene &scene) {
    LOG_INFO("Loading OBJ file: {}", path);
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
        newMesh.name     = shape.name.empty() ? std::string("mesh_") + std::to_string(scene.meshes.size()) : shape.name;
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

std::optional<TextureImage> detail::loadTexture(fastgltf::Asset &asset, fastgltf::Image &image) {
    std::optional<TextureImage> result;
    std::visit(fastgltf::visitor{
                       [](auto &arg) {},
                       [&](fastgltf::sources::URI &filePath) {
                           assert(filePath.fileByteOffset == 0);// We don't support offsets with stbi.
                           assert(filePath.uri.isLocalPath());  // We're only capable of loading local files.
                           const std::string path(filePath.uri.path().begin(), filePath.uri.path().end());
                           result = TextureImage(path.c_str());
                       },
                       [&](fastgltf::sources::Array &vector) {
                           result = TextureImage(reinterpret_cast<const unsigned char *>(vector.bytes.data()), static_cast<size_t>(vector.bytes.size()), ImageFormat::STBI);
                       },
                       [&](fastgltf::sources::BufferView &view) {
                           auto &bufferView = asset.bufferViews[view.bufferViewIndex];
                           auto &buffer     = asset.buffers[bufferView.bufferIndex];
                           std::visit(fastgltf::visitor{
                                              [](auto &arg) {},
                                              [&](fastgltf::sources::Array &vector) {
                                                  result = TextureImage(reinterpret_cast<const unsigned char *>(vector.bytes.data() + bufferView.byteOffset), static_cast<int>(bufferView.byteLength), ImageFormat::STBI);
                                              }},
                                      buffer.data);
                       },
               },
               image.data);
    return result;
}

bool loadGltf(const std::filesystem::path &path, Scene &scene) {
    LOG_INFO("Loading glTF file: {}", path.string());

    constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadExternalBuffers;
    fastgltf::Asset gltf;
    fastgltf::Parser parser;

    // LOAD
    auto data = fastgltf::GltfDataBuffer::FromPath(path);
    if (!data) {
        LOG_ERROR("Failed to load glTF file: {}", path.string());
        return false;
    }

    // DETERMINE GLTF TYPE AND PARSE
    const auto type = fastgltf::determineGltfFileType(data.get());
    if (type == fastgltf::GltfType::glTF) {
        auto load = parser.loadGltf(data.get(), path.parent_path(), gltfOptions);
        if (load) gltf = std::move(load.get());
        else {
            LOG_ERROR("Failed to parse glTF file: {}", fastgltf::getErrorMessage(load.error()));
            return false;
        }
    } else if (type == fastgltf::GltfType::GLB) {
        auto load = parser.loadGltfBinary(data.get(), path.parent_path(), gltfOptions);
        if (load) gltf = std::move(load.get());
        else {
            LOG_ERROR("Failed to parse GLB file: {}", fastgltf::getErrorMessage(load.error()));
            return false;
        }
    } else {
        LOG_ERROR("Failed to determine GLTF type: {}", path.string());
        return false;
    }

    // TEXTURES
    size_t textureIndex = 0;
    std::unordered_map<size_t, size_t> indexMap;
    for (fastgltf::Image &image: gltf.images) {
        std::optional<TextureImage> img = detail::loadTexture(gltf, image);

        std::string imgName;
        if (image.name.empty()) {
            imgName = "texture_" + std::to_string(scene.textures.size());
        } else {
            imgName = image.name;
        }

        if (img.has_value()) {
            indexMap[textureIndex] = scene.textures.size();
            scene.textures.emplace_back(std::move(img.value()));
            LOG_INFO("Loaded texture: {}", imgName);
        } else {
            indexMap[textureIndex] = -1;
            LOG_ERROR("Failed to load texture: {}", imgName);
        }
        textureIndex++;
    }

    // MATERIALS
    scene.materials.reserve(gltf.materials.size());
    std::unordered_map<size_t, size_t> materials;
    for (fastgltf::Material &material : gltf.materials) {
        Material mat;
        mat.mType = Material::METALLIC_ROUGHNESS;
        mat.parameters.albedo = Vec3(material.pbrData.baseColorFactor.data());
        mat.parameters.metallic = material.pbrData.metallicFactor;
        mat.parameters.roughness = material.pbrData.roughnessFactor;

        if (material.pbrData.baseColorTexture.has_value()) {
            size_t index = gltf.textures[material.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
            mat.textureIndices.albedo = indexMap[index];
        }

        if (material.pbrData.metallicRoughnessTexture.has_value()) {
            size_t index = gltf.textures[material.pbrData.metallicRoughnessTexture.value().textureIndex].imageIndex.value();
            mat.textureIndices.mr = indexMap[index];
        }
        scene.materials.emplace_back(std::move(mat));
    }

    // SCENE DATA
    for (fastgltf::Mesh &mesh : gltf.meshes) {
        std::string prefix = mesh.name.c_str();
        prefix += "_";

        int meshIndex = 0;
        for (auto &&p : mesh.primitives) {
            Mesh newMesh;
            newMesh.name = prefix + std::to_string(meshIndex);
            LOG_INFO("Loading mesh: {}", newMesh.name);

            // INDICES
            {
                fastgltf::Accessor &accessor = gltf.accessors[p.indicesAccessor.value()];
                newMesh.indices.reserve(accessor.count / 3);
                std::vector<uint32_t> indices(accessor.count);

                // fastgltf iterates one-by-one, but we store our
                fastgltf::iterateAccessor<std::uint32_t>(gltf, accessor,
                    [&](const std::uint32_t idx) {
                        indices.push_back(idx);
                    });

                for (size_t i = 0; i < indices.size(); i += 3) {
                    newMesh.indices.emplace_back(&indices[i]);
                }
            }

            // VERTICES
            {
                fastgltf::Accessor &accessor = gltf.accessors[p.findAttribute("POSITION")->accessorIndex];
                newMesh.vertices.reserve(accessor.count);
                fastgltf::iterateAccessor<fastgltf::math::fvec3>(gltf, accessor,
                    [&](const fastgltf::math::fvec3 &v) {
                        newMesh.vertices.emplace_back(v.data());
                    });
            }

            // NORMALS
            {
                auto normals = p.findAttribute("NORMAL");
                if (normals != p.attributes.end()) {
                    auto &accessor = gltf.accessors[normals->accessorIndex];
                    fastgltf::iterateAccessor<fastgltf::math::fvec3>(gltf, accessor,
                        [&](const fastgltf::math::fvec3 &v) {
                            newMesh.normals.emplace_back(v.data());
                        });
                }
            }

            // UVS
            {
                auto uvs = p.findAttribute("TEXCOORD_0");
                if (uvs != p.attributes.end()) {
                    auto &accessor = gltf.accessors[uvs->accessorIndex];
                    fastgltf::iterateAccessor<fastgltf::math::fvec2>(gltf, accessor,
                        [&](const fastgltf::math::fvec2 &v) {
                            newMesh.uvs.emplace_back(v.data());
                        });
                }
            }

            // COLORS
            {
                auto colors = p.findAttribute("COLOR_0");
                if (colors != p.attributes.end()) {
                    auto &accessor = gltf.accessors[colors->accessorIndex];
                    fastgltf::iterateAccessor<fastgltf::math::fvec4>(gltf, accessor,
                        [&](const fastgltf::math::fvec4 &v) {
                            newMesh.colors.emplace_back(v.data());
                        });
                }
            }

            if (p.materialIndex.has_value()) {
                newMesh.material = &scene.materials[p.materialIndex.value()];
            } else {
                newMesh.material = &scene.materials.front();
            }

            meshIndex = scene.meshes.size();
            for (int i = 0; i < newMesh.indices.size(); i++) {
                Triangle tri;
                tri.index     = i;
                tri.meshIndex = meshIndex;
                scene.triangles.emplace_back(tri);
            }

            LOG_INFO("Loaded mesh {} with {} vertices and {} faces", newMesh.name, newMesh.indices.size(), newMesh.vertices.size());

            scene.meshes.emplace_back(std::move(newMesh));
            meshIndex++;
        }
    }

    LOG_INFO("Loaded scene: {}", scene.name);
    return true;
}