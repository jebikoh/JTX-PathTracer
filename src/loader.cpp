#include "loader.hpp"

#include "assimp/Importer.hpp"
#include "mesh.hpp"
#include "scene.hpp"

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <unordered_map>

static constexpr int SCENE_MATERIAL_LIMIT = 128;

void loadScene(const std::string &path, Scene &scene) {
    // Reserve space for materials
    if (scene.materials.capacity() < SCENE_MATERIAL_LIMIT) {
        scene.materials.reserve(SCENE_MATERIAL_LIMIT);
    }

    // Load scene & pre-process
    Assimp::Importer importer;
    const aiScene *assimpScene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_PreTransformVertices);
    if (!assimpScene || (assimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !assimpScene->mRootNode) {
        std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
        return;
    }

    const size_t lastSlash = path.find_last_of("/\\");
    std::string baseDir    = (lastSlash != std::string::npos) ? path.substr(0, lastSlash + 1) : "";

    // We process textures & materials first. These maps are used later to assign
    // textures to materials and materials to meshes
    std::unordered_map<std::string, size_t> textureMap;
    std::unordered_map<std::string, size_t> materialMap;

    // Load all embedded textures
    for (unsigned int i = 0; i < assimpScene->mNumTextures; ++i) {
        const aiTexture *aiTex = assimpScene->mTextures[i];
        std::string texKey     = aiTex->mFilename.C_Str();
        if (texKey.empty()) {
            texKey = "embedded_" + std::to_string(i);
        }

        TextureImage texture;
        bool loaded = false;

        // This indicates a compressed texture
        if (aiTex->mHeight == 0) {
            loaded = texture.load(reinterpret_cast<const unsigned char *>(aiTex->pcData),
                                  aiTex->mWidth, ImageFormat::AUTO);
        } else {
            loaded = texture.load(reinterpret_cast<const unsigned char *>(aiTex->pcData),
                                  aiTex->mWidth * aiTex->mHeight * 4, ImageFormat::AUTO);
        }

        if (loaded) {
            scene.textures.push_back(std::move(texture));
            textureMap[texKey] = scene.textures.size() - 1;
            std::cout << "Loaded embedded texture: " << texKey << std::endl;
        } else {
            std::cerr << "Failed to load embedded texture: " << texKey << std::endl;
        }
    }

    // Lambda to load textures from materials
    auto loadTexture = [&](const aiMaterial *aiMat, const aiTextureType type) -> int {
        if (aiMat->GetTextureCount(type) > 0) {
            aiString texPath;
            if (aiMat->GetTexture(type, 0, &texPath) == AI_SUCCESS) {
                std::string texName = texPath.C_Str();

                // Embedded textures start with '*'
                if (!texName.empty() && texName[0] == '*') {
                    const size_t embeddedIndex           = std::stoul(texName.substr(1));
                    const std::string embeddedName = "embedded_" + std::to_string(embeddedIndex);

                    if (textureMap.contains(embeddedName)) {
                        return textureMap[embeddedName];
                    }

                    std::cerr << "Embedded texture not found: " << embeddedName << std::endl;
                    return -1;
                }

                // Load texture from file
                const std::string fullTexPath = baseDir + texName;
                if (textureMap.contains(fullTexPath)) {
                    return textureMap[fullTexPath];
                }

                TextureImage texture;
                if (texture.load(fullTexPath.c_str())) {
                    scene.textures.push_back(std::move(texture));
                    int texId               = scene.textures.size() - 1;
                    textureMap[fullTexPath] = texId;
                    std::cout << "Loaded texture: " << fullTexPath << std::endl;
                    return texId;
                }

                std::cerr << "Failed to load texture: " << fullTexPath << std::endl;
            }
        }
        return -1;
    };

    // Load materials
    for (unsigned int i = 0; i < assimpScene->mNumMaterials; ++i) {
        aiMaterial *aiMat = assimpScene->mMaterials[i];

        aiString aiMatName;
        aiMat->Get(AI_MATKEY_NAME, aiMatName);
        std::string matName = aiMatName.C_Str();

        if (materialMap.contains(matName)) continue;

        int albedoTexId = loadTexture(aiMat, aiTextureType_DIFFUSE);

        int metallicRoughnessTexId;
        float metallic             = 0.0f;
        float roughness            = 1.0f;
        bool isMetallicRoughness   = false;

        if (aiMat->GetTextureCount(aiTextureType_GLTF_METALLIC_ROUGHNESS) > 0 ||
            aiMat->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS ||
            aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS) {
            isMetallicRoughness    = true;
            metallicRoughnessTexId = loadTexture(aiMat, aiTextureType_GLTF_METALLIC_ROUGHNESS);
        }

        Material mat;
        if (isMetallicRoughness) {
            mat.type = Material::METALLIC_ROUGHNESS;
            mat.albedoTexId = albedoTexId;
            mat.metallicRoughnessTexId = metallicRoughnessTexId;

            mat.albedo = Color::WHITE;
            mat.alphaX = metallic;
            mat.alphaY = roughness;
        } else {
            mat.type = Material::DIFFUSE;
            mat.albedoTexId = albedoTexId;
            mat.albedo = Color::WHITE;
            mat.metallicRoughnessTexId = -1;
        }

        scene.materials.push_back(mat);
        materialMap[matName] = scene.materials.size() - 1;
        std::cout << "Loaded material: " << matName << std::endl;
    }

    for (unsigned int m = 0; m < assimpScene->mNumMeshes; m++) {
        aiMesh *aiMeshPtr = assimpScene->mMeshes[m];

        size_t numVerts   = aiMeshPtr->mNumVertices;
        auto finalVerts   = new Vec3[numVerts];
        auto finalNormals = new Vec3[numVerts];

        Vec2f *finalUVs = nullptr;
        bool hasUV      = (aiMeshPtr->mTextureCoords[0] != nullptr);
        if (hasUV) {
            finalUVs = new Vec2f[numVerts];
        }

        for (size_t i = 0; i < numVerts; i++) {
            aiVector3D v  = aiMeshPtr->mVertices[i];
            finalVerts[i] = Vec3(v.x, v.y, v.z);

            if (aiMeshPtr->HasNormals()) {
                aiVector3D n    = aiMeshPtr->mNormals[i];
                finalNormals[i] = Vec3(n.x, n.y, n.z);
            } else {
                std::cout << "Missing normals" << std::endl;
                finalNormals[i] = Vec3(0.0f, 1.0f, 0.0f);
            }

            if (hasUV) {
                aiVector3D uv = aiMeshPtr->mTextureCoords[0][i];
                finalUVs[i]   = Vec2f(uv.x, uv.y);
            } else if (finalUVs) {
                finalUVs[i] = Vec2f(0.0f, 0.0f);
            }
        }

        size_t numTriangles = aiMeshPtr->mNumFaces;
        auto *finalIndices  = new Vec3i[numTriangles];
        for (size_t i = 0; i < numTriangles; i++) {
            aiFace face = aiMeshPtr->mFaces[i];
            if (face.mNumIndices != 3) {
                std::cerr << "Warning: mesh " << m << " has a face that isn't a triangle.\n";
                continue;
            }
            finalIndices[i] = Vec3i(face.mIndices[0], face.mIndices[1], face.mIndices[2]);
        }

        std::string mName = aiMeshPtr->mName.C_Str();
        if (mName.empty()) {
            mName = "mesh_" + std::to_string(m);
        }

        Material *meshMaterial = nullptr;
        if (aiMeshPtr->mMaterialIndex < assimpScene->mNumMaterials) {
            aiMaterial *mat = assimpScene->mMaterials[aiMeshPtr->mMaterialIndex];
            aiString matName;
            mat->Get(AI_MATKEY_NAME, matName);
            auto it = materialMap.find(matName.C_Str());
            if (it != materialMap.end()) {
                meshMaterial = &scene.materials[it->second];
            }
        }
        if (!meshMaterial) {
            scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Vec3(1, 0.3, 0.5), .albedoTexId = -1});
            meshMaterial = &scene.materials.back();
        }

        scene.meshes.emplace_back(mName, finalIndices, numTriangles, finalVerts, numVerts, finalNormals, finalUVs, meshMaterial);

        int meshIndex = static_cast<int>(scene.meshes.size()) - 1;
        for (size_t t = 0; t < numTriangles; t++) {
            Triangle tri;
            tri.index     = static_cast<int>(t);
            tri.meshIndex = meshIndex;
            scene.triangles.push_back(tri);
        }

        std::cout << "Loaded mesh: " << mName << std::endl;
    }
}