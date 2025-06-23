// ==================================================================================
// AngarakaMath/Public/Math/Quaternion.hpp - Rotation representation
// ==================================================================================

#pragma once

#include "Vector3.hpp"

namespace Angaraka::Math
{
    struct Matrix4x4;

    // ==================================================================================
    // Quaternion - Rotation representation
    // ==================================================================================

    struct Quaternion
    {
        float x, y, z, w;

        // Constructors
        constexpr Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
        constexpr Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
        explicit Quaternion(const Vector3& axis, float angle);
        explicit Quaternion(const Vector3& eulerAngles);
        explicit Quaternion(const Matrix4x4& rotationMatrix);

        // Array access
        float& operator[](size_t index) { return (&x)[index]; }
        const float& operator[](size_t index) const { return (&x)[index]; }

        // Arithmetic operators
        Quaternion operator+(const Quaternion& rhs) const { return { x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w }; }
        Quaternion operator-(const Quaternion& rhs) const { return { x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w }; }
        Quaternion operator*(const Quaternion& rhs) const;
        Quaternion operator*(float scalar) const { return { x * scalar, y * scalar, z * scalar, w * scalar }; }
        Quaternion operator-() const { return { -x, -y, -z, -w }; }

        // Assignment operators
        Quaternion& operator+=(const Quaternion& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }
        Quaternion& operator-=(const Quaternion& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return *this; }
        Quaternion& operator*=(const Quaternion& rhs);
        Quaternion& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }

        // Comparison
        bool operator==(const Quaternion& rhs) const;
        bool operator!=(const Quaternion& rhs) const { return !(*this == rhs); }

        // Quaternion operations
        float Length() const;
        float LengthSquared() const { return x * x + y * y + z * z + w * w; }
        Quaternion Normalized() const;
        void Normalize();
        Quaternion Conjugated() const { return { -x, -y, -z, w }; }
        Quaternion Inverted() const;
        float Dot(const Quaternion& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w; }

        // Rotation operations
        Vector3 RotateVector(const Vector3& vec) const;
        Matrix4x4 ToMatrix() const;
        Vector3 ToEulerAngles() const;
        float GetAngle() const;
        Vector3 GetAxis() const;

        // Static factory methods
        static Quaternion Identity();
        static Quaternion AngleAxis(const Vector3& axis, float angle);
        static Quaternion AngleAxis(float axisX, float axisY, float axisZ, float angle);
        static Quaternion FromEuler(float pitch, float yaw, float roll);
        static Quaternion FromEuler(const Vector3& eulerAngles);
        static Quaternion FromMatrix(const Matrix4x4& matrix);
        static Quaternion LookRotation(const Vector3& forward, const Vector3& up = Vector3::Up);
        static Quaternion FromToRotation(const Vector3& from, const Vector3& to);

        // Interpolation
        static Quaternion Lerp(const Quaternion& a, const Quaternion& b, float t);
        static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t);
        static Quaternion Squad(const Quaternion& a, const Quaternion& b, const Quaternion& c, const Quaternion& d, float t);
    };
}