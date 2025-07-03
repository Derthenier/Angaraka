module;

#include <Angaraka/Base.hpp>

export module Angaraka.Math.Color;

import Angaraka.Math.Vector3;
import Angaraka.Math.Vector4;

namespace Angaraka::Math {

    // ==================================================================================
    // Color
    // ==================================================================================
    export struct Color {
        F32 R{ 0.0f };  // Red component (0.0 to 1.0)
        F32 G{ 0.0f };  // Green component (0.0 to 1.0)
        F32 B{ 0.0f };  // Blue component (0.0 to 1.0)
        F32 A{ 1.0f };  // Alpha component (0.0 to 1.0, 1.0 is fully opaque)

        inline explicit Color(F32 r = 0.0f, F32 g = 0.0f, F32 b = 0.0f, F32 a = 1.0f) : R(r), G(g), B(b), A(a) { }
        inline explicit Color(const Vector3& rgb, F32 a = 1.0f) : R(rgb.x), G(rgb.y), B(rgb.z), A(a) { }
        inline explicit Color(const Vector4& rgba) : R(rgba.x), G(rgba.y), B(rgba.z), A(rgba.w) { }
        ~Color() = default;

        inline Vector3 ToVector3() const { return Vector3(R, G, B); }
        inline Vector4 ToVector4() const { return Vector4(R, G, B, A); }
        inline Color operator*(const F32 intensity) const { return Color(R * intensity, G * intensity, B * intensity, A); }


        // Color space conversions
        static Color HSVToRGB(F32 h, F32 s, F32 v);
        static Color HSVToRGB(const Color& hsv);
        static Color RGBToHSV(F32 r, F32 g, F32 b);
        static Color RGBToHSV(const Color& rgb);
        static Color HSLToRGB(F32 h, F32 s, F32 l);
        static Color HSLToRGB(const Color& hsl);
        static Color RGBToHSL(F32 r, F32 g, F32 b);
        static Color RGBToHSL(const Color& rgb);
        static Color RGBFromBytes(U8 r, U8 g, U8 b);
        static Color RGBToBytes(const Color& rgb);
        static U32 PackRGBA(const Color& rgb, F32 alpha = 1.0f);
        static Color UnpackRGBA(U32 packedColor);
        static Color LinearToGamma(const Color& linearColor, F32 gamma = 2.2f);
        static Color GammaToLinear(const Color& gammaColor, F32 gamma = 2.2f);
        static Color Lerp(const Color& colorA, const Color& colorB, F32 t);
        static Color LerpHSV(const Color& rgbA, const Color& rgbB, F32 t);
        static Color AdjustBrightness(const Color& rgb, F32 brightness);
        static Color AdjustSaturation(const Color& rgb, F32 saturation);
        static Color AdjustHue(const Color& rgb, F32 hueShift);
        static Color AdjustContrast(const Color& rgb, F32 contrast);
        static Color Multiply(const Color& base, const Color& blend);
        static Color Screen(const Color& base, const Color& blend);
        static Color Overlay(const Color& base, const Color& blend);
        static Color SoftLight(const Color& base, const Color& blend);
    };

    export namespace Colors {
        const Color Black   = Color(0.00f, 0.00f, 0.00f, 1.00f);
        const Color White   = Color(1.00f, 1.00f, 1.00f, 1.00f);
        const Color Red     = Color(1.00f, 0.00f, 0.00f, 1.00f);
        const Color Green   = Color(0.00f, 1.00f, 0.00f, 1.00f);
        const Color Blue    = Color(0.00f, 0.00f, 1.00f, 1.00f);
        const Color Yellow  = Color(1.00f, 1.00f, 0.00f, 1.00f);
        const Color Cyan    = Color(0.00f, 1.00f, 1.00f, 1.00f);
        const Color Magenta = Color(1.00f, 0.00f, 1.00f, 1.00f);
        const Color Gray    = Color(0.50f, 0.50f, 0.50f, 1.00f);
        const Color Orange  = Color(1.00f, 0.50f, 0.00f, 1.00f);
        const Color Purple  = Color(0.50f, 0.00f, 1.00f, 1.00f);
        const Color Pink    = Color(1.00f, 0.75f, 0.80f, 1.00f);
        const Color Brown   = Color(0.60f, 0.30f, 0.20f, 1.00f);
    }
} // namespace Angaraka::Math