#pragma once
#include <cmath>

namespace NF {

// ---------------------------------------------------------------------------
// Vector2
// ---------------------------------------------------------------------------

/// @brief 2D floating-point vector.
struct Vector2 {
    float X{0.f}, Y{0.f};

    constexpr Vector2() noexcept = default;
    constexpr Vector2(float x, float y) noexcept : X(x), Y(y) {}

    [[nodiscard]] constexpr Vector2 operator+(const Vector2& o) const noexcept { return {X + o.X, Y + o.Y}; }
    [[nodiscard]] constexpr Vector2 operator-(const Vector2& o) const noexcept { return {X - o.X, Y - o.Y}; }
    [[nodiscard]] constexpr Vector2 operator*(float s)          const noexcept { return {X * s,   Y * s  }; }
    [[nodiscard]] constexpr Vector2 operator/(float s)          const noexcept { return {X / s,   Y / s  }; }
    [[nodiscard]] constexpr Vector2 operator-()                 const noexcept { return {-X, -Y}; }

    constexpr Vector2& operator+=(const Vector2& o) noexcept { X += o.X; Y += o.Y; return *this; }
    constexpr Vector2& operator-=(const Vector2& o) noexcept { X -= o.X; Y -= o.Y; return *this; }
    constexpr Vector2& operator*=(float s)          noexcept { X *= s;   Y *= s;   return *this; }
    constexpr Vector2& operator/=(float s)          noexcept { X /= s;   Y /= s;   return *this; }

    [[nodiscard]] constexpr bool operator==(const Vector2&) const noexcept = default;

    [[nodiscard]] float LengthSq()   const noexcept { return X * X + Y * Y; }
    [[nodiscard]] float Length()     const noexcept { return std::sqrt(LengthSq()); }

    /// @brief Return a unit-length copy of this vector.
    [[nodiscard]] Vector2 Normalized() const noexcept {
        const float l = Length();
        return l > 0.f ? *this / l : *this;
    }

    /// @brief Dot product with another vector.
    [[nodiscard]] constexpr float Dot(const Vector2& o) const noexcept { return X * o.X + Y * o.Y; }
};

[[nodiscard]] inline constexpr Vector2 operator*(float s, const Vector2& v) noexcept { return v * s; }

// ---------------------------------------------------------------------------
// Vector3
// ---------------------------------------------------------------------------

/// @brief 3D floating-point vector.
struct Vector3 {
    float X{0.f}, Y{0.f}, Z{0.f};

    constexpr Vector3() noexcept = default;
    constexpr Vector3(float x, float y, float z) noexcept : X(x), Y(y), Z(z) {}

    [[nodiscard]] constexpr Vector3 operator+(const Vector3& o) const noexcept { return {X + o.X, Y + o.Y, Z + o.Z}; }
    [[nodiscard]] constexpr Vector3 operator-(const Vector3& o) const noexcept { return {X - o.X, Y - o.Y, Z - o.Z}; }
    [[nodiscard]] constexpr Vector3 operator*(float s)          const noexcept { return {X * s,   Y * s,   Z * s  }; }
    [[nodiscard]] constexpr Vector3 operator/(float s)          const noexcept { return {X / s,   Y / s,   Z / s  }; }
    [[nodiscard]] constexpr Vector3 operator-()                 const noexcept { return {-X, -Y, -Z}; }

    constexpr Vector3& operator+=(const Vector3& o) noexcept { X += o.X; Y += o.Y; Z += o.Z; return *this; }
    constexpr Vector3& operator-=(const Vector3& o) noexcept { X -= o.X; Y -= o.Y; Z -= o.Z; return *this; }
    constexpr Vector3& operator*=(float s)          noexcept { X *= s;   Y *= s;   Z *= s;   return *this; }
    constexpr Vector3& operator/=(float s)          noexcept { X /= s;   Y /= s;   Z /= s;   return *this; }

    [[nodiscard]] constexpr bool operator==(const Vector3&) const noexcept = default;

    [[nodiscard]] float LengthSq() const noexcept { return X * X + Y * Y + Z * Z; }
    [[nodiscard]] float Length()   const noexcept { return std::sqrt(LengthSq()); }

    /// @brief Return a unit-length copy of this vector.
    [[nodiscard]] Vector3 Normalized() const noexcept {
        const float l = Length();
        return l > 0.f ? *this / l : *this;
    }

    /// @brief Dot product with another vector.
    [[nodiscard]] constexpr float Dot(const Vector3& o) const noexcept {
        return X * o.X + Y * o.Y + Z * o.Z;
    }

    /// @brief Cross product: returns a vector perpendicular to both operands.
    [[nodiscard]] constexpr Vector3 Cross(const Vector3& o) const noexcept {
        return {Y * o.Z - Z * o.Y,
                Z * o.X - X * o.Z,
                X * o.Y - Y * o.X};
    }
};

[[nodiscard]] inline constexpr Vector3 operator*(float s, const Vector3& v) noexcept { return v * s; }

// ---------------------------------------------------------------------------
// Vector4
// ---------------------------------------------------------------------------

/// @brief 4D floating-point vector (also used as homogeneous coordinates).
struct Vector4 {
    float X{0.f}, Y{0.f}, Z{0.f}, W{0.f};

    constexpr Vector4() noexcept = default;
    constexpr Vector4(float x, float y, float z, float w) noexcept : X(x), Y(y), Z(z), W(w) {}
    constexpr Vector4(const Vector3& v, float w) noexcept : X(v.X), Y(v.Y), Z(v.Z), W(w) {}

    [[nodiscard]] constexpr Vector4 operator+(const Vector4& o) const noexcept { return {X + o.X, Y + o.Y, Z + o.Z, W + o.W}; }
    [[nodiscard]] constexpr Vector4 operator-(const Vector4& o) const noexcept { return {X - o.X, Y - o.Y, Z - o.Z, W - o.W}; }
    [[nodiscard]] constexpr Vector4 operator*(float s)          const noexcept { return {X * s,   Y * s,   Z * s,   W * s  }; }
    [[nodiscard]] constexpr Vector4 operator/(float s)          const noexcept { return {X / s,   Y / s,   Z / s,   W / s  }; }
    [[nodiscard]] constexpr Vector4 operator-()                 const noexcept { return {-X, -Y, -Z, -W}; }

    constexpr Vector4& operator+=(const Vector4& o) noexcept { X += o.X; Y += o.Y; Z += o.Z; W += o.W; return *this; }
    constexpr Vector4& operator-=(const Vector4& o) noexcept { X -= o.X; Y -= o.Y; Z -= o.Z; W -= o.W; return *this; }
    constexpr Vector4& operator*=(float s)          noexcept { X *= s;   Y *= s;   Z *= s;   W *= s;   return *this; }
    constexpr Vector4& operator/=(float s)          noexcept { X /= s;   Y /= s;   Z /= s;   W /= s;   return *this; }

    [[nodiscard]] constexpr bool operator==(const Vector4&) const noexcept = default;

    [[nodiscard]] float LengthSq() const noexcept { return X * X + Y * Y + Z * Z + W * W; }
    [[nodiscard]] float Length()   const noexcept { return std::sqrt(LengthSq()); }

    /// @brief Return a unit-length copy of this vector.
    [[nodiscard]] Vector4 Normalized() const noexcept {
        const float l = Length();
        return l > 0.f ? *this / l : *this;
    }

    /// @brief 4D dot product.
    [[nodiscard]] constexpr float Dot(const Vector4& o) const noexcept {
        return X * o.X + Y * o.Y + Z * o.Z + W * o.W;
    }
};

[[nodiscard]] inline constexpr Vector4 operator*(float s, const Vector4& v) noexcept { return v * s; }

} // namespace NF
