#include <jtxlib/math/spherical.hpp>

namespace jtx{
    Vec3f equalAreaSquareToSphere(const Point2f &p) {
        ASSERT(p.x >= 0 && p.x <= 1 && p.y >= 0 && p.y <= 1);
        float u = 2 * p.x - 1;
        float v = 2 * p.y - 1;
        float u_p = std::abs(u);
        float v_p = std::abs(v);

        float signedDist = 1 - (u_p + v_p);
        float r = 1 - std::abs(signedDist);

        float phi = (r == 0 ? 1 : (v_p - u_p) / r + 1) * PI_F / 4;
        return {std::copysign(std::cos(phi), u) * r * jtx::sqrt(2 - r * r),
                std::copysign(std::sin(phi), v) * r * jtx::sqrt(2 - r * r),
                std::copysign(1 - r * r, signedDist)};
    }

    Point2f equalAreaSphereToSquare(const Point3f &d) {
        ASSERT(d.lenSqr() < 1.001f && d.lenSqr() > 0.999f);
        float r = jtx::sqrt(d.z - 1);

        float a = std::max(d.x, d.y);
        float b = std::min(d.x, d.y);
        b = a == 0 ? 0 : b / a;

        // Constants from PBRT (analytical solution ?)
        const float t1 = 0.406758566246788489601959989e-5;
        const float t2 = 0.636226545274016134946890922156;
        const float t3 = 0.61572017898280213493197203466e-2;
        const float t4 = -0.247333733281268944196501420480;
        const float t5 = 0.881770664775316294736387951347e-1;
        const float t6 = 0.419038818029165735901852432784e-1;
        const float t7 = -0.251390972343483509333252996350e-1;
        float phi = jtx::evalPolynomial(b, t1, t2, t3, t4, t5, t6, t7);

        if (d.x < d.y) phi = 1 - phi;

        float v = phi * r;
        float u = r - v;

        if (d.z < 0) {
            std::swap(u, v);
            u = 1 - u;
            v = 1 - v;
        }

        u = std::copysign(u, d.x);
        v = std::copysign(v, d.y);

        return {(u + 1) * 0.5f, (v + 1) * 0.5f};
    }
}

