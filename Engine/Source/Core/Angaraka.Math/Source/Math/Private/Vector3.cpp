// ==================================================================================
// AngarakaMath/Private/Vector3.cpp
// ==================================================================================

#include "Angaraka/MathCore.hpp"
#include "Angaraka/Math/Vector3.hpp"
#include <cmath>

namespace Angaraka::Math
{
    // ==================================================================================
    // Vector3 Implementation
    // ==================================================================================

    const Vector3 Vector3::Zero{ 0.0f, 0.0f, 0.0f };
    const Vector3 Vector3::One{ 1.0f, 1.0f, 1.0f };
    const Vector3 Vector3::UnitX{ 1.0f, 0.0f, 0.0f };
    const Vector3 Vector3::UnitY{ 0.0f, 1.0f, 0.0f };
    const Vector3 Vector3::UnitZ{ 0.0f, 0.0f, 1.0f };
    const Vector3 Vector3::Forward{ 0.0f, 0.0f, -1.0f };  // -Z (OpenGL/Vulkan style)
    const Vector3 Vector3::Back{ 0.0f, 0.0f, 1.0f };
    const Vector3 Vector3::Up{ 0.0f, 1.0f, 0.0f };
    const Vector3 Vector3::Down{ 0.0f, -1.0f, 0.0f };
    const Vector3 Vector3::Left{ -1.0f, 0.0f, 0.0f };
    const Vector3 Vector3::Right{ 1.0f, 0.0f, 0.0f };

    bool Vector3::operator==(const Vector3& rhs) const
    {
        return IsNearlyEqual(x, rhs.x) && IsNearlyEqual(y, rhs.y) && IsNearlyEqual(z, rhs.z);
    }

    F32 Vector3::Length() const
    {
        return Sqrt(x * x + y * y + z * z);
    }

    Vector3 Vector3::Normalized() const
    {
        F32 len = Length();
        if (IsNearlyZero(len))
            return Vector3::Zero;
        return *this / len;
    }

    void Vector3::Normalize()
    {
        *this = Normalized();
    }

    Vector3 Vector3::Cross(const Vector3& rhs) const
    {
        return {
            y * rhs.z - z * rhs.y,
            z * rhs.x - x * rhs.z,
            x * rhs.y - y * rhs.x
        };
    }

    F32 Vector3::DistanceTo(const Vector3& other) const
    {
        return (*this - other).Length();
    }

    F32 Vector3::DistanceSquaredTo(const Vector3& other) const
    {
        return (*this - other).LengthSquared();
    }

    Vector3 Vector3::Lerp(const Vector3& target, F32 t) const
    {
        return *this + (target - *this) * t;
    }

    Vector3 Vector3::Slerp(const Vector3& target, F32 t) const
    {
        F32 dot = Clamp(Dot(target), -1.0f, 1.0f);
        F32 theta = std::acos(dot) * t;
        Vector3 relative = (target - *this * dot).Normalized();
        return (*this * std::cos(theta)) + (relative * std::sin(theta));
    }

    Vector3 Vector3::Reflect(const Vector3& normal) const
    {
        return *this - 2.0f * Dot(normal) * normal;
    }

    Vector3 Vector3::Project(const Vector3& onto) const
    {
        F32 dot = Dot(onto);
        F32 lengthSq = onto.LengthSquared();
        if (IsNearlyZero(lengthSq))
            return Vector3::Zero;
        return onto * (dot / lengthSq);
    }

    Vector3 Vector3::Reject(const Vector3& from) const
    {
        return *this - Project(from);
    }

    Vector3 operator*(F32 scalar, const Vector3& vec)
    {
        return vec * scalar;
    }
}