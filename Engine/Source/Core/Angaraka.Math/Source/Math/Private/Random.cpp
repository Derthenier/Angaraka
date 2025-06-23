// ==================================================================================
// AngarakaMath/Public/Math/Random.cpp
// ==================================================================================

#pragma once

#include "Angaraka/MathCore.hpp"
#include "Angaraka/Math/Random.hpp"
#include <random>

namespace Angaraka::Math
{
    // ==================================================================================
    // Random Implementation
    // ==================================================================================

    static std::random_device s_randomDevice;
    static std::mt19937 s_generator(s_randomDevice());

    void Random::SetSeed(uint32_t seed)
    {
        s_generator.seed(seed);
    }

    float Random::Value()
    {
        std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
        return distribution(s_generator);
    }

    float Random::Range(float min, float max)
    {
        std::uniform_real_distribution<float> distribution(min, max);
        return distribution(s_generator);
    }

    int Random::Range(int min, int max)
    {
        std::uniform_int_distribution<int> distribution(min, max);
        return distribution(s_generator);
    }

    uint32_t Random::UInt32()
    {
        return s_generator();
    }

    bool Random::Bool(float probability)
    {
        return Value() < probability;
    }

    float Random::Gaussian(float mean, float stddev)
    {
        std::normal_distribution<float> distribution(mean, stddev);
        return distribution(s_generator);
    }

    Vector2 Random::InsideUnitCircle()
    {
        float angle = Range(0.0f, TwoPiF);
        float radius = Sqrt(Value()); // Square root for uniform distribution
        return { radius * std::cos(angle), radius * std::sin(angle) };
    }

    Vector2 Random::OnUnitCircle()
    {
        float angle = Range(0.0f, TwoPiF);
        return { std::cos(angle), std::sin(angle) };
    }

    Vector3 Random::InsideUnitSphere()
    {
        Vector3 point;
        do
        {
            point = { Range(-1.0f, 1.0f), Range(-1.0f, 1.0f), Range(-1.0f, 1.0f) };
        } while (point.LengthSquared() > 1.0f);

        return point;
    }

    Vector3 Random::OnUnitSphere()
    {
        float theta = Range(0.0f, TwoPiF);
        float phi = std::acos(Range(-1.0f, 1.0f));

        float sinPhi = std::sin(phi);
        return {
            sinPhi * std::cos(theta),
            sinPhi * std::sin(theta),
            std::cos(phi)
        };
    }

    Vector3 Random::InCone(const Vector3& direction, float angle)
    {
        Vector3 normalizedDir = direction.Normalized();
        float cosAngle = std::cos(angle);

        // Generate random point on unit sphere
        Vector3 randomSphere = OnUnitSphere();

        // If the random point is within the cone, use it
        if (randomSphere.Dot(normalizedDir) >= cosAngle)
            return randomSphere;

        // Otherwise, reflect it into the cone
        Vector3 reflected = randomSphere - 2.0f * (randomSphere.Dot(normalizedDir) - cosAngle) * normalizedDir;
        return reflected.Normalized();
    }

    Vector3 Random::ColorHSV(float hueMin, float hueMax, float satMin, float satMax, float valMin, float valMax)
    {
        float h = Range(hueMin, hueMax);
        float s = Range(satMin, satMax);
        float v = Range(valMin, valMax);

        return HSVToRGB(h, s, v);
    }
}