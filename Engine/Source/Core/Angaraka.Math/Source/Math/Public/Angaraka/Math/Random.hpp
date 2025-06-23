// ==================================================================================
// AngarakaMath/Public/Math/Random.hpp - RNG
// ==================================================================================

#pragma once

#include "Vector2.hpp"
#include "Vector3.hpp"
#include "Vector4.hpp"
#include <cstdint>

namespace Angaraka::Math
{
    // ==================================================================================
    // Random Number Generation
    // ==================================================================================
    class Random
    {
    public:
        static void SetSeed(uint32_t seed);

        // Float random values
        static float Value();                                    // [0, 1]
        static float Range(float min, float max);               // [min, max]
        static float Gaussian(float mean = 0.0f, float stddev = 1.0f); // Gaussian distribution

        // Integer random values
        static int Range(int min, int max);                     // [min, max] inclusive
        static uint32_t UInt32();                               // Full range uint32

        // Boolean
        static bool Bool(float probability = 0.5f);             // True with given probability

        // Vector random values
        static Vector2 InsideUnitCircle();
        static Vector2 OnUnitCircle();
        static Vector3 InsideUnitSphere();
        static Vector3 OnUnitSphere();
        static Vector3 InCone(const Vector3& direction, float angle);

        // Color
        static Vector3 ColorHSV(float hueMin = 0.0f, float hueMax = 1.0f,
            float satMin = 0.0f, float satMax = 1.0f,
            float valMin = 0.0f, float valMax = 1.0f);
    };
}