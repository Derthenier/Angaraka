// ==================================================================================
// AngarakaMath/Private/Vector2.cpp
// ==================================================================================
module;

#include <Angaraka/Base.hpp>

module Angaraka.Math.Vector2;

import Angaraka.Math;

namespace Angaraka::Math {

    // ==================================================================================
    // Vector2 Implementation
    // ==================================================================================

    const Vector2 Vector2::Zero{ 0.0f, 0.0f };
    const Vector2 Vector2::One{ 1.0f, 1.0f };
    const Vector2 Vector2::UnitX{ 1.0f, 0.0f };
    const Vector2 Vector2::UnitY{ 0.0f, 1.0f };
    const Vector2 Vector2::Left{ -1.0f, 0.0f };
    const Vector2 Vector2::Right{ 1.0f, 0.0f };
    const Vector2 Vector2::Up{ 0.0f, 1.0f };
    const Vector2 Vector2::Down{ 0.0f, -1.0f };

    bool Vector2::operator==(const Vector2& rhs) const
    {
        return Util::IsNearlyEqual(x, rhs.x) && Util::IsNearlyEqual(y, rhs.y);
    }

    F32 Vector2::Length() const
    {
        return Util::Sqrt(x * x + y * y);
    }

    Vector2 Vector2::Normalized() const
    {
        F32 len = Length();
        if (Util::IsNearlyZero(len))
            return Vector2::Zero;
        return *this / len;
    }

    void Vector2::Normalize()
    {
        *this = Normalized();
    }

    F32 Vector2::DistanceTo(const Vector2& other) const
    {
        return (*this - other).Length();
    }

    F32 Vector2::DistanceSquaredTo(const Vector2& other) const
    {
        return (*this - other).LengthSquared();
    }

    Vector2 Vector2::Lerp(const Vector2& target, F32 t) const
    {
        return *this + (target - *this) * t;
    }

    Vector2 Vector2::Reflect(const Vector2& normal) const
    {
        return *this - 2.0f * Dot(normal) * normal;
    }

    Vector2 operator*(F32 scalar, const Vector2& vec)
    {
        return vec * scalar;
    }
}