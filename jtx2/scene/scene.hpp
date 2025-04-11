#pragma once

#include "jtx.hpp"
#include "material.h"

// TODO: add TRS transform and basic scene graph
namespace jtx {

class Image8u;

/**
 * A mesh consists of a group of triangles that share a single material.
 *
 * A mesh's geometry is defined by its starting position in it's parent
 * scene's index buffer and the number of triangles it contains.
 */
struct Mesh {
    std::string name;
    uint32_t startIndex;
    uint32_t numIndices;
    uint32_t materialIndex;
};

/**
 * The scene struct holds all triangle (face) and vertex data for all the encapsulated meshes.
 * Each triangle is defined by 3 indices into the vertex data arrays: positions, normals, uvs,
 * and colors.
 *
 * Meshes are defined as a group of (typically adjacent) vertices that share a single material.
 * The scene holds an array of Mesh structs, which assign a single material to a group of triangles
 * by specifying a starting index and triangle count into the scene's index buffer.
 *
 * The scene also maintains a list of material indices per face so we can deduce what material
 * to use for shading after BVH traversal and intersection testing
 */
struct Scene {
    std::string name;

    // Triangle data
    std::vector<vec3u> indices;
    std::vector<uint32_t> materialIndices;

    // Vertex data
    std::vector<vec3> positions;
    std::vector<vec3> normals;
    std::vector<vec2u> uvs;
    std::vector<vec3> colors;

    // Materials & textures
    std::vector<Material> materials;
    std::vector<Image8u> textures;
    std::vector<Mesh> meshes;
};

}// namespace jtx