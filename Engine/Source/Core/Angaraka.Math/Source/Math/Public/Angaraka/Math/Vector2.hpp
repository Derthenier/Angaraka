// ==================================================================================
// AngarakaMath/Public/Math/Vector2.hpp - 2D Vector
// ==================================================================================

#pragma once

namespace Angaraka::Math
{
    // ==================================================================================
    // Vector2 - 2D Vector
    // ==================================================================================

    struct Vector2
    {
        float x, y;

        // Constructors
        constexpr Vector2() : x(0.0f), y(0.0f) {}
        constexpr Vector2(float x, float y) : x(x), y(y) {}
        constexpr explicit Vector2(float value) : x(value), y(value) {}

        // Array access
        float& operator[](size_t index) { return (&x)[index]; }
        const float& operator[](size_t index) const { return (&x)[index]; }

        // Arithmetic operators
        Vector2 operator+(const Vector2& rhs) const { return { x + rhs.x, y + rhs.y }; }
        Vector2 operator-(const Vector2& rhs) const { return { x - rhs.x, y - rhs.y }; }
        Vector2 operator*(float scalar) const { return { x * scalar, y * scalar }; }
        Vector2 operator/(float scalar) const { return { x / scalar, y / scalar }; }
        Vector2 operator-() const { return { -x, -y }; }

        // Component-wise multiplication
        Vector2 operator*(const Vector2& rhs) const { return { x * rhs.x, y * rhs.y }; }
        Vector2 operator/(const Vector2& rhs) const { return { x / rhs.x, y / rhs.y }; }

        // Assignment operators
        Vector2& operator+=(const Vector2& rhs) { x += rhs.x; y += rhs.y; return *this; }
        Vector2& operator-=(const Vector2& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
        Vector2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
        Vector2& operator/=(float scalar) { x /= scalar; y /= scalar; return *this; }
        Vector2& operator*=(const Vector2& rhs) { x *= rhs.x; y *= rhs.y; return *this; }
        Vector2& operator/=(const Vector2& rhs) { x /= rhs.x; y /= rhs.y; return *this; }

        // Comparison
        bool operator==(const Vector2& rhs) const;
        bool operator!=(const Vector2& rhs) const { return !(*this == rhs); }

        // Vector operations
        float Length() const;
        float LengthSquared() const { return x * x + y * y; }
        Vector2 Normalized() const;
        void Normalize();
        float Dot(const Vector2& rhs) const { return x * rhs.x + y * rhs.y; }
        float Cross(const Vector2& rhs) const { return x * rhs.y - y * rhs.x; } // 2D cross product (scalar)

        // Utility functions
        Vector2 Perpendicular() const { return { -y, x }; }
        float DistanceTo(const Vector2& other) const;
        float DistanceSquaredTo(const Vector2& other) const;
        Vector2 Lerp(const Vector2& target, float t) const;
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

    // Non-member operators
    Vector2 operator*(float scalar, const Vector2& vec);
}