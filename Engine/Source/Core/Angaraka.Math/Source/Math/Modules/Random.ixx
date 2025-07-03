module;

#include <Angaraka/Base.hpp>
#include <cstdint>

export module Angaraka.Math.Random;

import Angaraka.Math.Vector2;
import Angaraka.Math.Vector3;
import Angaraka.Math.Vector4;

namespace Angaraka::Math {
    // ==================================================================================
    // Random Number Generation
    // ==================================================================================
    export struct Random {
        static void SetSeed(U32 seed);

        // Float random values
        static F32 Value();                                     // [0, 1]
        static F32 Range(F32 min, F32 max);                     // [min, max]
        static F32 Gaussian(F32 mean = 0.0f, F32 stddev = 1.0f);// Gaussian distribution

        // Integer random values
        static int Range(int min, int max);                     // [min, max] inclusive
        static U32 UInt32();                                    // Full range uint32

        // Boolean
        static bool Bool(F32 probability = 0.5f);               // True with given probability

        // Vector random values
        static Vector2 InsideUnitCircle();
        static Vector2 OnUnitCircle();
        static Vector3 InsideUnitSphere();
        static Vector3 OnUnitSphere();
        static Vector3 InCone(const Vector3& direction, F32 angle);

        // Color
        static Vector3 ColorHSV(F32 hueMin = 0.0f, F32 hueMax = 1.0f,
            F32 satMin = 0.0f, F32 satMax = 1.0f,
            F32 valMin = 0.0f, F32 valMax = 1.0f);
    };
} // namespace Angaraka::Math