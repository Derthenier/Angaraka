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

#include <DirectXMath.h>
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
    float SinDegrees(float degrees);
    float CosDegrees(float degrees);
    float TanDegrees(float degrees);

    // Advanced math functions
    float Sqrt(float value);
    float InverseSqrt(float value);  // Fast inverse square root
    float Pow(float base, float exponent);
    float Log(float value);
    float Log2(float value);
    float Log10(float value);
    float Exp(float value);
    float Floor(float value);
    float Ceil(float value);
    float Round(float value);
    float Frac(float value);        // Fractional part
    float Mod(float x, float y);    // Floating point modulo

    // Angle utilities
    float NormalizeAngle(float angle);           // Normalize to [-180, 180]
    float NormalizeAngle360(float angle);        // Normalize to [0, 360]
    float NormalizeAngleRadians(float angle);    // Normalize to [-PI, PI]
    float AngleDifference(float a, float b);     // Shortest angle between two angles

    // Comparison with epsilon
    bool IsNearlyEqual(float a, float b, float epsilon = EpsilonF);
    bool IsNearlyZero(float value, float epsilon = EpsilonF);
    bool IsNearlyEqual(const Vector2& a, const Vector2& b, float epsilon = EpsilonF);
    bool IsNearlyEqual(const Vector3& a, const Vector3& b, float epsilon = EpsilonF);
    bool IsNearlyEqual(const Vector4& a, const Vector4& b, float epsilon = EpsilonF);

    // Smoothing functions
    float SmoothStep(float edge0, float edge1, float x);
    float SmootherStep(float edge0, float edge1, float x);

    // Easing functions
    float EaseInQuad(float t);
    float EaseOutQuad(float t);
    float EaseInOutQuad(float t);
    float EaseInCubic(float t);
    float EaseOutCubic(float t);
    float EaseInOutCubic(float t);

    // ==================================================================================
    // Geometric Utilities
    // ==================================================================================

    // Distance functions
    float PointToLineDistance(const Vector3& point, const Vector3& lineStart, const Vector3& lineEnd);
    float PointToPlaneDistance(const Vector3& point, const Vector3& planePoint, const Vector3& planeNormal);

    // Intersection tests
    bool RayIntersectsSphere(const Vector3& rayOrigin, const Vector3& rayDirection,
        const Vector3& sphereCenter, float sphereRadius,
        float& t1, float& t2);

    bool RayIntersectsPlane(const Vector3& rayOrigin, const Vector3& rayDirection,
        const Vector3& planePoint, const Vector3& planeNormal,
        float& t);

    bool LineSegmentIntersect2D(const Vector2& a1, const Vector2& a2,
        const Vector2& b1, const Vector2& b2,
        Vector2& intersection);

    // Area/Volume calculations
    float TriangleArea(const Vector3& a, const Vector3& b, const Vector3& c);
    float TriangleArea2D(const Vector2& a, const Vector2& b, const Vector2& c);
    Vector3 TriangleNormal(const Vector3& a, const Vector3& b, const Vector3& c);
    Vector3 TriangleCenter(const Vector3& a, const Vector3& b, const Vector3& c);

    // Barycentric coordinates
    Vector3 BarycentricCoordinates(const Vector3& point, const Vector3& a, const Vector3& b, const Vector3& c);
    bool IsInsideTriangle(const Vector3& barycentrics);

    // Color space conversions
    Vector3 HSVToRGB(float h, float s, float v);
    Vector3 HSVToRGB(const Vector3& hsv);
    Vector3 RGBToHSV(float r, float g, float b);
    Vector3 RGBToHSV(const Vector3& rgb);
    Vector3 HSLToRGB(float h, float s, float l);
    Vector3 HSLToRGB(const Vector3& hsl);
    Vector3 RGBToHSL(float r, float g, float b);
    Vector3 RGBToHSL(const Vector3& rgb);
    Vector3 RGBFromBytes(uint8_t r, uint8_t g, uint8_t b);
    Vector3 RGBToBytes(const Vector3& rgb);
    uint32_t PackRGBA(const Vector3& rgb, float alpha = 1.0f);
    Vector4 UnpackRGBA(uint32_t packedColor);
    Vector3 LinearToGamma(const Vector3& linearColor, float gamma = 2.2f);
    Vector3 GammaToLinear(const Vector3& gammaColor, float gamma = 2.2f);
    Vector3 ColorLerp(const Vector3& colorA, const Vector3& colorB, float t);
    Vector3 ColorLerpHSV(const Vector3& rgbA, const Vector3& rgbB, float t);
    Vector3 AdjustBrightness(const Vector3& rgb, float brightness);
    Vector3 AdjustSaturation(const Vector3& rgb, float saturation);
    Vector3 AdjustHue(const Vector3& rgb, float hueShift);
    Vector3 AdjustContrast(const Vector3& rgb, float contrast);
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