module;

#include <Angaraka/Base.hpp>

export module Angaraka.Math;

namespace Angaraka::Math {

    // Forward declarations
    export struct Vector2;
    export struct Vector3;
    export struct Vector4;
    export struct Matrix4x4;
    export struct Quaternion;
    export struct Transform;
    export struct Color;
    export struct Random;
    export struct BoundingBox;
    export struct Ray;
    export class Frustum;

    export namespace Constants {
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

    export namespace Util {
        // Basic math functions
        template<typename T>
        inline constexpr T Abs(T value) { return value < T(0) ? -value : value; }

        template<typename T>
        inline constexpr T Sign(T value) { return value < T(0) ? T(-1) : (value > T(0) ? T(1) : T(0)); }

        template<typename T>
        inline constexpr T Min(T a, T b) { return a < b ? a : b; }

        template<typename T>
        inline constexpr T Max(T a, T b) { return a > b ? a : b; }

        template<typename T>
        inline constexpr T Min(T a, T b, T c) { return Min(a, Min(b, c)); }

        template<typename T>
        inline constexpr T Max(T a, T b, T c) { return Max(a, Max(b, c)); }

        template<typename T>
        inline constexpr T Clamp(T value, T min, T max) { return Max(min, Min(value, max)); }

        template<typename T>
        inline constexpr T Clamp01(T value) { return Clamp(value, T(0), T(1)); }

        template<typename T>
        inline constexpr T Lerp(T a, T b, T t) { return a + t * (b - a); }

        template<typename T>
        inline constexpr T InverseLerp(T a, T b, T value) { return (value - a) / (b - a); }

        template<typename T>
        inline constexpr T Remap(T value, T fromMin, T fromMax, T toMin, T toMax)
        {
            T t = InverseLerp(fromMin, fromMax, value);
            return Lerp(toMin, toMax, t);
        }

        template<typename T>
        inline constexpr T Square(T value) { return value * value; }

        template<typename T>
        inline constexpr T Cube(T value) { return value * value * value; }

        // Trigonometric functions (degrees)
        F32 SinDegrees(F32 degrees);
        F32 CosDegrees(F32 degrees);
        F32 TanDegrees(F32 degrees);

        // Advanced math functions
        F32 Sqrt(F32 value);
        F32 InverseSqrt(F32 value);  // Fast inverse square root
        F32 Pow(F32 base, F32 exponent);
        F32 Log(F32 value);
        F32 Log2(F32 value);
        F32 Log10(F32 value);
        F32 Exp(F32 value);
        F32 Floor(F32 value);
        F32 Ceil(F32 value);
        F32 Round(F32 value);
        F32 Frac(F32 value);        // Fractional part
        F32 Mod(F32 x, F32 y);    // Floating point modulo

        // Angle utilities
        F32 NormalizeAngle(F32 angle);         // Normalize to [-180, 180]
        F32 NormalizeAngle360(F32 angle);      // Normalize to [0, 360]
        F32 NormalizeAngleRadians(F32 angle);  // Normalize to [-PI, PI]
        F32 AngleDifference(F32 a, F32 b);     // Shortest angle between two angles
        F32 RadiansToDegrees(F32 radians);
        F32 DegreesToRadians(F32 degrees);

        // Comparison with epsilon
        bool IsNearlyEqual(F32 a, F32 b, F32 epsilon = Constants::EpsilonF);
        bool IsNearlyZero(F32 value, F32 epsilon = Constants::EpsilonF);
        bool IsNearlyEqual(const Vector2& a, const Vector2& b, F32 epsilon = Constants::EpsilonF);
        bool IsNearlyEqual(const Vector3& a, const Vector3& b, F32 epsilon = Constants::EpsilonF);
        bool IsNearlyEqual(const Vector4& a, const Vector4& b, F32 epsilon = Constants::EpsilonF);

        // Smoothing functions
        F32 SmoothStep(F32 edge0, F32 edge1, F32 x);
        F32 SmootherStep(F32 edge0, F32 edge1, F32 x);

        // Easing functions
        F32 EaseInQuad(F32 t);
        F32 EaseOutQuad(F32 t);
        F32 EaseInOutQuad(F32 t);
        F32 EaseInCubic(F32 t);
        F32 EaseOutCubic(F32 t);
        F32 EaseInOutCubic(F32 t);
    }

} // namespace Angaraka::Math