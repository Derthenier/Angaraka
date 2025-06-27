// Engine/Source/Core/Angaraka.Math/Source/Math/Public/Angaraka/MathCore.hpp
#pragma once

#include <Angaraka/Base.hpp>

#include "Angaraka/Math/Constants.hpp"
#include "Angaraka/Math/Vector2.hpp"
#include "Angaraka/Math/Vector3.hpp"
#include "Angaraka/Math/Vector4.hpp"
#include "Angaraka/Math/Matrix44.hpp"
#include "Angaraka/Math/Quaternion.hpp"
#include "Angaraka/Math/Transform.hpp"
#include "Angaraka/Math/Random.hpp"

#include <array>
#include <string>

namespace Angaraka::Math {
    // ==================================================================================
    // Utility Functions
    // ==================================================================================

    // Basic math functions
    template<typename T>
    constexpr T Abs(T value) { return value < T(0) ? -value : value; }

    template<typename T>
    constexpr T Sign(T value) { return value < T(0) ? T(-1) : (value > T(0) ? T(1) : T(0)); }

    template<typename T>
    constexpr T Min(T a, T b) { return a < b ? a : b; }

    template<typename T>
    constexpr T Max(T a, T b) { return a > b ? a : b; }

    template<typename T>
    constexpr T Min(T a, T b, T c) { return Min(a, Min(b, c)); }

    template<typename T>
    constexpr T Max(T a, T b, T c) { return Max(a, Max(b, c)); }

    template<typename T>
    constexpr T Clamp(T value, T min, T max) { return Max(min, Min(value, max)); }

    template<typename T>
    constexpr T Clamp01(T value) { return Clamp(value, T(0), T(1)); }

    template<typename T>
    constexpr T Lerp(T a, T b, T t) { return a + t * (b - a); }

    template<typename T>
    constexpr T InverseLerp(T a, T b, T value) { return (value - a) / (b - a); }

    template<typename T>
    constexpr T Remap(T value, T fromMin, T fromMax, T toMin, T toMax)
    {
        T t = InverseLerp(fromMin, fromMax, value);
        return Lerp(toMin, toMax, t);
    }

    template<typename T>
    constexpr T Square(T value) { return value * value; }

    template<typename T>
    constexpr T Cube(T value) { return value * value * value; }

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
    F32 NormalizeAngle(F32 angle);           // Normalize to [-180, 180]
    F32 NormalizeAngle360(F32 angle);        // Normalize to [0, 360]
    F32 NormalizeAngleRadians(F32 angle);    // Normalize to [-PI, PI]
    F32 AngleDifference(F32 a, F32 b);     // Shortest angle between two angles

    // Comparison with epsilon
    bool IsNearlyEqual(F32 a, F32 b, F32 epsilon = EpsilonF);
    bool IsNearlyZero(F32 value, F32 epsilon = EpsilonF);
    bool IsNearlyEqual(const Vector2& a, const Vector2& b, F32 epsilon = EpsilonF);
    bool IsNearlyEqual(const Vector3& a, const Vector3& b, F32 epsilon = EpsilonF);
    bool IsNearlyEqual(const Vector4& a, const Vector4& b, F32 epsilon = EpsilonF);

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

    // ==================================================================================
    // Geometric Utilities
    // ==================================================================================

    // Distance functions
    F32 PointToLineDistance(const Vector3& point, const Vector3& lineStart, const Vector3& lineEnd);
    F32 PointToPlaneDistance(const Vector3& point, const Vector3& planePoint, const Vector3& planeNormal);

    // Intersection tests
    bool RayIntersectsSphere(const Vector3& rayOrigin, const Vector3& rayDirection,
        const Vector3& sphereCenter, F32 sphereRadius,
        float& t1, float& t2);

    bool RayIntersectsPlane(const Vector3& rayOrigin, const Vector3& rayDirection,
        const Vector3& planePoint, const Vector3& planeNormal,
        float& t);

    bool LineSegmentIntersect2D(const Vector2& a1, const Vector2& a2,
        const Vector2& b1, const Vector2& b2,
        Vector2& intersection);

    // Area/Volume calculations
    F32 TriangleArea(const Vector3& a, const Vector3& b, const Vector3& c);
    F32 TriangleArea2D(const Vector2& a, const Vector2& b, const Vector2& c);
    Vector3 TriangleNormal(const Vector3& a, const Vector3& b, const Vector3& c);
    Vector3 TriangleCenter(const Vector3& a, const Vector3& b, const Vector3& c);

    // Barycentric coordinates
    Vector3 BarycentricCoordinates(const Vector3& point, const Vector3& a, const Vector3& b, const Vector3& c);
    bool IsInsideTriangle(const Vector3& barycentrics);

    // Color space conversions
    Vector3 HSVToRGB(F32 h, F32 s, F32 v);
    Vector3 HSVToRGB(const Vector3& hsv);
    Vector3 RGBToHSV(F32 r, F32 g, F32 b);
    Vector3 RGBToHSV(const Vector3& rgb);
    Vector3 HSLToRGB(F32 h, F32 s, F32 l);
    Vector3 HSLToRGB(const Vector3& hsl);
    Vector3 RGBToHSL(F32 r, F32 g, F32 b);
    Vector3 RGBToHSL(const Vector3& rgb);
    Vector3 RGBFromBytes(U8 r, U8 g, U8 b);
    Vector3 RGBToBytes(const Vector3& rgb);
    U32 PackRGBA(const Vector3& rgb, F32 alpha = 1.0f);
    Vector4 UnpackRGBA(U32 packedColor);
    Vector3 LinearToGamma(const Vector3& linearColor, F32 gamma = 2.2f);
    Vector3 GammaToLinear(const Vector3& gammaColor, F32 gamma = 2.2f);
    Vector3 ColorLerp(const Vector3& colorA, const Vector3& colorB, F32 t);
    Vector3 ColorLerpHSV(const Vector3& rgbA, const Vector3& rgbB, F32 t);
    Vector3 AdjustBrightness(const Vector3& rgb, F32 brightness);
    Vector3 AdjustSaturation(const Vector3& rgb, F32 saturation);
    Vector3 AdjustHue(const Vector3& rgb, F32 hueShift);
    Vector3 AdjustContrast(const Vector3& rgb, F32 contrast);
    Vector3 ColorMultiply(const Vector3& base, const Vector3& blend);
    Vector3 ColorScreen(const Vector3& base, const Vector3& blend);
    Vector3 ColorOverlay(const Vector3& base, const Vector3& blend);
    Vector3 ColorSoftLight(const Vector3& base, const Vector3& blend);

    // Common color constants
    namespace Colors
    {
        const Vector3 Black = Vector3(0.0f, 0.0f, 0.0f);
        const Vector3 White = Vector3(1.0f, 1.0f, 1.0f);
        const Vector3 Red = Vector3(1.0f, 0.0f, 0.0f);
        const Vector3 Green = Vector3(0.0f, 1.0f, 0.0f);
        const Vector3 Blue = Vector3(0.0f, 0.0f, 1.0f);
        const Vector3 Yellow = Vector3(1.0f, 1.0f, 0.0f);
        const Vector3 Cyan = Vector3(0.0f, 1.0f, 1.0f);
        const Vector3 Magenta = Vector3(1.0f, 0.0f, 1.0f);
        const Vector3 Gray = Vector3(0.5f, 0.5f, 0.5f);
        const Vector3 Orange = Vector3(1.0f, 0.5f, 0.0f);
        const Vector3 Purple = Vector3(0.5f, 0.0f, 1.0f);
        const Vector3 Pink = Vector3(1.0f, 0.75f, 0.8f);
        const Vector3 Brown = Vector3(0.6f, 0.3f, 0.0f);
    }
} // namespace Angaraka::Math