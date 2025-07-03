module;

#include <Angaraka/Base.hpp>
#include <cstring>
#include <cmath>

module Angaraka.Math;

import Angaraka.Math.Vector2;
import Angaraka.Math.Vector3;
import Angaraka.Math.Vector4;

namespace Angaraka::Math {

    // ==================================================================================
    // Utility Functions Implementation
    // ==================================================================================

    F32 Util::SinDegrees(F32 degrees)
    {
        return std::sin(degrees * Constants::DegToRadF);
    }

    F32 Util::CosDegrees(F32 degrees)
    {
        return std::cos(degrees * Constants::DegToRadF);
    }

    F32 Util::TanDegrees(F32 degrees)
    {
        return std::tan(degrees * Constants::DegToRadF);
    }

    F32 Util::Sqrt(F32 value)
    {
        return std::sqrt(value);
    }

    F32 Util::InverseSqrt(F32 value)
    {
        // Fast inverse square root approximation
        if (IsNearlyZero(value))
            return 0.0f;
        return 1.0f / std::sqrt(value);
    }

    F32 Util::Pow(F32 base, F32 exponent)
    {
        return std::pow(base, exponent);
    }

    F32 Util::Log(F32 value)
    {
        return std::log(value);
    }

    F32 Util::Log2(F32 value)
    {
        return std::log2(value);
    }

    F32 Util::Log10(F32 value)
    {
        return std::log10(value);
    }

    F32 Util::Exp(F32 value)
    {
        return std::exp(value);
    }

    F32 Util::Floor(F32 value)
    {
        return std::floor(value);
    }

    F32 Util::Ceil(F32 value)
    {
        return std::ceil(value);
    }

    F32 Util::Round(F32 value)
    {
        return std::round(value);
    }

    F32 Util::Frac(F32 value)
    {
        return value - Floor(value);
    }

    F32 Util::Mod(F32 x, F32 y)
    {
        return std::fmod(x, y);
    }

    F32 Util::NormalizeAngle(F32 angle)
    {
        while (angle > 180.0f) angle -= 360.0f;
        while (angle < -180.0f) angle += 360.0f;
        return angle;
    }

    F32 Util::NormalizeAngle360(F32 angle)
    {
        while (angle >= 360.0f) angle -= 360.0f;
        while (angle < 0.0f) angle += 360.0f;
        return angle;
    }

    F32 Util::NormalizeAngleRadians(F32 angle)
    {
        while (angle > Constants::PiF) angle -= Constants::TwoPiF;
        while (angle < -Constants::PiF) angle += Constants::TwoPiF;
        return angle;
    }

    F32 Util::AngleDifference(F32 a, F32 b)
    {
        F32 diff = NormalizeAngle(b - a);
        return diff;
    }

    F32 Util::RadiansToDegrees(F32 radians)
    {
        return radians * Constants::RadToDegF;
    }

    F32 Util::DegreesToRadians(F32 degrees)
    {
        return degrees * Constants::DegToRadF;
    }

    bool Util::IsNearlyEqual(F32 a, F32 b, F32 epsilon)
    {
        return Abs(a - b) <= epsilon;
    }

    bool Util::IsNearlyZero(F32 value, F32 epsilon)
    {
        return Abs(value) <= epsilon;
    }

    bool Util::IsNearlyEqual(const Vector2& a, const Vector2& b, F32 epsilon)
    {
        return IsNearlyEqual(a.x, b.x, epsilon) && IsNearlyEqual(a.y, b.y, epsilon);
    }

    bool Util::IsNearlyEqual(const Vector3& a, const Vector3& b, F32 epsilon)
    {
        return IsNearlyEqual(a.x, b.x, epsilon) && IsNearlyEqual(a.y, b.y, epsilon) && IsNearlyEqual(a.z, b.z, epsilon);
    }

    bool Util::IsNearlyEqual(const Vector4& a, const Vector4& b, F32 epsilon)
    {
        return IsNearlyEqual(a.x, b.x, epsilon) && IsNearlyEqual(a.y, b.y, epsilon) &&
            IsNearlyEqual(a.z, b.z, epsilon) && IsNearlyEqual(a.w, b.w, epsilon);
    }

    F32 Util::SmoothStep(F32 edge0, F32 edge1, F32 x)
    {
        F32 t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    F32 Util::SmootherStep(F32 edge0, F32 edge1, F32 x)
    {
        F32 t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    // Easing functions
    F32 Util::EaseInQuad(F32 t) { return t * t; }
    F32 Util::EaseOutQuad(F32 t) { return 1.0f - (1.0f - t) * (1.0f - t); }
    F32 Util::EaseInOutQuad(F32 t) { return t < 0.5f ? 2.0f * t * t : 1.0f - Pow(-2.0f * t + 2.0f, 2.0f) / 2.0f; }
    F32 Util::EaseInCubic(F32 t) { return t * t * t; }
    F32 Util::EaseOutCubic(F32 t) { return 1.0f - Pow(1.0f - t, 3.0f); }
    F32 Util::EaseInOutCubic(F32 t) { return t < 0.5f ? 4.0f * t * t * t : 1.0f - Pow(-2.0f * t + 2.0f, 3.0f) / 2.0f; }

} // namespace Angaraka::Math