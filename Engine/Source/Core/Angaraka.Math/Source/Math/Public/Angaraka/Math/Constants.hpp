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

    // Common float constants
    constexpr float PiF = Pi<float>;
    constexpr float TwoPiF = TwoPi<float>;
    constexpr float HalfPiF = HalfPi<float>;
    constexpr float EpsilonF = Epsilon<float>;
    constexpr float DegToRadF = DegToRad<float>;
    constexpr float RadToDegF = RadToDeg<float>;

    constexpr float MBToBytes = 1024.0f * 1024.0f;
    constexpr float GBToBytes = MBToBytes * 1024.0f;
    constexpr float BytesToMB = 0.0000009536743166f;
    constexpr float BytesToGB = 0.0000000000931323f;
}