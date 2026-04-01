#pragma once
#include "Core/Math/Vector.h"
#include <cmath>

namespace NF {

/// @brief Unit quaternion representing a 3D orientation.
///
/// Convention: W is the scalar part and X/Y/Z are the vector part.
/// The default-constructed quaternion is the identity (no rotation).
struct Quaternion {
    float X{0.f}, Y{0.f}, Z{0.f}, W{1.f};

    constexpr Quaternion() noexcept = default;
    constexpr Quaternion(float x, float y, float z, float w) noexcept
        : X(x), Y(y), Z(z), W(w) {}

    /// @brief Return the identity quaternion (W=1, X=Y=Z=0).
    [[nodiscard]] static constexpr Quaternion Identity() noexcept { return {}; }

    // -----------------------------------------------------------------------
    // Arithmetic operators
    // -----------------------------------------------------------------------

    [[nodiscard]] constexpr Quaternion operator+(const Quaternion& o) const noexcept {
        return {X + o.X, Y + o.Y, Z + o.Z, W + o.W};
    }
    [[nodiscard]] constexpr Quaternion operator*(float s) const noexcept {
        return {X * s, Y * s, Z * s, W * s};
    }

    /// @brief Hamilton product — combine two rotations (right-to-left order).
    [[nodiscard]] constexpr Quaternion operator*(const Quaternion& o) const noexcept {
        return {
            W * o.X + X * o.W + Y * o.Z - Z * o.Y,
            W * o.Y - X * o.Z + Y * o.W + Z * o.X,
            W * o.Z + X * o.Y - Y * o.X + Z * o.W,
            W * o.W - X * o.X - Y * o.Y - Z * o.Z
        };
    }

    [[nodiscard]] constexpr bool operator==(const Quaternion&) const noexcept = default;

    // -----------------------------------------------------------------------
    // Core operations
    // -----------------------------------------------------------------------

    /// @brief Return the conjugate (inverse for unit quaternions).
    [[nodiscard]] constexpr Quaternion Conjugate() const noexcept { return {-X, -Y, -Z, W}; }

    [[nodiscard]] float LengthSq() const noexcept { return X * X + Y * Y + Z * Z + W * W; }
    [[nodiscard]] float Length()   const noexcept { return std::sqrt(LengthSq()); }

    /// @brief Return a unit-length copy of this quaternion.
    [[nodiscard]] Quaternion Normalize() const noexcept {
        const float l = Length();
        return l > 0.f ? *this * (1.f / l) : *this;
    }

    /// @brief Rotate a 3D vector by this quaternion (assumes unit length).
    [[nodiscard]] constexpr Vector3 Rotate(const Vector3& v) const noexcept {
        // Efficient sandwich product: q * (0,v) * q^-1
        const Vector3 qv{X, Y, Z};
        const Vector3 uv  = qv.Cross(v);
        const Vector3 uuv = qv.Cross(uv);
        return v + (uv * W + uuv) * 2.f;
    }

    // -----------------------------------------------------------------------
    // Interpolation
    // -----------------------------------------------------------------------

    /// @brief Spherical linear interpolation between two unit quaternions.
    /// @param a  Start orientation (t=0).
    /// @param b  End orientation (t=1).
    /// @param t  Blend factor in [0, 1].
    /// @return   Interpolated unit quaternion.
    [[nodiscard]] static Quaternion Slerp(Quaternion a, Quaternion b, float t) noexcept {
        float dot = a.X * b.X + a.Y * b.Y + a.Z * b.Z + a.W * b.W;

        // Ensure we travel along the shortest arc
        if (dot < 0.f) {
            b   = b * -1.f;
            dot = -dot;
        }

        // Fall back to normalised lerp when the quaternions are nearly identical
        constexpr float kLinearThreshold = 0.9995f;
        if (dot > kLinearThreshold) {
            Quaternion r{
                a.X + t * (b.X - a.X),
                a.Y + t * (b.Y - a.Y),
                a.Z + t * (b.Z - a.Z),
                a.W + t * (b.W - a.W)
            };
            return r.Normalize();
        }

        const float theta0 = std::acos(dot);
        const float theta  = theta0 * t;
        const float sinT0  = std::sin(theta0);
        const float s0     = std::cos(theta) - dot * std::sin(theta) / sinT0;
        const float s1     = std::sin(theta) / sinT0;

        return {
            a.X * s0 + b.X * s1,
            a.Y * s0 + b.Y * s1,
            a.Z * s0 + b.Z * s1,
            a.W * s0 + b.W * s1
        };
    }
};

} // namespace NF
