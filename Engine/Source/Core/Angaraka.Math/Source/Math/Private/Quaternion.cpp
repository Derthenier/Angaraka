// ==================================================================================
// AngarakaMath/Public/Math/Quaternion.cpp
// ==================================================================================

#pragma once

#include "Angaraka/MathCore.hpp"
#include "Angaraka/Math/Quaternion.hpp"
#include <cmath>

namespace Angaraka::Math
{
    // ==================================================================================
    // Quaternion Implementation
    // ==================================================================================

    Quaternion::Quaternion(const Vector3& axis, F32 angle)
    {
        F32 halfAngle = angle * 0.5f;
        F32 s = std::sin(halfAngle);
        Vector3 normalizedAxis = axis.Normalized();

        x = normalizedAxis.x * s;
        y = normalizedAxis.y * s;
        z = normalizedAxis.z * s;
        w = std::cos(halfAngle);
    }

    Quaternion::Quaternion(const Vector3& eulerAngles)
    {
        *this = FromEuler(eulerAngles);
    }

    Quaternion Quaternion::operator*(const Quaternion& rhs) const
    {
        return {
            w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
            w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
            w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w,
            w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z
        };
    }

    Quaternion& Quaternion::operator*=(const Quaternion& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    bool Quaternion::operator==(const Quaternion& rhs) const
    {
        return IsNearlyEqual(x, rhs.x) && IsNearlyEqual(y, rhs.y) &&
            IsNearlyEqual(z, rhs.z) && IsNearlyEqual(w, rhs.w);
    }

    F32 Quaternion::Length() const
    {
        return Sqrt(x * x + y * y + z * z + w * w);
    }

    Quaternion Quaternion::Normalized() const
    {
        F32 len = Length();
        if (IsNearlyZero(len))
            return Identity();
        return *this * (1.0f / len);
    }

    void Quaternion::Normalize()
    {
        *this = Normalized();
    }

    Quaternion Quaternion::Inverted() const
    {
        F32 lengthSq = LengthSquared();
        if (IsNearlyZero(lengthSq))
            return Identity();

        Quaternion conj = Conjugated();
        return conj * (1.0f / lengthSq);
    }

    Vector3 Quaternion::RotateVector(const Vector3& vec) const
    {
        // Optimized quaternion-vector multiplication
        Vector3 qvec(x, y, z);
        Vector3 uv = qvec.Cross(vec);
        Vector3 uuv = qvec.Cross(uv);

        return vec + ((uv * w) + uuv) * 2.0f;
    }

    Matrix4x4 Quaternion::ToMatrix() const
    {
        F32 xx = x * x;
        F32 yy = y * y;
        F32 zz = z * z;
        F32 xy = x * y;
        F32 xz = x * z;
        F32 yz = y * z;
        F32 wx = w * x;
        F32 wy = w * y;
        F32 wz = w * z;

        return Matrix4x4(
            1.0f - 2.0f * (yy + zz), 2.0f * (xy - wz), 2.0f * (xz + wy), 0.0f,
            2.0f * (xy + wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz - wx), 0.0f,
            2.0f * (xz - wy), 2.0f * (yz + wx), 1.0f - 2.0f * (xx + yy), 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );
    }

    Vector3 Quaternion::ToEulerAngles() const
    {
        Vector3 angles;

        // Roll (x-axis rotation)
        F32 sinr_cosp = 2.0f * (w * x + y * z);
        F32 cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
        angles.x = std::atan2(sinr_cosp, cosr_cosp);

        // Pitch (y-axis rotation)
        F32 sinp = 2.0f * (w * y - z * x);
        if (Abs(sinp) >= 1.0f)
            angles.y = std::copysign(HalfPiF, sinp); // Use 90 degrees if out of range
        else
            angles.y = std::asin(sinp);

        // Yaw (z-axis rotation)
        F32 siny_cosp = 2.0f * (w * z + x * y);
        F32 cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
        angles.z = std::atan2(siny_cosp, cosy_cosp);

        return angles;
    }

    Quaternion Quaternion::Identity()
    {
        return { 0.0f, 0.0f, 0.0f, 1.0f };
    }

    Quaternion Quaternion::AngleAxis(const Vector3& axis, F32 angle)
    {
        return Quaternion(axis, angle);
    }

    Quaternion Quaternion::FromEuler(F32 pitch, F32 yaw, F32 roll)
    {
        F32 halfPitch = pitch * 0.5f;
        F32 halfYaw = yaw * 0.5f;
        F32 halfRoll = roll * 0.5f;

        F32 cp = std::cos(halfPitch);
        F32 sp = std::sin(halfPitch);
        F32 cy = std::cos(halfYaw);
        F32 sy = std::sin(halfYaw);
        F32 cr = std::cos(halfRoll);
        F32 sr = std::sin(halfRoll);

        return {
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy,
            cr * cp * cy + sr * sp * sy
        };
    }

    Quaternion Quaternion::FromEuler(const Vector3& eulerAngles)
    {
        return FromEuler(eulerAngles.x, eulerAngles.y, eulerAngles.z);
    }

    Quaternion Quaternion::Slerp(const Quaternion& a, const Quaternion& b, F32 t)
    {
        Quaternion q1 = a.Normalized();
        Quaternion q2 = b.Normalized();

        F32 dot = q1.Dot(q2);

        // If the dot product is negative, slerp won't take the shorter path.
        // Note that v1 and -v1 are equivalent when the represent rotations.
        if (dot < 0.0f)
        {
            q2 = -q2;
            dot = -dot;
        }

        // If the inputs are too close for comfort, linearly interpolate
        if (dot > 0.9995f)
        {
            return (q1 + (q2 - q1) * t).Normalized();
        }

        F32 theta_0 = std::acos(dot);
        F32 theta = theta_0 * t;
        F32 sin_theta = std::sin(theta);
        F32 sin_theta_0 = std::sin(theta_0);

        F32 s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
        F32 s1 = sin_theta / sin_theta_0;

        return (q1 * s0) + (q2 * s1);
    }
}