#include <jtxlib/math/mat4.hpp>
#include <jtxlib/math/math.hpp>
#include <jtxlib/math/vec3.hpp>
#include <jtxlib/math/vecmath.hpp>

namespace jtx {
    float jtx::Mat4::determinant() const {
        float s0 = jtx::dop(data[0][0], data[1][1], data[1][0], data[0][1]);
        float s1 = jtx::dop(data[0][0], data[1][2], data[1][0], data[0][2]);
        float s2 = jtx::dop(data[0][0], data[1][3], data[1][0], data[0][3]);

        float s3 = jtx::dop(data[0][1], data[1][2], data[1][1], data[0][2]);
        float s4 = jtx::dop(data[0][1], data[1][3], data[1][1], data[0][3]);
        float s5 = jtx::dop(data[0][2], data[1][3], data[1][2], data[0][3]);

        float c0 = jtx::dop(data[2][0], data[3][1], data[3][0], data[2][1]);
        float c1 = jtx::dop(data[2][0], data[3][2], data[3][0], data[2][2]);
        float c2 = jtx::dop(data[2][0], data[3][3], data[3][0], data[2][3]);

        float c3 = jtx::dop(data[2][1], data[3][2], data[3][1], data[2][2]);
        float c4 = jtx::dop(data[2][1], data[3][3], data[3][1], data[2][3]);
        float c5 = jtx::dop(data[2][2], data[3][3], data[3][2], data[2][3]);

        return jtx::dop(s0, c5, s1, c4) + jtx::dop(s2, c3, -s3, c2) +
               jtx::dop(s5, c0, s4, c1);
    }


    std::optional<jtx::Mat4> jtx::Mat4::inverse() const {
        float s0 = jtx::dop(data[0][0], data[1][1], data[1][0], data[0][1]);
        float s1 = jtx::dop(data[0][0], data[1][2], data[1][0], data[0][2]);
        float s2 = jtx::dop(data[0][0], data[1][3], data[1][0], data[0][3]);

        float s3 = jtx::dop(data[0][1], data[1][2], data[1][1], data[0][2]);
        float s4 = jtx::dop(data[0][1], data[1][3], data[1][1], data[0][3]);
        float s5 = jtx::dop(data[0][2], data[1][3], data[1][2], data[0][3]);

        float c0 = jtx::dop(data[2][0], data[3][1], data[3][0], data[2][1]);
        float c1 = jtx::dop(data[2][0], data[3][2], data[3][0], data[2][2]);
        float c2 = jtx::dop(data[2][0], data[3][3], data[3][0], data[2][3]);

        float c3 = jtx::dop(data[2][1], data[3][2], data[3][1], data[2][2]);
        float c4 = jtx::dop(data[2][1], data[3][3], data[3][1], data[2][3]);
        float c5 = jtx::dop(data[2][2], data[3][3], data[3][2], data[2][3]);

        float det = jtx::dop(s0, c5, s1, c4) + jtx::dop(s2, c3, -s3, c2) +
                    jtx::dop(s5, c0, s4, c1);
        if (det == 0.0f) {
            return {};
        }

        float invDet = 1.0f / det;

        return {{invDet * jtx::innerProd(data[1][1], c5, data[1][3], c3, -data[1][2], c4),
                 invDet * jtx::innerProd(-data[0][1], c5, data[0][2], c4, -data[0][3], c3),
                 invDet * jtx::innerProd(data[3][1], s5, data[3][3], s3, -data[3][2], s4),
                 invDet * jtx::innerProd(-data[2][1], s5, data[2][2], s4, -data[2][3], s3),
                 invDet * jtx::innerProd(-data[1][0], c5, data[1][2], c2, -data[1][3], c1),
                 invDet * jtx::innerProd(data[0][0], c5, data[0][3], c1, -data[0][2], c2),
                 invDet * jtx::innerProd(-data[3][0], s5, data[3][2], s2, -data[3][3], s1),
                 invDet * jtx::innerProd(data[2][0], s5, data[2][3], s1, -data[2][2], s2),
                 invDet * jtx::innerProd(data[1][0], c4, data[1][3], c0, -data[1][1], c2),
                 invDet * jtx::innerProd(-data[0][0], c4, data[0][1], c2, -data[0][3], c0),
                 invDet * jtx::innerProd(data[3][0], s4, data[3][3], s0, -data[3][1], s2),
                 invDet * jtx::innerProd(-data[2][0], s4, data[2][1], s2, -data[2][3], s0),
                 invDet * jtx::innerProd(-data[1][0], c3, data[1][1], c1, -data[1][2], c0),
                 invDet * jtx::innerProd(data[0][0], c3, data[0][2], c0, -data[0][1], c1),
                 invDet * jtx::innerProd(-data[3][0], s3, data[3][1], s1, -data[3][2], s0),
                 invDet * jtx::innerProd(data[2][0], s3, data[2][2], s0, -data[2][1], s1)}};
    }

    Mat4 rotate(float sinTheta, float cosTheta, const Vec3f& axis) {
        Vec3f a = jtx::normalize(axis);
        Mat4 m;
        // Compute rotation of first basis vector
        m[0][0] = a.x * a.x + (1 - a.x * a.x) * cosTheta;
        m[0][1] = a.x * a.y * (1 - cosTheta) - a.z * sinTheta;
        m[0][2] = a.x * a.z * (1 - cosTheta) + a.y * sinTheta;
        m[0][3] = 0;
        // Compute rotation of the second basis vector
        m[1][0] = a.x * a.y * (1 - cosTheta) + a.z * sinTheta;
        m[1][1] = a.y * a.y + (1 - a.y * a.y) * cosTheta;
        m[1][2] = a.y * a.z * (1 - cosTheta) - a.x * sinTheta;
        m[1][3] = 0;

        m[2][0] = a.x * a.z * (1 - cosTheta) - a.y * sinTheta;
        m[2][1] = a.y * a.z * (1 - cosTheta) + a.x * sinTheta;
        m[2][2] = a.z * a.z + (1 - a.z * a.z) * cosTheta;
        m[2][3] = 0;

        m[3][0] = 0;
        m[3][1] = 0;
        m[3][2] = 0;
        m[3][3] = 1;

        return m;
    }

    Mat4 rotate(float theta, const jtx::Vec3f& axis) {
        float sinTheta = std::sin(jtx::radians(theta));
        float cosTheta = std::cos(jtx::radians(theta));
        return jtx::rotate(sinTheta, cosTheta, axis);
    }

    Mat4 rotateFromTo(const Vec3f &from, const Vec3f &to) {
        // Intermediate vector
        Vec3f refl;
        if (std::abs(from.x) < 0.72f && std::abs(to.x) < 0.72f) {
            refl = Vec3f(1, 0, 0);
        } else if (std::abs(from.y) < 0.72f && std::abs(to.y) < 0.72f) {
            refl = Vec3f(0, 1, 0);
        } else {
            refl = Vec3f(0, 0, 1);
        }
        Vec3f u = refl - from;
        Vec3f v = refl - to;

        Mat4 r;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                float uu = jtx::dot(u, u);
                float vv = jtx::dot(v, v);
                r[i][j] = ((i == j) ? 1.0f : 0.0f) -
                          (2 / uu) * u[i] * u[j] -
                          (2 / vv) * v[i] * v[j] +
                          (4 * jtx::dot(u, v)) / (uu * vv) * u[i] * v[j];
            }
        }

        return r;
    }

    std::string to_string(const Mat4 &mat) {
        {
            std::string res = "Mat4(\n";
            for (int i = 0; i < 4; ++i) {
                res += "  [";
                for (int j = 0; j < 4; ++j) {
                    res += std::to_string(mat[i][j]);
                    if (j != 3) res += ", ";
                }
                res += "]";
                if (i != 3) res += ",";
                res += "\n";
            }
            res += ")";
            return res;
        }
    }
}
