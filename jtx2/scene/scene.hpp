#pragma once

#include <image.hpp>
#include <jtx.hpp>
#include <material.h>
#include <util/aabb.hpp>
#include <util/sampling.hpp>

// TODO: add TRS transform and basic scene graph
namespace jtx {

struct SceneUpdate {
    int32_t materialIndex = -1;
    int32_t objectIndex   = -1;
};

struct Triangle {
    int triangleIndex;
    AABB bbox;

    vec3 Centroid() const { return 0.5f * (bbox.pmin + bbox.pmax); }
};

/**
 * A mesh consists of a group of triangles that share a single material.
 *
 * A mesh's geometry is defined by its starting position in its parent
 * scene's index buffer and the number of triangles it contains.
 */
struct Mesh {
    std::string name;
    uint32_t startIndex;
    uint32_t numIndices;// In triangles, not vertices
    uint32_t materialIndex;
};

struct LightSample {
    vec3 position;
    vec3 normal;
    vec3 emission;
    vec3 direction;
    float distance;
    float pdf;
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

    // Camera
    struct CameraSettings {
        vec3 position{};
        vec3 target{};
        vec3 up{};

        // Focal length (mm): distance from center of lens to convergence point
        //                    (i.e. distance from lens to image sensor)
        //                    shorter length -> wider FOV
        //                    larger length  -> narrow FOV
        float focalLength = 0.05f;// 50mm
        // Sensor width (mm): physical width of image sensor.
        float sensorWidth = 0.036f;// 36mm
        // Focal distance (m): distance from lens to point in plane of perfect focus
        float focalDistance = 10.0f;

        // Enable/disable depth of field
        bool bEnableDof = false;
        // F-Stop: ratio of len's focalLength to diameter of aperture
        //         larger f-stop -> smaller aperture -> deeper depth of field
        //         smaller f-stop -> larger aperture -> shallower depth of field
        float fStop = 2.8f;// Aperture size (f-stop)

        // TODO: these should be moved elsewhere -- not camera related
        // - Exposure (ISO100)
        // - Tone mapping: ACES, Reinhard, Uncharted2
        // - Display device: sRGB, Display P3
    } cameraSettings;

    // Triangle data
    std::vector<vec3u> indices;
    std::vector<uint32_t> materialIndices;

    // Vertex data
    std::vector<vec3> positions;
    std::vector<vec3> normals;
    std::vector<vec2> texCoords;
    std::vector<vec3> colors;

    // Materials & textures
    std::vector<Material> materials;
    std::vector<Image8u> textures;
    std::vector<Mesh> meshes;

    // Skybox color
    vec3 skyColor;

    // Lights
    std::vector<uint32_t> emissiveTriangles;// Indices of triangles that are emissive

    size_t AddMaterial(const Material &material) {
        materials.push_back(material);
        return materials.size() - 1;
    }

    void UpdateMeshMaterial(const size_t meshIndex, const size_t materialIndex) {
        auto &mesh         = meshes[meshIndex];
        mesh.materialIndex = materialIndex;
        for (int i = mesh.startIndex; i < mesh.startIndex + mesh.numIndices; ++i) {
            materialIndices[i] = materialIndex;
        }
    }

    void Destroy() {
        indices.clear();
        indices.shrink_to_fit();
        materialIndices.clear();
        materialIndices.shrink_to_fit();

        positions.clear();
        positions.shrink_to_fit();
        normals.clear();
        normals.shrink_to_fit();
        texCoords.clear();
        texCoords.shrink_to_fit();
        colors.clear();
        colors.shrink_to_fit();

        materials.clear();
        materials.shrink_to_fit();

        for (auto &texture: textures) {
            texture.Destroy();
        }
        textures.clear();
        textures.shrink_to_fit();

        meshes.clear();
        meshes.shrink_to_fit();

        emissiveTriangles.clear();
        emissiveTriangles.shrink_to_fit();
    }

    std::vector<Triangle> GetTriangles() const {
        std::vector<Triangle> triangles;

        for (int i = 0; i < indices.size(); ++i) {
            Triangle tri;
            const vec3u idx   = indices[i];
            tri.triangleIndex = i;
            tri.bbox          = AABB(positions[idx.x], positions[idx.y], positions[idx.z]);
            triangles.emplace_back(tri);
        }

        return triangles;
    }

    void SampleTriangle(const vec2 &s, const uint32_t index, LightSample &sample) const {
        const vec3 b = SampleUniformTriangle(s);

        const vec3u tri = indices[index];

        const vec3 p0   = positions[tri.x];
        const vec3 p1   = positions[tri.y];
        const vec3 p2   = positions[tri.z];
        sample.position = b.x * p0 + b.y * p1 + b.z * p2;

        const vec3 n  = Normalize(Cross(p1 - p0, p2 - p0));
        const vec3 n0 = normals[tri.x];
        const vec3 n1 = normals[tri.y];
        const vec3 n2 = normals[tri.z];
        const vec3 sn = b.x * n0 + b.y * n1 + b.z * n2;
        sample.normal = Dot(n, sn) < 0.0f ? -n : n;

        sample.emission = materials[materialIndices[index]].parameters.emission;
        sample.pdf      = 1.0f / (0.5f * Cross(p1 - p0, p2 - p0).Length());
    }

    /**
     * Calculates the probability density function (PDF) of sampling a triangle.
     * @param index triangle index
     * @return pdf of sampling a point on the triangle
     */
    float TrianglePDF(const uint32_t index) const {
        const vec3u tri = indices[index];
        const vec3 p0   = positions[tri.x];
        const vec3 p1   = positions[tri.y];
        const vec3 p2   = positions[tri.z];
        return 1.0f / (0.5f * Cross(p1 - p0, p2 - p0).Length());
    }
};

}// namespace jtx