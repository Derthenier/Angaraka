// ==================================================================================
// AngarakaMath/Public/Math/Constants.hpp
// ==================================================================================

#pragma once

#include <concepts>

namespace Angaraka::Math
{
    // ==================================================================================
    // Constants
    // ==================================================================================

    template<std::floating_point T>
    constexpr T Pi = T(3.14159265358979323846);

    template<std::floating_point T>
    constexpr T TwoPi = T(6.28318530717958647693);

    template<std::floating_point T>
    constexpr T HalfPi = T(1.57079632679489661923);

    template<std::floating_point T>
    constexpr T QuarterPi = T(0.78539816339744830962);

    template<std::floating_point T>
    constexpr T Epsilon = T(0.000001);

    template<std::floating_point T>
    constexpr T DegToRad = Pi<T> / T(180);

    template<std::floating_point T>
    constexpr T RadToDeg = T(180) / Pi<T>;

    // Common F32 constants
    constexpr F32 PiF = Pi<float>;
    constexpr F32 TwoPiF = TwoPi<float>;
    constexpr F32 HalfPiF = HalfPi<float>;
    constexpr F32 EpsilonF = Epsilon<float>;
    constexpr F32 DegToRadF = DegToRad<float>;
    constexpr F32 RadToDegF = RadToDeg<float>;

    constexpr F32 MBToBytes = 1024.0f * 1024.0f;
    constexpr F32 GBToBytes = MBToBytes * 1024.0f;
    constexpr F32 BytesToMB = 0.0000009536743166f;
    constexpr F32 BytesToGB = 0.0000000000931323f;
}