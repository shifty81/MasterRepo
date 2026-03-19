#pragma once
#include <cmath>
#include <algorithm>

namespace Engine::Math {

constexpr float PI      = 3.14159265358979323846f;
constexpr float DEG2RAD = PI / 180.0f;
constexpr float RAD2DEG = 180.0f / PI;

// ---------------------------------------------------------------------------
// Vec2
// ---------------------------------------------------------------------------
struct Vec2 {
    float x = 0.0f, y = 0.0f;

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s)       const { return {x * s, y * s}; }
    Vec2 operator/(float s)       const { return {x / s, y / s}; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s)       { x *= s;   y *= s;   return *this; }
    Vec2& operator/=(float s)       { x /= s;   y /= s;   return *this; }
    bool  operator==(const Vec2& o) const { return x == o.x && y == o.y; }
    bool  operator!=(const Vec2& o) const { return !(*this == o); }

    float Dot(const Vec2& o)  const { return x * o.x + y * o.y; }
    float LengthSq()          const { return x * x + y * y; }
    float Length()            const { return std::sqrt(LengthSq()); }
    Vec2  Normalized()        const { float l = Length(); return l > 0.0f ? *this / l : Vec2{}; }
};

inline Vec2 operator*(float s, const Vec2& v) { return v * s; }

// ---------------------------------------------------------------------------
// Vec3
// ---------------------------------------------------------------------------
struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s)       const { return {x * s, y * s, z * s}; }
    Vec3 operator/(float s)       const { return {x / s, y / s, z / s}; }
    Vec3 operator-()              const { return {-x, -y, -z}; }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s)       { x *= s;   y *= s;   z *= s;   return *this; }
    Vec3& operator/=(float s)       { x /= s;   y /= s;   z /= s;   return *this; }
    bool  operator==(const Vec3& o) const { return x == o.x && y == o.y && z == o.z; }
    bool  operator!=(const Vec3& o) const { return !(*this == o); }

    float Dot(const Vec3& o)  const { return x * o.x + y * o.y + z * o.z; }
    float LengthSq()          const { return x * x + y * y + z * z; }
    float Length()            const { return std::sqrt(LengthSq()); }
    Vec3  Normalized()        const { float l = Length(); return l > 0.0f ? *this / l : Vec3{}; }
    Vec3  Cross(const Vec3& o) const {
        return { y * o.z - z * o.y,
                 z * o.x - x * o.z,
                 x * o.y - y * o.x };
    }
};

inline Vec3 operator*(float s, const Vec3& v) { return v * s; }

// ---------------------------------------------------------------------------
// Vec4
// ---------------------------------------------------------------------------
struct Vec4 {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;

    Vec4 operator+(const Vec4& o) const { return {x + o.x, y + o.y, z + o.z, w + o.w}; }
    Vec4 operator-(const Vec4& o) const { return {x - o.x, y - o.y, z - o.z, w - o.w}; }
    Vec4 operator*(float s)       const { return {x * s, y * s, z * s, w * s}; }
    Vec4 operator/(float s)       const { return {x / s, y / s, z / s, w / s}; }
    Vec4& operator+=(const Vec4& o) { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
    Vec4& operator-=(const Vec4& o) { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
    Vec4& operator*=(float s)       { x *= s;   y *= s;   z *= s;   w *= s;   return *this; }
    Vec4& operator/=(float s)       { x /= s;   y /= s;   z /= s;   w /= s;   return *this; }
    bool  operator==(const Vec4& o) const { return x == o.x && y == o.y && z == o.z && w == o.w; }
    bool  operator!=(const Vec4& o) const { return !(*this == o); }

    float Dot(const Vec4& o)  const { return x * o.x + y * o.y + z * o.z + w * o.w; }
    float LengthSq()          const { return x * x + y * y + z * z + w * w; }
    float Length()            const { return std::sqrt(LengthSq()); }
    Vec4  Normalized()        const { float l = Length(); return l > 0.0f ? *this / l : Vec4{}; }

    Vec3 XYZ() const { return {x, y, z}; }
};

inline Vec4 operator*(float s, const Vec4& v) { return v * s; }

// ---------------------------------------------------------------------------
// Mat4 — column-major 4x4 float matrix
// m[col][row]
// ---------------------------------------------------------------------------
struct Mat4 {
    float m[4][4] = {};

    static Mat4 Identity() {
        Mat4 r{};
        r.m[0][0] = r.m[1][1] = r.m[2][2] = r.m[3][3] = 1.0f;
        return r;
    }

    static Mat4 Translation(const Vec3& t) {
        Mat4 r = Identity();
        r.m[3][0] = t.x;
        r.m[3][1] = t.y;
        r.m[3][2] = t.z;
        return r;
    }

    static Mat4 Scale(const Vec3& s) {
        Mat4 r = Identity();
        r.m[0][0] = s.x;
        r.m[1][1] = s.y;
        r.m[2][2] = s.z;
        return r;
    }

    // Rodrigues' rotation formula
    static Mat4 Rotation(float angleDeg, Vec3 axis) {
        axis = axis.Normalized();
        float cosAngle    = std::cos(angleDeg * DEG2RAD);
        float sinAngle    = std::sin(angleDeg * DEG2RAD);
        float oneMinusCos = 1.0f - cosAngle;
        float axisX = axis.x, axisY = axis.y, axisZ = axis.z;

        Mat4 r = Identity();
        r.m[0][0] = oneMinusCos*axisX*axisX + cosAngle;
        r.m[0][1] = oneMinusCos*axisX*axisY + sinAngle*axisZ;
        r.m[0][2] = oneMinusCos*axisX*axisZ - sinAngle*axisY;

        r.m[1][0] = oneMinusCos*axisX*axisY - sinAngle*axisZ;
        r.m[1][1] = oneMinusCos*axisY*axisY + cosAngle;
        r.m[1][2] = oneMinusCos*axisY*axisZ + sinAngle*axisX;

        r.m[2][0] = oneMinusCos*axisX*axisZ + sinAngle*axisY;
        r.m[2][1] = oneMinusCos*axisY*axisZ - sinAngle*axisX;
        r.m[2][2] = oneMinusCos*axisZ*axisZ + cosAngle;
        return r;
    }

    // Standard OpenGL-style perspective (maps to NDC [-1,1])
    static Mat4 Perspective(float fovDeg, float aspect, float nearZ, float farZ) {
        float f = 1.0f / std::tan(fovDeg * DEG2RAD * 0.5f);
        float rDepth = 1.0f / (nearZ - farZ);

        Mat4 r{};
        r.m[0][0] = f / aspect;
        r.m[1][1] = f;
        r.m[2][2] = (farZ + nearZ) * rDepth;
        r.m[2][3] = -1.0f;
        r.m[3][2] = 2.0f * farZ * nearZ * rDepth;
        return r;
    }

    Mat4 Multiply(const Mat4& o) const {
        Mat4 r{};
        for (int col = 0; col < 4; ++col)
            for (int row = 0; row < 4; ++row)
                for (int k = 0; k < 4; ++k)
                    r.m[col][row] += m[k][row] * o.m[col][k];
        return r;
    }

    Vec4 Transform(const Vec4& v) const {
        return {
            m[0][0]*v.x + m[1][0]*v.y + m[2][0]*v.z + m[3][0]*v.w,
            m[0][1]*v.x + m[1][1]*v.y + m[2][1]*v.z + m[3][1]*v.w,
            m[0][2]*v.x + m[1][2]*v.y + m[2][2]*v.z + m[3][2]*v.w,
            m[0][3]*v.x + m[1][3]*v.y + m[2][3]*v.z + m[3][3]*v.w
        };
    }

    Mat4 operator*(const Mat4& o) const { return Multiply(o); }
    Vec4 operator*(const Vec4& v) const { return Transform(v); }
};

// ---------------------------------------------------------------------------
// Quaternion
// ---------------------------------------------------------------------------
struct Quaternion {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;

    static Quaternion Identity() { return {0.0f, 0.0f, 0.0f, 1.0f}; }

    static Quaternion FromAxisAngle(const Vec3& axis, float angleDeg) {
        float half = angleDeg * DEG2RAD * 0.5f;
        float s    = std::sin(half);
        Vec3  a    = axis.Normalized();
        return { a.x * s, a.y * s, a.z * s, std::cos(half) };
    }

    Quaternion Multiply(const Quaternion& o) const {
        return {
             w*o.x + x*o.w + y*o.z - z*o.y,
             w*o.y - x*o.z + y*o.w + z*o.x,
             w*o.z + x*o.y - y*o.x + z*o.w,
             w*o.w - x*o.x - y*o.y - z*o.z
        };
    }

    Quaternion operator*(const Quaternion& o) const { return Multiply(o); }

    float LengthSq() const { return x*x + y*y + z*z + w*w; }
    float Length()   const { return std::sqrt(LengthSq()); }
    Quaternion Normalized() const {
        float l = Length();
        return l > 0.0f ? Quaternion{x/l, y/l, z/l, w/l} : Identity();
    }

    Quaternion Conjugate() const { return {-x, -y, -z, w}; }

    Mat4 ToMatrix() const {
        Quaternion q = Normalized();
        float xx = q.x*q.x, yy = q.y*q.y, zz = q.z*q.z;
        float xy = q.x*q.y, xz = q.x*q.z, yz = q.y*q.z;
        float wx = q.w*q.x, wy = q.w*q.y, wz = q.w*q.z;

        Mat4 r = Mat4::Identity();
        r.m[0][0] = 1.0f - 2.0f*(yy + zz);
        r.m[0][1] =        2.0f*(xy + wz);
        r.m[0][2] =        2.0f*(xz - wy);

        r.m[1][0] =        2.0f*(xy - wz);
        r.m[1][1] = 1.0f - 2.0f*(xx + zz);
        r.m[1][2] =        2.0f*(yz + wx);

        r.m[2][0] =        2.0f*(xz + wy);
        r.m[2][1] =        2.0f*(yz - wx);
        r.m[2][2] = 1.0f - 2.0f*(xx + yy);
        return r;
    }

    // Spherical linear interpolation
    static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t) {
        Quaternion qa = a.Normalized();
        Quaternion qb = b.Normalized();

        float dot = qa.x*qb.x + qa.y*qb.y + qa.z*qb.z + qa.w*qb.w;

        // Ensure shortest path
        if (dot < 0.0f) {
            qb  = {-qb.x, -qb.y, -qb.z, -qb.w};
            dot = -dot;
        }

        // Fall back to linear interpolation for nearly identical quaternions
        if (dot > 0.9995f) {
            Quaternion r{
                qa.x + t*(qb.x - qa.x),
                qa.y + t*(qb.y - qa.y),
                qa.z + t*(qb.z - qa.z),
                qa.w + t*(qb.w - qa.w)
            };
            return r.Normalized();
        }

        float theta0 = std::acos(dot);
        float theta  = theta0 * t;
        float s0     = std::cos(theta) - dot * std::sin(theta) / std::sin(theta0);
        float s1     = std::sin(theta) / std::sin(theta0);

        return {
            s0*qa.x + s1*qb.x,
            s0*qa.y + s1*qb.y,
            s0*qa.z + s1*qb.z,
            s0*qa.w + s1*qb.w
        };
    }
};

// ---------------------------------------------------------------------------
// Transform
// ---------------------------------------------------------------------------
struct Transform {
    Vec3       position = {0.0f, 0.0f, 0.0f};
    Quaternion rotation = Quaternion::Identity();
    Vec3       scale    = {1.0f, 1.0f, 1.0f};

    Mat4 ToMatrix() const {
        return Mat4::Translation(position)
             * rotation.ToMatrix()
             * Mat4::Scale(scale);
    }
};

} // namespace Engine::Math
