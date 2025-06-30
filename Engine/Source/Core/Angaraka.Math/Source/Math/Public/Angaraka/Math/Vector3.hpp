// ==================================================================================
// AngarakaMath/Public/Math/Vector3.hpp - 3D Vector
// ==================================================================================

#pragma once

#include "Vector2.hpp"

namespace Angaraka::Math
{
    // ==================================================================================
    // Vector3 - 3D Vector
    // ==================================================================================

    struct Vector3
    {
        F32 x, y, z;

        // Constructors
        constexpr Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
        constexpr Vector3(F32 x, F32 y, F32 z) : x(x), y(y), z(z) {}
        constexpr explicit Vector3(F32 value) : x(value), y(value), z(value) {}
        constexpr Vector3(const Vector2& xy, F32 z) : x(xy.x), y(xy.y), z(z) {}

        // Array access
        float& operator[](size_t index) { return (&x)[index]; }
        const float& operator[](size_t index) const { return (&x)[index]; }

        // Arithmetic operators
        Vector3 operator+(const Vector3& rhs) const { return { x + rhs.x, y + rhs.y, z + rhs.z }; }
        Vector3 operator-(const Vector3& rhs) const { return { x - rhs.x, y - rhs.y, z - rhs.z }; }
        Vector3 operator*(F32 scalar) const { return { x * scalar, y * scalar, z * scalar }; }
        Vector3 operator/(F32 scalar) const { return { x / scalar, y / scalar, z / scalar }; }
        Vector3 operator-() const { return { -x, -y, -z }; }

        // Component-wise multiplication
        Vector3 operator*(const Vector3& rhs) const { return { x * rhs.x, y * rhs.y, z * rhs.z }; }
        Vector3 operator/(const Vector3& rhs) const { return { x / rhs.x, y / rhs.y, z / rhs.z }; }

        // Assignment operators
        Vector3& operator+=(const Vector3& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
        Vector3& operator-=(const Vector3& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
        Vector3& operator*=(F32 scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
        Vector3& operator/=(F32 scalar) { x /= scalar; y /= scalar; z /= scalar; return *this; }
        Vector3& operator*=(const Vector3& rhs) { x *= rhs.x; y *= rhs.y; z *= rhs.z; return *this; }
        Vector3& operator/=(const Vector3& rhs) { x /= rhs.x; y /= rhs.y; z /= rhs.z; return *this; }

        // Comparison
        bool operator==(const Vector3& rhs) const;
        bool operator!=(const Vector3& rhs) const { return !(*this == rhs); }

        // Element-wise minimum with another Vector3
        // This method takes another Vector2 and returns a new Vector3
        // where each component is the minimum of the corresponding components
        // of the two vectors.
        [[nodiscard]] Vector3 Min(const Vector3& other) const
        {
            return Vector3(std::min(x, other.x), std::min(y, other.y), std::min(z, other.z));
        }

        // Element-wise maximum with another Vector3
        // This method takes another Vector2 and returns a new Vector3
        // where each component is the maximum of the corresponding components
        // of the two vectors.
        [[nodiscard]] Vector3 Max(const Vector3& other) const
        {
            return Vector3(std::max(x, other.x), std::max(y, other.y), std::max(z, other.z));
        }

        // Static versions
        static Vector3 Min(const Vector3& a, const Vector3& b)
        {
            return a.Min(b);
        }

        static Vector3 Max(const Vector3& a, const Vector3& b)
        {
            return a.Max(b);
        }

        // Vector operations
        F32 Length() const;
        F32 LengthSquared() const { return x * x + y * y + z * z; }
        Vector3 Normalized() const;
        void Normalize();
        F32 Dot(const Vector3& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z; }
        Vector3 Cross(const Vector3& rhs) const;

        // Utility functions
        F32 DistanceTo(const Vector3& other) const;
        F32 DistanceSquaredTo(const Vector3& other) const;
        Vector3 Lerp(const Vector3& target, F32 t) const;
        Vector3 Slerp(const Vector3& target, F32 t) const; // Spherical linear interpolation
        Vector3 Reflect(const Vector3& normal) const;
        Vector3 Project(const Vector3& onto) const;
        Vector3 Reject(const Vector3& from) const;

        // Swizzling
        Vector2 xy() const { return { x, y }; }
        Vector2 xz() const { return { x, z }; }
        Vector2 yz() const { return { y, z }; }

        // Static constants
        static const Vector3 Zero;
        static const Vector3 One;
        static const Vector3 UnitX;
        static const Vector3 UnitY;
        static const Vector3 UnitZ;
        static const Vector3 Forward;   // -Z (OpenGL style)
        static const Vector3 Back;      // +Z
        static const Vector3 Up;        // +Y
        static const Vector3 Down;      // -Y
        static const Vector3 Left;      // -X
        static const Vector3 Right;     // +X
    };

    // Non-member operators
    Vector3 operator*(F32 scalar, const Vector3& vec);
}