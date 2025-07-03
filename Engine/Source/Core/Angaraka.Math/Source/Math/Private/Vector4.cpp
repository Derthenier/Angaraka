// ==================================================================================
// AngarakaMath/Private/Vector4.cpp
// ==================================================================================
module;

#include <Angaraka/Base.hpp>
#include <cmath>

module Angaraka.Math.Vector4;

import Angaraka.Math;

namespace Angaraka::Math {

    // ==================================================================================
    // Vector4 Implementation
    // ==================================================================================

    const Vector4 Vector4::Zero{ 0.0f, 0.0f, 0.0f, 0.0f };
    const Vector4 Vector4::One{ 1.0f, 1.0f, 1.0f, 1.0f };
    const Vector4 Vector4::UnitX{ 1.0f, 0.0f, 0.0f, 0.0f };
    const Vector4 Vector4::UnitY{ 0.0f, 1.0f, 0.0f, 0.0f };
    const Vector4 Vector4::UnitZ{ 0.0f, 0.0f, 1.0f, 0.0f };
    const Vector4 Vector4::UnitW{ 0.0f, 0.0f, 0.0f, 1.0f };

    bool Vector4::operator==(const Vector4& rhs) const
    {
        return Util::IsNearlyEqual(x, rhs.x) && Util::IsNearlyEqual(y, rhs.y) &&
            Util::IsNearlyEqual(z, rhs.z) && Util::IsNearlyEqual(w, rhs.w);
    }

    F32 Vector4::Length() const
    {
        return Util::Sqrt(x * x + y * y + z * z + w * w);
    }

    Vector4 Vector4::Normalized() const
    {
        F32 len = Length();
        if (Util::IsNearlyZero(len))
            return Vector4::Zero;
        return *this / len;
    }

    void Vector4::Normalize()
    {
        *this = Normalized();
    }

    Vector4 Vector4::Lerp(const Vector4& target, F32 t) const
    {
        return *this + (target - *this) * t;
    }

    Vector4 operator*(F32 scalar, const Vector4& vec)
    {
        return vec * scalar;
    }
}