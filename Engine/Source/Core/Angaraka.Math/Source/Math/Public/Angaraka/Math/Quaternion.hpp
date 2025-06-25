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
        F32 x, y, z, w;

        // Constructors
        constexpr Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
        constexpr Quaternion(F32 x, F32 y, F32 z, F32 w) : x(x), y(y), z(z), w(w) {}
        explicit Quaternion(const Vector3& axis, F32 angle);
        explicit Quaternion(const Vector3& eulerAngles);
        explicit Quaternion(const Matrix4x4& rotationMatrix);

        // Array access
        float& operator[](size_t index) { return (&x)[index]; }
        const float& operator[](size_t index) const { return (&x)[index]; }

        // Arithmetic operators
        Quaternion operator+(const Quaternion& rhs) const { return { x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w }; }
        Quaternion operator-(const Quaternion& rhs) const { return { x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w }; }
        Quaternion operator*(const Quaternion& rhs) const;
        Quaternion operator*(F32 scalar) const { return { x * scalar, y * scalar, z * scalar, w * scalar }; }
        Quaternion operator-() const { return { -x, -y, -z, -w }; }

        // Assignment operators
        Quaternion& operator+=(const Quaternion& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }
        Quaternion& operator-=(const Quaternion& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return *this; }
        Quaternion& operator*=(const Quaternion& rhs);
        Quaternion& operator*=(F32 scalar) { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }

        // Comparison
        bool operator==(const Quaternion& rhs) const;
        bool operator!=(const Quaternion& rhs) const { return !(*this == rhs); }

        // Quaternion operations
        F32 Length() const;
        F32 LengthSquared() const { return x * x + y * y + z * z + w * w; }
        Quaternion Normalized() const;
        void Normalize();
        Quaternion Conjugated() const { return { -x, -y, -z, w }; }
        Quaternion Inverted() const;
        F32 Dot(const Quaternion& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w; }

        // Rotation operations
        Vector3 RotateVector(const Vector3& vec) const;
        Matrix4x4 ToMatrix() const;
        Vector3 ToEulerAngles() const;
        F32 GetAngle() const;
        Vector3 GetAxis() const;

        // Static factory methods
        static Quaternion Identity();
        static Quaternion AngleAxis(const Vector3& axis, F32 angle);
        static Quaternion AngleAxis(F32 axisX, F32 axisY, F32 axisZ, F32 angle);
        static Quaternion FromEuler(F32 pitch, F32 yaw, F32 roll);
        static Quaternion FromEuler(const Vector3& eulerAngles);
        static Quaternion FromMatrix(const Matrix4x4& matrix);
        static Quaternion LookRotation(const Vector3& forward, const Vector3& up = Vector3::Up);
        static Quaternion FromToRotation(const Vector3& from, const Vector3& to);

        // Interpolation
        static Quaternion Lerp(const Quaternion& a, const Quaternion& b, F32 t);
        static Quaternion Slerp(const Quaternion& a, const Quaternion& b, F32 t);
        static Quaternion Squad(const Quaternion& a, const Quaternion& b, const Quaternion& c, const Quaternion& d, F32 t);
    };
}