module;

#include <Angaraka/Base.hpp>

export module Angaraka.Math.Vector2;

namespace Angaraka::Math {

    // ==================================================================================
    // Vector2 - 2D Vector
    // ==================================================================================
    export struct Vector2
    {
        F32 x, y;

        // Constructors
        inline constexpr Vector2() : x(0.0f), y(0.0f) {}
        inline constexpr Vector2(F32 x, F32 y) : x(x), y(y) {}
        inline constexpr explicit Vector2(F32 value) : x(value), y(value) {}

        // Array access
        F32& operator[](size_t index) { return (&x)[index]; }
        const F32& operator[](size_t index) const { return (&x)[index]; }

        // Arithmetic operators
        inline Vector2 operator+(const Vector2& rhs) const { return { x + rhs.x, y + rhs.y }; }
        inline Vector2 operator-(const Vector2& rhs) const { return { x - rhs.x, y - rhs.y }; }
        inline Vector2 operator*(F32 scalar) const { return { x * scalar, y * scalar }; }
        inline Vector2 operator/(F32 scalar) const { return { x / scalar, y / scalar }; }
        inline Vector2 operator-() const { return { -x, -y }; }

        // Component-wise multiplication
        inline Vector2 operator*(const Vector2& rhs) const { return { x * rhs.x, y * rhs.y }; }
        inline Vector2 operator/(const Vector2& rhs) const { return { x / rhs.x, y / rhs.y }; }

        // Assignment operators
        inline Vector2& operator+=(const Vector2& rhs) { x += rhs.x; y += rhs.y; return *this; }
        inline Vector2& operator-=(const Vector2& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
        inline Vector2& operator*=(F32 scalar) { x *= scalar; y *= scalar; return *this; }
        inline Vector2& operator/=(F32 scalar) { x /= scalar; y /= scalar; return *this; }
        inline Vector2& operator*=(const Vector2& rhs) { x *= rhs.x; y *= rhs.y; return *this; }
        inline Vector2& operator/=(const Vector2& rhs) { x /= rhs.x; y /= rhs.y; return *this; }

        // Comparison
        bool operator==(const Vector2& rhs) const;
        inline bool operator!=(const Vector2& rhs) const { return !(*this == rhs); }

        // Element-wise minimum with another Vector2
        // This method takes another Vector2 and returns a new Vector2
        // where each component is the minimum of the corresponding components
        // of the two vectors.
        // For example, if v1 = (1, 5) and v2 = (3, 2), v1.Min(v2) will be (1, 2).
        [[nodiscard]] inline Vector2 Min(const Vector2& other) const
        {
            return Vector2(std::min(x, other.x), std::min(y, other.y));
        }

        // Element-wise maximum with another Vector2
        // This method takes another Vector2 and returns a new Vector2
        // where each component is the maximum of the corresponding components
        // of the two vectors.
        // For example, if v1 = (1, 5) and v2 = (3, 2), v1.Max(v2) will be (3, 5).
        [[nodiscard]] inline Vector2 Max(const Vector2& other) const
        {
            return Vector2(std::max(x, other.x), std::max(y, other.y));
        }

        // Static versions
        inline static Vector2 Min(const Vector2& a, const Vector2& b)
        {
            return a.Min(b);
        }

        inline static Vector2 Max(const Vector2& a, const Vector2& b)
        {
            return a.Max(b);
        }

        // Vector operations
        F32 Length() const;
        inline F32 LengthSquared() const { return x * x + y * y; }
        Vector2 Normalized() const;
        void Normalize();
        inline F32 Dot(const Vector2& rhs) const { return x * rhs.x + y * rhs.y; }
        inline F32 Cross(const Vector2& rhs) const { return x * rhs.y - y * rhs.x; } // 2D cross product (scalar)

        // Utility functions
        inline Vector2 Perpendicular() const { return { -y, x }; }
        F32 DistanceTo(const Vector2& other) const;
        F32 DistanceSquaredTo(const Vector2& other) const;
        Vector2 Lerp(const Vector2& target, F32 t) const;
        Vector2 Reflect(const Vector2& normal) const;

        // Static constants
        static const Vector2 Zero;
        static const Vector2 One;
        static const Vector2 UnitX;
        static const Vector2 UnitY;
        static const Vector2 Left;
        static const Vector2 Right;
        static const Vector2 Up;
        static const Vector2 Down;
    };

    export Vector2 operator*(F32 scalar, const Vector2& vec);
} // namespace Angaraka::Math