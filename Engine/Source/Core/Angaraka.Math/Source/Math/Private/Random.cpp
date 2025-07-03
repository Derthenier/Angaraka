// ==================================================================================
// AngarakaMath/Public/Math/Random.cpp
// ==================================================================================
module;

#include <Angaraka/Base.hpp>
#include <cstdint>
#include <random>

module Angaraka.Math.Random;

import Angaraka.Math;
import Angaraka.Math.Vector2;
import Angaraka.Math.Vector3;
import Angaraka.Math.Vector4;
import Angaraka.Math.Color;

namespace Angaraka::Math
{
    // ==================================================================================
    // Random Implementation
    // ==================================================================================

    static std::random_device s_randomDevice;
    static std::mt19937 s_generator(s_randomDevice());

    void Random::SetSeed(U32 seed)
    {
        s_generator.seed(seed);
    }

    F32 Random::Value()
    {
        std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
        return distribution(s_generator);
    }

    F32 Random::Range(F32 min, F32 max)
    {
        std::uniform_real_distribution<float> distribution(min, max);
        return distribution(s_generator);
    }

    int Random::Range(int min, int max)
    {
        std::uniform_int_distribution<int> distribution(min, max);
        return distribution(s_generator);
    }

    U32 Random::UInt32()
    {
        return s_generator();
    }

    bool Random::Bool(F32 probability)
    {
        return Value() < probability;
    }

    F32 Random::Gaussian(F32 mean, F32 stddev)
    {
        std::normal_distribution<float> distribution(mean, stddev);
        return distribution(s_generator);
    }

    Vector2 Random::InsideUnitCircle()
    {
        F32 angle = Range(0.0f, Constants::TwoPiF);
        F32 radius = Util::Sqrt(Value()); // Square root for uniform distribution
        return { radius * std::cos(angle), radius * std::sin(angle) };
    }

    Vector2 Random::OnUnitCircle()
    {
        F32 angle = Range(0.0f, Constants::TwoPiF);
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
        F32 theta = Range(0.0f, Constants::TwoPiF);
        F32 phi = std::acos(Range(-1.0f, 1.0f));

        F32 sinPhi = std::sin(phi);
        return {
            sinPhi * std::cos(theta),
            sinPhi * std::sin(theta),
            std::cos(phi)
        };
    }

    Vector3 Random::InCone(const Vector3& direction, F32 angle)
    {
        Vector3 normalizedDir = direction.Normalized();
        F32 cosAngle = std::cos(angle);

        // Generate random point on unit sphere
        Vector3 randomSphere = OnUnitSphere();

        // If the random point is within the cone, use it
        if (randomSphere.Dot(normalizedDir) >= cosAngle)
            return randomSphere;

        // Otherwise, reflect it into the cone
        Vector3 reflected = randomSphere - 2.0f * (randomSphere.Dot(normalizedDir) - cosAngle) * normalizedDir;
        return reflected.Normalized();
    }

    Vector3 Random::ColorHSV(F32 hueMin, F32 hueMax, F32 satMin, F32 satMax, F32 valMin, F32 valMax)
    {
        F32 h = Range(hueMin, hueMax);
        F32 s = Range(satMin, satMax);
        F32 v = Range(valMin, valMax);

        return Color::HSVToRGB(h, s, v).ToVector3();
    }
}