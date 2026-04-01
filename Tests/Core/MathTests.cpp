/// @file MathTests.cpp — Unit tests for Core math types (Vector, Quaternion, Matrix).
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "Core/Math/Vector.h"
#include "Core/Math/Quaternion.h"
#include "Core/Math/Matrix.h"

using namespace NF;
using Catch::Matchers::WithinAbs;

static constexpr float kEps = 1e-5f;

// ---------------------------------------------------------------------------
// Vector2
// ---------------------------------------------------------------------------

TEST_CASE("Vector2 arithmetic", "[math][vector2]") {
    const Vector2 a{3.f, 4.f};
    const Vector2 b{1.f, 2.f};

    REQUIRE(a + b == Vector2{4.f, 6.f});
    REQUIRE(a - b == Vector2{2.f, 2.f});
    REQUIRE(a * 2.f == Vector2{6.f, 8.f});
    REQUIRE(a / 2.f == Vector2{1.5f, 2.f});
    REQUIRE(-a == Vector2{-3.f, -4.f});
}

TEST_CASE("Vector2 length", "[math][vector2]") {
    const Vector2 v{3.f, 4.f};
    REQUIRE_THAT(v.Length(), WithinAbs(5.f, kEps));
    REQUIRE_THAT(v.Normalized().Length(), WithinAbs(1.f, kEps));
}

TEST_CASE("Vector2 dot product", "[math][vector2]") {
    const Vector2 a{1.f, 0.f};
    const Vector2 b{0.f, 1.f};
    REQUIRE_THAT(a.Dot(b), WithinAbs(0.f, kEps));
    REQUIRE_THAT(a.Dot(a), WithinAbs(1.f, kEps));
}

// ---------------------------------------------------------------------------
// Vector3
// ---------------------------------------------------------------------------

TEST_CASE("Vector3 arithmetic", "[math][vector3]") {
    const Vector3 a{1.f, 2.f, 3.f};
    const Vector3 b{4.f, 5.f, 6.f};

    REQUIRE(a + b == Vector3{5.f, 7.f, 9.f});
    REQUIRE(b - a == Vector3{3.f, 3.f, 3.f});
    REQUIRE(a * 2.f == Vector3{2.f, 4.f, 6.f});
}

TEST_CASE("Vector3 dot and cross product", "[math][vector3]") {
    const Vector3 x{1.f, 0.f, 0.f};
    const Vector3 y{0.f, 1.f, 0.f};
    const Vector3 z{0.f, 0.f, 1.f};

    REQUIRE_THAT(x.Dot(y), WithinAbs(0.f, kEps));
    REQUIRE_THAT(x.Dot(x), WithinAbs(1.f, kEps));

    const Vector3 cross = x.Cross(y);
    REQUIRE_THAT(cross.X, WithinAbs(z.X, kEps));
    REQUIRE_THAT(cross.Y, WithinAbs(z.Y, kEps));
    REQUIRE_THAT(cross.Z, WithinAbs(z.Z, kEps));
}

TEST_CASE("Vector3 normalize", "[math][vector3]") {
    const Vector3 v{1.f, 2.f, 3.f};
    REQUIRE_THAT(v.Normalized().Length(), WithinAbs(1.f, kEps));
}

// ---------------------------------------------------------------------------
// Quaternion
// ---------------------------------------------------------------------------

TEST_CASE("Quaternion identity", "[math][quaternion]") {
    const Quaternion q = Quaternion::Identity();
    REQUIRE_THAT(q.W, WithinAbs(1.f, kEps));
    REQUIRE_THAT(q.X, WithinAbs(0.f, kEps));
    REQUIRE_THAT(q.Y, WithinAbs(0.f, kEps));
    REQUIRE_THAT(q.Z, WithinAbs(0.f, kEps));
    REQUIRE_THAT(q.Length(), WithinAbs(1.f, kEps));
}

TEST_CASE("Quaternion normalize", "[math][quaternion]") {
    const Quaternion q{2.f, 0.f, 0.f, 0.f}; // non-unit
    const Quaternion n = q.Normalize();
    REQUIRE_THAT(n.Length(), WithinAbs(1.f, kEps));
}

// ---------------------------------------------------------------------------
// Matrix4x4
// ---------------------------------------------------------------------------

TEST_CASE("Matrix4x4 identity multiply", "[math][matrix]") {
    const Matrix4x4 I = Matrix4x4::Identity();
    const Matrix4x4 M = I.Multiply(I);

    for (int col = 0; col < 4; ++col)
        for (int row = 0; row < 4; ++row)
            REQUIRE_THAT(M.M[col][row], WithinAbs(I.M[col][row], kEps));
}

TEST_CASE("Matrix4x4 transpose", "[math][matrix]") {
    Matrix4x4 m = Matrix4x4::Identity();
    m.M[0][1] = 5.f;  // row=1, col=0
    const Matrix4x4 t = m.Transpose();
    REQUIRE_THAT(t.M[1][0], WithinAbs(5.f, kEps));
}
