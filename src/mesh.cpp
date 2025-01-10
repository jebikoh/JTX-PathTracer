#define TINYOBJLOADER_IMPLEMENTATION

#include "mesh.hpp"
#include "tiny_obj_loader.h"

static Material DEFAULT_MAT;

std::vector<Mesh> loadMesh(const std::string &path) {
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = "./";// Path to material files

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(path, reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error();
        }
        exit(1);
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto &attrib = reader.GetAttrib();
    auto &shapes = reader.GetShapes();
    // auto& materials = reader.GetMaterials(); // Not used for now.

    std::vector<Mesh> meshes;
    meshes.reserve(shapes.size());

    for (size_t s = 0; s < shapes.size(); s++) {
        const auto &shape = shapes[s];

        // Count total triangles in this shape assuming each face is a triangle.
        const size_t triangleCount = shape.mesh.num_face_vertices.size();

        if (triangleCount == 0) continue;// Skip if no triangles.

        // Allocate arrays for this mesh.
        const int totalIndices = static_cast<int>(triangleCount * 3);
        auto verticesArray     = new Vec3[totalIndices];
        auto *normalsArray     = new Vec3[totalIndices];
        auto *indicesArray     = new int[totalIndices];

        size_t triangleIndex = 0;
        size_t index_offset  = 0;
        for (size_t f = 0; f < triangleCount; f++) {
            for (size_t v = 0; v < 3; v++) {
                const tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

                const float vx                   = attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 0];
                const float vy                   = attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 1];
                const float vz                   = attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 2];
                verticesArray[triangleIndex + v] = Vec3(vx, vy, vz);

                if (idx.normal_index >= 0) {
                    const float nx                  = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 0];
                    const float ny                  = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 1];
                    const float nz                  = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 2];
                    normalsArray[triangleIndex + v] = Vec3(nx, ny, nz);
                } else {
                    normalsArray[triangleIndex + v] = Vec3(0.0f, 0.0f, 0.0f);
                }

                indicesArray[triangleIndex + v] = static_cast<int>(triangleIndex + v);
            }
            triangleIndex += 3;
            index_offset += 3;
        }

        meshes.emplace_back(reinterpret_cast<const Vec3i *>(indicesArray), verticesArray, normalsArray, DEFAULT_MAT);
    }

    return meshes;
}
