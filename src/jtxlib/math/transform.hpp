/**
 * This class is very similar to Mat4, but takes the extra step of calculating the inverse.
 */
#pragma once

#include <jtxlib/math/mat4.hpp>
#include <jtxlib/math/vec3.hpp>
#include <jtxlib/math/vecmath.hpp>


namespace jtx {
    class Transform {
    public:
        //region Constructors
        Transform() : m(Mat4(1.0f)), mInv(Mat4(1.0f)) {};

        Transform(const Mat4 &m, const Mat4 &mInv) : m(m), mInv(mInv) {};

        explicit Transform(const Mat4 &m) : m(m) {
            std::optional<Mat4> inv = m.inverse();
            if (inv.has_value()) {
                mInv = inv.value();
            } else {
                float nan = std::numeric_limits<float>::signaling_NaN();
                for (int i = 0; i < 4; i++) {
                    for (int j = 0; j < 4; j++) {
                        mInv[i][j] = nan;
                    }
                }
            }
        }

        explicit Transform(const float mat[4][4]) : Transform(Mat4(mat)) {};

        ~Transform() = default;
        //endregion

        //region Getters
        [[nodiscard]] const Mat4 &getMatrix() const {
            return m;
        }

        [[nodiscard]] const Mat4 &getInverseMatrix() const {
            return mInv;
        }
        //endregion

        //region Operators
        bool operator==(const Transform &other) const {
            return m == other.m;
        }

        bool operator!=(const Transform &other) const {
            return m != other.m;
        }
        //endregion

        //region Methods
        [[nodiscard]] bool isIdentity() const { return m.isIdentity(); }

        [[nodiscard]] inline Transform inverse() const {
            return {mInv, m};
        }

        [[nodiscard]] inline Transform transpose() const {
            return {m.transpose(), mInv.transpose()};

        }

        // Need to differentiate because points and normals are just typedefs

        JTX_NUM_ONLY_T
        inline Point3<T> applyToPoint(const Point3<T> &p) const {
            T xp = m[0][0] * p.x + m[0][1] * p.y + m[0][2] * p.z + m[0][3];
            T yp = m[1][0] * p.x + m[1][1] * p.y + m[1][2] * p.z + m[1][3];
            T zp = m[2][0] * p.x + m[2][1] * p.y + m[2][2] * p.z + m[2][3];
            T wp = m[3][0] * p.x + m[3][1] * p.y + m[3][2] * p.z + m[3][3];

            if (wp == 1) {
                return {xp, yp, zp};
            } else {
                return Point3f(xp, yp, zp) / wp;
            }
        }

        JTX_NUM_ONLY_T
        inline Point3<T> applyInverseToPoint(const Point3<T> &p) const {
            T xp = mInv[0][0] * p.x + mInv[0][1] * p.y + mInv[0][2] * p.z + mInv[0][3];
            T yp = mInv[1][0] * p.x + mInv[1][1] * p.y + mInv[1][2] * p.z + mInv[1][3];
            T zp = mInv[2][0] * p.x + mInv[2][1] * p.y + mInv[2][2] * p.z + mInv[2][3];
            T wp = mInv[3][0] * p.x + mInv[3][1] * p.y + mInv[3][2] * p.z + mInv[3][3];

            if (wp == 1) {
                return {xp, yp, zp};
            } else {
                return Point3f(xp, yp, zp) / wp;
            }
        }

        JTX_NUM_ONLY_T
        inline Vec3<T> applyToVec(const Vec3<T> &v) const {
            return {m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
                    m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
                    m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z};
        }

        JTX_NUM_ONLY_T
        inline Vec3<T> applyInverseToVec(const Vec3<T> &v) const {
            return {mInv[0][0] * v.x + mInv[0][1] * v.y + mInv[0][2] * v.z,
                    mInv[1][0] * v.x + mInv[1][1] * v.y + mInv[1][2] * v.z,
                    mInv[2][0] * v.x + mInv[2][1] * v.y + mInv[2][2] * v.z};
        }

        [[nodiscard]] inline Normal3f applyToNormal(const Normal3f &n) const {
            return {mInv[0][0] * n.x + mInv[1][0] * n.y + mInv[2][0] * n.z,
                    mInv[0][1] * n.x + mInv[1][1] * n.y + mInv[2][1] * n.z,
                    mInv[0][2] * n.x + mInv[1][2] * n.y + mInv[2][2] * n.z};
        }

        [[nodiscard]] inline Normal3f applyInverseToNormal(const Normal3f &n) const {
            return {m[0][0] * n.x + m[1][0] * n.y + m[2][0] * n.z,
                    m[0][1] * n.x + m[1][1] * n.y + m[2][1] * n.z,
                    m[0][2] * n.x + m[1][2] * n.y + m[2][2] * n.z};
        }
        //endregion

        //region Static methods
        static Transform translate(const float delta) {
            return {jtx::translate(delta), jtx::translate(-delta)};
        }

        static Transform translate(float x, float y, float z) {
            return {jtx::translate(x, y, z), jtx::translate(-x, -y, -z)};
        }

        static Transform translate(const Vec3f &v) {
            return {jtx::translate(v.x, v.y, v.z), jtx::translate(-v.x, -v.y, -v.z)};
        }

        static Transform scale(float sx, float sy, float sz) {
            ASSERT(sx != 0 && sy != 0 && sz != 0);
            return {jtx::scale(sx, sy, sz), jtx::scale(1 / sx, 1 / sy, 1 / sz)};
        }

        static Transform rotateX(float theta) {
            auto m = jtx::rotateX(theta);
            return {m, m.transpose()};
        }

        static Transform rotateY(float theta) {
            auto m = jtx::rotateY(theta);
            return {m, m.transpose()};
        }

        static Transform rotateZ(float theta) {
            auto m = jtx::rotateZ(theta);
            return {m, m.transpose()};
        }

        static Transform rotate(float sinTheta, float cosTheta, const Vec3f& axis) {
            Mat4 m = jtx::rotate(sinTheta, cosTheta, axis);
            return {m, m.transpose()};
        }

        static Transform rotate(float theta, const Vec3f& axis) {
            Mat4 m = jtx::rotate(theta, axis);
            return {m, m.transpose()};
        }

        static Transform rotateFromTo(const Vec3f& from, const Vec3f& to) {
            Mat4 m = jtx::rotateFromTo(from, to);
            return {m, m.transpose()};
        }

        // Note to self, PBRT calculates worldFromCamera, then inverts it
        // Old JTX calculated cameraFromWorld (and also was row-major)
        static Transform lookAt(const Point3f &pos, const Point3f &target, const Vec3f &up) {
            Vec3f dir = jtx::normalize(target - pos);
            ASSERT((jtx::cross(jtx::normalize(up), dir)).len() != 0);
            Vec3f right = jtx::normalize(jtx::cross(jtx::normalize(up), dir));
            Vec3f newUp = jtx::cross(dir, right);

            Mat4 m(right.x, newUp.x, dir.x, pos.x,
                   right.y, newUp.y, dir.y, pos.y,
                   right.z, newUp.z, dir.z, pos.z,
                   0, 0, 0, 1);
            return {jtx::invert(m), m};
        }
        //endregion

    private:
        Mat4 m;
        Mat4 mInv;
    };

    inline Transform inverse(const Transform &t) {
        return t.inverse();
    }

    inline Transform transpose(const Transform &t) {
        return t.transpose();
    }
}