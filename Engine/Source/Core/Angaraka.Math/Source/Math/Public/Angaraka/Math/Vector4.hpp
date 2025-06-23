// ==================================================================================
// AngarakaMath/Public/Math/Vector4.hpp - 4D Vector
// ==================================================================================

#pragma once

#include "Vector2.hpp"
#include "Vector3.hpp"

namespace Angaraka::Math
{
    // ==================================================================================
    // Vector4 - 4D Vector (XYZW)
    // ==================================================================================

    struct Vector4
    {
        float x, y, z, w;

        // Constructors
        constexpr Vector4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
        constexpr Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
        constexpr explicit Vector4(float value) : x(value), y(value), z(value), w(value) {}
        constexpr Vector4(const Vector3& xyz, float w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}
        constexpr Vector4(const Vector2& xy, float z, float w) : x(xy.x), y(xy.y), z(z), w(w) {}
        constexpr Vector4(const Vector2& xy, const Vector2& zw) : x(xy.x), y(xy.y), z(zw.x), w(zw.y) {}

        // Array access
        float& operator[](size_t index) { return (&x)[index]; }
        const float& operator[](size_t index) const { return (&x)[index]; }

        // Arithmetic operators
        Vector4 operator+(const Vector4& rhs) const { return { x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w }; }
        Vector4 operator-(const Vector4& rhs) const { return { x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w }; }
        Vector4 operator*(float scalar) const { return { x * scalar, y * scalar, z * scalar, w * scalar }; }
        Vector4 operator/(float scalar) const { return { x / scalar, y / scalar, z / scalar, w / scalar }; }
        Vector4 operator-() const { return { -x, -y, -z, -w }; }

        // Component-wise multiplication
        Vector4 operator*(const Vector4& rhs) const { return { x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w }; }
        Vector4 operator/(const Vector4& rhs) const { return { x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w }; }

        // Assignment operators
        Vector4& operator+=(const Vector4& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }
        Vector4& operator-=(const Vector4& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return *this; }
        Vector4& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }
        Vector4& operator/=(float scalar) { x /= scalar; y /= scalar; z /= scalar; w /= scalar; return *this; }
        Vector4& operator*=(const Vector4& rhs) { x *= rhs.x; y *= rhs.y; z *= rhs.z; w *= rhs.w; return *this; }
        Vector4& operator/=(const Vector4& rhs) { x /= rhs.x; y /= rhs.y; z /= rhs.z; w /= rhs.w; return *this; }

        // Comparison
        bool operator==(const Vector4& rhs) const;
        bool operator!=(const Vector4& rhs) const { return !(*this == rhs); }

        // Vector operations
        float Length() const;
        float LengthSquared() const { return x * x + y * y + z * z + w * w; }
        Vector4 Normalized() const;
        void Normalize();
        float Dot(const Vector4& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w; }

        // Utility functions
        Vector4 Lerp(const Vector4& target, float t) const;

        // Swizzling
        Vector2 xy() const { return { x, y }; }
        Vector2 zw() const { return { z, w }; }
        Vector3 xyz() const { return { x, y, z }; }

        // Static constants
        static const Vector4 Zero;
        static const Vector4 One;
        static const Vector4 UnitX;
        static const Vector4 UnitY;
        static const Vector4 UnitZ;
        static const Vector4 UnitW;
    };

    // Non-member operators
    Vector4 operator*(float scalar, const Vector4& vec);
}