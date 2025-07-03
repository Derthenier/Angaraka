module;

#include <Angaraka/Base.hpp>

export module Angaraka.Math.Vector3;

import Angaraka.Math.Vector2;

namespace Angaraka::Math {

    // ==================================================================================
    // Vector3 - 3D Vector
    // ==================================================================================
    export struct Vector3
    {
        F32 x, y, z;

        // Constructors
        inline constexpr Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
        inline constexpr Vector3(F32 x, F32 y, F32 z) : x(x), y(y), z(z) {}
        inline constexpr explicit Vector3(F32 value) : x(value), y(value), z(value) {}
        inline constexpr Vector3(const Vector2& xy, F32 z) : x(xy.x), y(xy.y), z(z) {}

        // Array access
        inline F32& operator[](size_t index) { return (&x)[index]; }
        inline const F32& operator[](size_t index) const { return (&x)[index]; }

        // Arithmetic operators
        inline Vector3 operator+(const Vector3& rhs) const { return { x + rhs.x, y + rhs.y, z + rhs.z }; }
        inline Vector3 operator-(const Vector3& rhs) const { return { x - rhs.x, y - rhs.y, z - rhs.z }; }
        inline Vector3 operator*(F32 scalar) const { return { x * scalar, y * scalar, z * scalar }; }
        inline Vector3 operator/(F32 scalar) const { return { x / scalar, y / scalar, z / scalar }; }
        inline Vector3 operator-() const { return { -x, -y, -z }; }

        // Component-wise multiplication
        inline Vector3 operator*(const Vector3& rhs) const { return { x * rhs.x, y * rhs.y, z * rhs.z }; }
        inline Vector3 operator/(const Vector3& rhs) const { return { x / rhs.x, y / rhs.y, z / rhs.z }; }

        // Assignment operators
        inline Vector3& operator+=(const Vector3& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
        inline Vector3& operator-=(const Vector3& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
        inline Vector3& operator*=(F32 scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
        inline Vector3& operator/=(F32 scalar) { x /= scalar; y /= scalar; z /= scalar; return *this; }
        inline Vector3& operator*=(const Vector3& rhs) { x *= rhs.x; y *= rhs.y; z *= rhs.z; return *this; }
        inline Vector3& operator/=(const Vector3& rhs) { x /= rhs.x; y /= rhs.y; z /= rhs.z; return *this; }

        // Comparison
        bool operator==(const Vector3& rhs) const;
        inline bool operator!=(const Vector3& rhs) const { return !(*this == rhs); }

        // Element-wise minimum with another Vector3
        // This method takes another Vector2 and returns a new Vector3
        // where each component is the minimum of the corresponding components
        // of the two vectors.
        [[nodiscard]] inline Vector3 Min(const Vector3& other) const
        {
            return Vector3(std::min(x, other.x), std::min(y, other.y), std::min(z, other.z));
        }

        // Element-wise maximum with another Vector3
        // This method takes another Vector2 and returns a new Vector3
        // where each component is the maximum of the corresponding components
        // of the two vectors.
        [[nodiscard]] inline Vector3 Max(const Vector3& other) const
        {
            return Vector3(std::max(x, other.x), std::max(y, other.y), std::max(z, other.z));
        }

        // Static versions
        inline static Vector3 Min(const Vector3& a, const Vector3& b)
        {
            return a.Min(b);
        }

        inline static Vector3 Max(const Vector3& a, const Vector3& b)
        {
            return a.Max(b);
        }

        // Vector operations
        F32 Length() const;
        inline F32 LengthSquared() const { return x * x + y * y + z * z; }
        Vector3 Normalized() const;
        void Normalize();
        inline F32 Dot(const Vector3& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z; }
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
        inline Vector2 xy() const { return { x, y }; }
        inline Vector2 xz() const { return { x, z }; }
        inline Vector2 yz() const { return { y, z }; }

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
    export Vector3 operator*(F32 scalar, const Vector3& vec);
} // namespace Angaraka::Math