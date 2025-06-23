// ==================================================================================
// AngarakaMath/Private/Vector2.cpp
// ==================================================================================

#include "Angaraka/MathCore.hpp"
#include "Angaraka/Math/Vector2.hpp"

namespace Angaraka::Math
{
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
        return IsNearlyEqual(x, rhs.x) && IsNearlyEqual(y, rhs.y);
    }

    float Vector2::Length() const
    {
        return Sqrt(x * x + y * y);
    }

    Vector2 Vector2::Normalized() const
    {
        float len = Length();
        if (IsNearlyZero(len))
            return Vector2::Zero;
        return *this / len;
    }

    void Vector2::Normalize()
    {
        *this = Normalized();
    }

    float Vector2::DistanceTo(const Vector2& other) const
    {
        return (*this - other).Length();
    }

    float Vector2::DistanceSquaredTo(const Vector2& other) const
    {
        return (*this - other).LengthSquared();
    }

    Vector2 Vector2::Lerp(const Vector2& target, float t) const
    {
        return *this + (target - *this) * t;
    }

    Vector2 Vector2::Reflect(const Vector2& normal) const
    {
        return *this - 2.0f * Dot(normal) * normal;
    }

    Vector2 operator*(float scalar, const Vector2& vec)
    {
        return vec * scalar;
    }
}