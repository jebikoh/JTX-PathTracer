#include <jtxlib/math/mat4.hpp>
#include "tconstants.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

TEST_CASE("Mat4 default constructor is identity matrix", "[Mat4]") {
    jtx::Mat4 mat{1.0f};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++)
            REQUIRE(mat[i][j] == (i == j ? 1.0f : 0.0f));
    }

    bool is_identity = mat.isIdentity();
    REQUIRE(is_identity);
}

TEST_CASE("Mat4 copy constructor", "[Mat4]") {
    jtx::Mat4 mat{1.0f};
    jtx::Mat4 matCopy(mat);

    REQUIRE(matCopy == mat);
    REQUIRE(matCopy.isIdentity());
}

TEST_CASE("Mat4 individual float constructor", "[Mat4]") {
    jtx::Mat4 mat{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    REQUIRE(mat.isIdentity());
}

TEST_CASE("Mat4 float array constructor", "[Mat4]") {
    float data[4][4] = {{1.0f, 0.0f, 0.0f, 0.0f},
                        {0.0f, 1.0f, 0.0f, 0.0f},
                        {0.0f, 0.0f, 1.0f, 0.0f},
                        {0.0f, 0.0f, 0.0f, 1.0f}};
    jtx::Mat4 mat{data};
    REQUIRE(mat.isIdentity());
}

TEST_CASE("Mat4 (in)equality operators", "[Mat4]") {
    jtx::Mat4 m1{};
    jtx::Mat4 m2{};
    m2[0][0] = 2.0f;

    jtx::Mat4 m3{};

    REQUIRE(m1 != m2);
    REQUIRE(m1 == m3);
}

TEST_CASE("Mat4 zero matrix", "[Mat4]") {
    jtx::Mat4 mat = jtx::Mat4{};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++)
            REQUIRE(mat[i][j] == 0.0f);
    }
}

TEST_CASE("Mat4 diagonal", "[Mat4]") {
    jtx::Mat4 diag = jtx::Mat4::diagonal(2, 2, 2, 2);
    jtx::Mat4 diag_ref{2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2};
    REQUIRE(diag == diag_ref);
}

TEST_CASE("Mat4 addition operators", "[Mat4]") {
    jtx::Mat4 mat1{1.0f};
    jtx::Mat4 mat2{1.0f};
    float scalar = 2.0f;

    jtx::Mat4 sum = mat1 + mat2;
    REQUIRE(sum == jtx::Mat4::diagonal(2, 2, 2, 2));

    jtx::Mat4 sum2 = sum + scalar;
    jtx::Mat4 sum2_ref{4, 2, 2, 2, 2, 4, 2, 2, 2, 2, 4, 2, 2, 2, 2, 4};
    REQUIRE(sum2 == sum2_ref);

    jtx::Mat4 sum3 = scalar + sum2;
    jtx::Mat4 sum3_ref{6, 4, 4, 4, 4, 6, 4, 4, 4, 4, 6, 4, 4, 4, 4, 6};
    REQUIRE(sum3 == sum3_ref);
}

TEST_CASE("Mat4 subtraction operators", "[Mat4]") {
    jtx::Mat4 mat1{};
    jtx::Mat4 mat2{};
    float scalar = 2.0f;

    jtx::Mat4 diff = mat1 - mat2;
    REQUIRE(diff == jtx::Mat4{});

    jtx::Mat4 diff2 = diff - scalar;
    jtx::Mat4 diff2_ref{-2.0, -2.0, -2.0, -2.0, -2.0, -2.0, -2.0, -2.0, -2.0, -2.0, -2.0, -2.0, -2.0, -2.0, -2.0, -2.0};
    REQUIRE(diff2 == diff2_ref);
}

TEST_CASE("Mat4 scalar multiplication operators", "[Mat4]") {
    jtx::Mat4 mat{1.0f};
    float scalar = 2.0f;

    jtx::Mat4 prod = mat * scalar;
    jtx::Mat4 prod2 = scalar * mat;

    jtx::Mat4 prod_ref = jtx::Mat4::diagonal(2, 2, 2, 2);
    REQUIRE(prod == prod_ref);
    REQUIRE(prod2 == prod_ref);
}

TEST_CASE("Mat4 scalar division operators", "[Mat4]") {
    jtx::Mat4 mat{1.0f};
    float scalar = 2.0f;

    jtx::Mat4 quot = mat / scalar;
    jtx::Mat4 quot_ref = jtx::Mat4::diagonal(0.5, 0.5, 0.5, 0.5);
    REQUIRE(quot == quot_ref);
}

TEST_CASE("Mat4 isIdentity", "[Mat4]") {
    jtx::Mat4 mat{1.0f};
    REQUIRE(mat.isIdentity());

    jtx::Mat4 mat2{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    REQUIRE(mat2.isIdentity());

    jtx::Mat4 mat3{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f};
    REQUIRE_FALSE(mat3.isIdentity());
}

TEST_CASE("Mat4 mul (Vec4f)", "[Mat4]") {
    jtx::Mat4 mat{1.0f};
    jtx::Vec4f vec{1.0f, 2.0f, 3.0f, 4.0f};

    jtx::Vec4f prod = mat.mul(vec);
    jtx::Vec4f prod_ref{1.0f, 2.0f, 3.0f, 4.0f};
    REQUIRE(prod == prod_ref);
}

TEST_CASE("Mat4 mul and * operator (Mat4)", "[Mat4]") {
    jtx::Mat4 identity{1.0f};
    jtx::Mat4 mat2{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f,
                   16.0f};

    jtx::Mat4 prod = identity.mul(mat2);
    REQUIRE(prod == mat2);

    jtx::Mat4 prod2 = identity * mat2;
    REQUIRE(prod2 == mat2);
}

TEST_CASE("Mat4 determinant", "[Mat4]") {
    jtx::Mat4 mat{2, 1, 0, 3, 4, -1, 2, 0, -3, 2, 1, 5, 1, 0, -2, 3};
    REQUIRE_THAT(mat.determinant(), Catch::Matchers::WithinRel(-85.0f, T_EPS));
}

TEST_CASE("Mat4 transpose", "[Mat4]") {
    jtx::Mat4 mat{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    jtx::Mat4 transposed = mat.transpose();
    jtx::Mat4 ref = jtx::Mat4{1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16};

    REQUIRE(transposed == ref);
    REQUIRE(transposed.transpose() == mat);
}

TEST_CASE("Mat4 inverse", "[Mat4]") {
    jtx::Mat4 mat{1, 1, 1, -1, 1, 1, -1, 1, 1, -1, 1, 1, -1, 1, 1, 1};
    jtx::Mat4 inv = jtx::invert(mat);
    jtx::Mat4 ref{0.25f, 0.25f, 0.25f, -0.25f, 0.25f, 0.25f, -0.25f, 0.25f, 0.25f, -0.25f, 0.25f, 0.25f, -0.25f, 0.25f,
                  0.25f, 0.25f};
    jtx::Mat4 identity{1.0f};
    REQUIRE(inv == ref);
    REQUIRE(inv * mat == identity);
    REQUIRE(mat * inv == identity);
}
