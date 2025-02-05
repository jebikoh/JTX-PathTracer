#include "scene.hpp"
#include "mesh.hpp"
#include <unordered_map>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

static constexpr int SCENE_MATERIAL_LIMIT = 64;

bool Scene::closestHit(const Ray &r, Interval t, Intersection &record) const {
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

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
        return;
    }

    std::string baseDir;
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        baseDir = path.substr(0, lastSlash + 1);
    } else {
        baseDir = "";
    }

    std::unordered_map<std::string, size_t> textureMap;
    std::unordered_map<std::string, size_t> materialMap;

    for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
        aiMaterial* aiMat = scene->mMaterials[i];

        aiString aiMatName;
        aiMat->Get(AI_MATKEY_NAME, aiMatName);
        std::string matName = aiMatName.C_Str();

        if (materialMap.contains(matName))
            continue;

        int texId = -1;
        if (aiMat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString texPath;
            if (aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                std::string fullTexPath = baseDir + texPath.C_Str();
                auto texIt = textureMap.find(fullTexPath);
                if (texIt != textureMap.end()) {
                    texId = texIt->second;
                } else {
                    TextureImage texture;
                    if (texture.load(fullTexPath.c_str())) {
                        std::cout << "Loaded texture: " << fullTexPath << std::endl;
                        textures.push_back(std::move(texture));
                        texId = textures.size() - 1;
                        textureMap[fullTexPath] = texId;
                    } else {
                        std::cerr << "Failed to load texture: " << fullTexPath << std::endl;
                    }
                }
            }
        }

        std::cout << "Loaded material: " << matName << std::endl;
        materials.push_back({.texId = texId});
        materialMap[matName] = materials.size() - 1;
    }

    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        aiMesh* aiMeshPtr = scene->mMeshes[m];

        size_t numVerts = aiMeshPtr->mNumVertices;
        auto finalVerts = new Vec3[numVerts];
        auto finalNormals = new Vec3[numVerts];

        Vec2f* finalUVs = nullptr;
        bool hasUV = (aiMeshPtr->mTextureCoords[0] != nullptr);
        if (hasUV) {
            finalUVs = new Vec2f[numVerts];
        }

        for (size_t i = 0; i < numVerts; i++) {
            aiVector3D v = aiMeshPtr->mVertices[i];
            finalVerts[i] = Vec3(v.x, v.y, v.z);

            if (aiMeshPtr->HasNormals()) {
                aiVector3D n = aiMeshPtr->mNormals[i];
                finalNormals[i] = Vec3(n.x, n.y, n.z);
            } else {
                finalNormals[i] = Vec3(0.0f, 1.0f, 0.0f);
            }

            if (hasUV) {
                aiVector3D uv = aiMeshPtr->mTextureCoords[0][i];
                finalUVs[i] = Vec2f(uv.x, uv.y);
            } else if(finalUVs) {
                finalUVs[i] = Vec2f(0.0f, 0.0f);
            }
        }

        size_t numTriangles = aiMeshPtr->mNumFaces;
        auto* finalIndices = new Vec3i[numTriangles];
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

        Material* meshMaterial = nullptr;
        if (aiMeshPtr->mMaterialIndex < scene->mNumMaterials) {
            aiMaterial* mat = scene->mMaterials[aiMeshPtr->mMaterialIndex];
            aiString matName;
            mat->Get(AI_MATKEY_NAME, matName);
            auto it = materialMap.find(matName.C_Str());
            if (it != materialMap.end()) {
                meshMaterial = &materials[it->second];
            }
        }
        if (!meshMaterial) {
            materials.push_back({.type = Material::DIFFUSE, .albedo = Color(1, 0.3, 0.5), .texId = -1});
            meshMaterial = &materials.back();
        }

        meshes.emplace_back(mName, finalIndices, numTriangles, finalVerts, numVerts, finalNormals, finalUVs, meshMaterial);

        int meshIndex = static_cast<int>(meshes.size()) - 1;
        for (size_t t = 0; t < numTriangles; t++) {
            Triangle tri;
            tri.index = static_cast<int>(t);
            tri.meshIndex = meshIndex;
            triangles.push_back(tri);
        }

        std::cout << "Loaded mesh: " << mName << std::endl;
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

    Light background = {
            .type      = Light::INFINITE,
            .intensity = Color(0.7, 0.8, 1.0),
            .scale     = 1.0};
    scene.lights.push_back(background);

    scene.materials.reserve(10);
    scene.spheres.reserve(10);

    // Objects & Materials
    scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Color(0.659, 0.659, 0.749)});
    scene.spheres.emplace_back(Vec3(0, -100.5, -1), 100, &scene.materials.back());

    const Vec3 GOLD_IOR = {0.15557, 0.42415, 1.3831};
    const Vec3 GOLD_K   = {-3.6024, -2.4721, -1.9155};

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

    Light background = {
            .type      = Light::INFINITE,
            .intensity = Color(0.7, 0.8, 1.0),
            .scale     = 1.0};
    scene.lights.push_back(background);

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

    std::cout << "Scene loaded with:" << std::endl;
    std::cout << " - " << scene.meshes.size() << " meshes" << std::endl;
    std::cout << " - " << scene.triangles.size() << " triangles" << std::endl;

    int numVertices = 0;
    for (const auto &mesh: scene.meshes) {
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
    const auto t           = Mat4::identity();
    const std::string path = "../src/assets/shaderball/shaderball.obj";
    auto scene             = createObjScene(path, t);

    scene.cameraProperties.center = Vec3(2.5, 16, 12);
    scene.cameraProperties.target = Vec3(0, 3, 0);
    scene.cameraProperties.yfov   = 40;

    Light background = {
            .type      = Light::INFINITE,
            .intensity = Color(0.7, 0.8, 1.0),
            .scale     = 1.0};
    scene.lights.push_back(background);

    const Vec3 GOLD_IOR = {0.15557, 0.42415, 1.3831};
    const Vec3 GOLD_K   = {-3.6024, -2.4721, -1.9155};

    // Base
    // *scene.meshes[0].material = {.type = Material::DIFFUSE, .texId = scene.meshes[0].material->texId};
    // Core
    // *scene.meshes[1].material = {.type = Material::CONDUCTOR, .IOR = GOLD_IOR, .k = GOLD_K, .alphaX = 0.05, .alphaY = 0.05, .texId = scene.meshes[1].material->texId};
    // Ground
    // *scene.meshes[2].material = {.type = Material::DIFFUSE, .texId = scene.meshes[2].material->texId};
    // Surface
    scene.materials.push_back({.type = Material::DIELECTRIC, .IOR = Vec3(1.5), .alphaX = 0.01, .alphaY = 0.01, .texId = scene.meshes[3].material->texId});
    scene.meshes[3].material = &scene.materials.back();
    // Bars
    // *scene.meshes[4].material = {.type = Material::DIFFUSE, .texId = scene.meshes[4].material->texId};

    return scene;
}

Scene createShaderBallSceneWithLight() {
    const auto t           = Mat4::identity();
    const std::string path = "../src/assets/shaderball.obj";
    auto scene             = createObjScene(path, t);

    scene.cameraProperties.center = Vec3(2.5, 16, 12);
    scene.cameraProperties.target = Vec3(0, 3, 0);
    scene.cameraProperties.yfov   = 40;

    //    Light background = {
    //            .type = Light::INFINITE,
    //            .intensity = Color(0,0,0),
    //            .scale = 1.0
    //    };
    //    scene.lights.push_back(background);

    // Single point light
    const Light point = {
            .type      = Light::POINT,
            .position  = Vec3(2.5, 16, 12),
            .intensity = WHITE,
            .scale     = 10};
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

    scene.cameraProperties.center = Vec3(0, 3, 8);
    scene.cameraProperties.target = Vec3(0, 0, 0);
    scene.cameraProperties.yfov   = 15;

    Light background = {
            .type      = Light::INFINITE,
            .intensity = Color(0.7, 0.8, 1.0),
            .scale     = 1.0};
    scene.lights.push_back(background);

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
