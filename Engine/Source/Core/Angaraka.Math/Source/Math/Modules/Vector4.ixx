module;

#include <Angaraka/Base.hpp>

export module Angaraka.Math.Vector4;

import Angaraka.Math.Vector2;
import Angaraka.Math.Vector3;

namespace Angaraka::Math {
    // ==================================================================================
    // Vector4 - 4D Vector (XYZW)
    // ==================================================================================
    export struct Vector4
    {
        F32 x, y, z, w;

        // Constructors
        inline constexpr Vector4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
        inline constexpr Vector4(F32 x, F32 y, F32 z, F32 w) : x(x), y(y), z(z), w(w) {}
        inline constexpr explicit Vector4(F32 value) : x(value), y(value), z(value), w(value) {}
        inline constexpr Vector4(const Vector3& xyz, F32 w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}
        inline constexpr Vector4(const Vector2& xy, F32 z, F32 w) : x(xy.x), y(xy.y), z(z), w(w) {}
        inline constexpr Vector4(const Vector2& xy, const Vector2& zw) : x(xy.x), y(xy.y), z(zw.x), w(zw.y) {}

        // Array access
        inline F32& operator[](size_t index) { return (&x)[index]; }
        inline const F32& operator[](size_t index) const { return (&x)[index]; }

        // Arithmetic operators
        inline Vector4 operator+(const Vector4& rhs) const { return { x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w }; }
        inline Vector4 operator-(const Vector4& rhs) const { return { x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w }; }
        inline Vector4 operator*(F32 scalar) const { return { x * scalar, y * scalar, z * scalar, w * scalar }; }
        inline Vector4 operator/(F32 scalar) const { return { x / scalar, y / scalar, z / scalar, w / scalar }; }
        inline Vector4 operator-() const { return { -x, -y, -z, -w }; }

        // Component-wise multiplication
        inline Vector4 operator*(const Vector4& rhs) const { return { x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w }; }
        inline Vector4 operator/(const Vector4& rhs) const { return { x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w }; }

        // Assignment operators
        inline Vector4& operator+=(const Vector4& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }
        inline Vector4& operator-=(const Vector4& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return *this; }
        inline Vector4& operator*=(F32 scalar) { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }
        inline Vector4& operator/=(F32 scalar) { x /= scalar; y /= scalar; z /= scalar; w /= scalar; return *this; }
        inline Vector4& operator*=(const Vector4& rhs) { x *= rhs.x; y *= rhs.y; z *= rhs.z; w *= rhs.w; return *this; }
        inline Vector4& operator/=(const Vector4& rhs) { x /= rhs.x; y /= rhs.y; z /= rhs.z; w /= rhs.w; return *this; }

        // Comparison
        bool operator==(const Vector4& rhs) const;
        inline bool operator!=(const Vector4& rhs) const { return !(*this == rhs); }

        // Element-wise minimum with another Vector4
        // This method takes another Vector2 and returns a new Vector4
        // where each component is the minimum of the corresponding components
        // of the two vectors.
        [[nodiscard]] inline Vector4 Min(const Vector4& other) const
        {
            return Vector4(std::min(x, other.x), std::min(y, other.y), std::min(z, other.z), std::min(w, other.w));
        }

        // Element-wise maximum with another Vector4
        // This method takes another Vector2 and returns a new Vector4
        // where each component is the maximum of the corresponding components
        // of the two vectors.
        [[nodiscard]] inline Vector4 Max(const Vector4& other) const
        {
            return Vector4(std::max(x, other.x), std::max(y, other.y), std::max(z, other.z), std::max(w, other.w));
        }

        // Static versions
        inline static Vector4 Min(const Vector4& a, const Vector4& b)
        {
            return a.Min(b);
        }

        inline static Vector4 Max(const Vector4& a, const Vector4& b)
        {
            return a.Max(b);
        }

        // Vector operations
        F32 Length() const;
        inline F32 LengthSquared() const { return x * x + y * y + z * z + w * w; }
        Vector4 Normalized() const;
        void Normalize();
        inline F32 Dot(const Vector4& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w; }

        // Utility functions
        Vector4 Lerp(const Vector4& target, F32 t) const;

        // Swizzling
        inline Vector2 xy() const { return { x, y }; }
        inline Vector2 zw() const { return { z, w }; }
        inline Vector3 xyz() const { return { x, y, z }; }

        // Static constants
        static const Vector4 Zero;
        static const Vector4 One;
        static const Vector4 UnitX;
        static const Vector4 UnitY;
        static const Vector4 UnitZ;
        static const Vector4 UnitW;
    };

    // Non-member operators
    export Vector4 operator*(F32 scalar, const Vector4& vec);
} // namespace Angaraka::Math