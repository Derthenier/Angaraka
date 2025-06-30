#pragma once

#include "Angaraka/Math/Vector3.hpp"
#include "Angaraka/Math/Vector4.hpp"

namespace Angaraka::Math {

    class Color {
    public:
        explicit Color(F32 r = 0.0f, F32 g = 0.0f, F32 b = 0.0f, F32 a = 1.0f)
            : red(r), green(g), blue(b), alpha(a) { }
        explicit Color(const Vector3& rgb, F32 a = 1.0f)
            : red(rgb.x), green(rgb.y), blue(rgb.z), alpha(a) { }
        explicit Color(const Vector4& rgba)
            : red(rgba.x), green(rgba.y), blue(rgba.z), alpha(rgba.w) { }
        ~Color() = default;

        Vector3 ToVector3() const {
            return Vector3(red, green, blue);
        }

        Vector4 ToVector4() const {
            return Vector4(red, green, blue, alpha);
        }

        Color operator*(const F32 intensity) const {
            return Color(red * intensity, green * intensity, blue * intensity, alpha);
        }

        // Component-wise retrieval
        F32 GetRed() const { return red; }
        F32 GetGreen() const { return green; }
        F32 GetBlue() const { return blue; }
        F32 GetAlpha() const { return alpha; }
        F32 Red() const { return red; }
        F32 Green() const { return green; }
        F32 Blue() const { return blue; }
        F32 Alpha() const { return alpha; }
        F32 R() const { return red; }
        F32 G() const { return green; }
        F32 B() const { return blue; }
        F32 A() const { return alpha; }

    private:
        F32 red{ 0.0f };
        F32 green{ 0.0f };
        F32 blue{ 0.0f };
        F32 alpha{ 1.0f };
    };

    namespace Colors {
        const Color Black = Color(0.0f, 0.0f, 0.0f, 1.0f);
        const Color White = Color(1.0f, 1.0f, 1.0f, 1.0f);
        const Color Red = Color(1.0f, 0.0f, 0.0f, 1.0f);
        const Color Green = Color(0.0f, 1.0f, 0.0f, 1.0f);
        const Color Blue = Color(0.0f, 0.0f, 1.0f, 1.0f);
        const Color Yellow = Color(1.0f, 1.0f, 0.0f, 1.0f);
        const Color Cyan = Color(0.0f, 1.0f, 1.0f, 1.0f);
        const Color Magenta = Color(1.0f, 0.0f, 1.0f, 1.0f);
        const Color Gray = Color(0.5f, 0.5f, 0.5f, 1.0f);
        const Color Orange = Color(1.0f, 0.5f, 0.0f, 1.0f);
        const Color Purple = Color(0.5f, 0.0f, 1.0f, 1.0f);
        const Color Pink = Color(1.0f, 0.75f, 0.8f, 1.0f);
        const Color Brown = Color(0.6f, 0.3f, 0.2f, 1.0f);
    }
}