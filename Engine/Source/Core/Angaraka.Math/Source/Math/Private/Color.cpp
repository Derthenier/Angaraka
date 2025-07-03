module Angaraka.Math.Color;

import Angaraka.Math;

namespace Angaraka::Math {

    // ==================================================================================
    // HSV to RGB Conversion
    // ==================================================================================

    Color Color::HSVToRGB(F32 h, F32 s, F32 v)
    {
        // Clamp input values to valid ranges
        h = Util::Mod(h, 360.0f); // Hue wraps around at 360
        if (h < 0.0f) h += 360.0f;
        s = Util::Clamp01(s);     // Saturation [0, 1]
        v = Util::Clamp01(v);     // Value [0, 1]

        F32 c = v * s;    // Chroma
        F32 x = c * (1.0f - Util::Abs(Util::Mod(h / 60.0f, 2.0f) - 1.0f));
        F32 m = v - c;    // Match value

        F32 r, g, b;

        if (h >= 0.0f && h < 60.0f)
        {
            r = c; g = x; b = 0.0f;
        }
        else if (h >= 60.0f && h < 120.0f)
        {
            r = x; g = c; b = 0.0f;
        }
        else if (h >= 120.0f && h < 180.0f)
        {
            r = 0.0f; g = c; b = x;
        }
        else if (h >= 180.0f && h < 240.0f)
        {
            r = 0.0f; g = x; b = c;
        }
        else if (h >= 240.0f && h < 300.0f)
        {
            r = x; g = 0.0f; b = c;
        }
        else // h >= 300.0f && h < 360.0f
        {
            r = c; g = 0.0f; b = x;
        }

        return Color(r + m, g + m, b + m);
    }

    Color Color::HSVToRGB(const Color& hsv)
    {
        return HSVToRGB(hsv.R, hsv.G, hsv.B);
    }

    // ==================================================================================
    // RGB to HSV Conversion
    // ==================================================================================

    Color Color::RGBToHSV(F32 r, F32 g, F32 b)
    {
        // Clamp input values to valid range [0, 1]
        r = Util::Clamp01(r);
        g = Util::Clamp01(g);
        b = Util::Clamp01(b);

        F32 maxVal = Util::Max(r, Util::Max(g, b));
        F32 minVal = Util::Min(r, Util::Min(g, b));
        F32 delta = maxVal - minVal;

        F32 h = 0.0f; // Hue
        F32 s = 0.0f; // Saturation
        F32 v = maxVal; // Value

        // Calculate saturation
        if (maxVal > Constants::EpsilonF)
        {
            s = delta / maxVal;
        }

        // Calculate hue
        if (delta > Constants::EpsilonF)
        {
            if (Util::IsNearlyEqual(maxVal, r))
            {
                h = 60.0f * (Util::Mod((g - b) / delta, 6.0f));
            }
            else if (Util::IsNearlyEqual(maxVal, g))
            {
                h = 60.0f * (((b - r) / delta) + 2.0f);
            }
            else // maxVal == b
            {
                h = 60.0f * (((r - g) / delta) + 4.0f);
            }
        }

        // Ensure hue is positive
        if (h < 0.0f)
        {
            h += 360.0f;
        }

        return Color(h, s, v);
    }

    Color Color::RGBToHSV(const Color& rgb)
    {
        return RGBToHSV(rgb.R, rgb.G, rgb.B);
    }

    // ==================================================================================
    // Additional Color Space Conversions
    // ==================================================================================

    // HSL (Hue, Saturation, Lightness) to RGB
    Color Color::HSLToRGB(F32 h, F32 s, F32 l)
    {
        h = Util::Mod(h, 360.0f);
        if (h < 0.0f) h += 360.0f;
        s = Util::Clamp01(s);
        l = Util::Clamp01(l);

        F32 c = (1.0f - Util::Abs(2.0f * l - 1.0f)) * s; // Chroma
        F32 x = c * (1.0f - Util::Abs(Util::Mod(h / 60.0f, 2.0f) - 1.0f));
        F32 m = l - c / 2.0f;

        F32 r, g, b;

        if (h >= 0.0f && h < 60.0f)
        {
            r = c; g = x; b = 0.0f;
        }
        else if (h >= 60.0f && h < 120.0f)
        {
            r = x; g = c; b = 0.0f;
        }
        else if (h >= 120.0f && h < 180.0f)
        {
            r = 0.0f; g = c; b = x;
        }
        else if (h >= 180.0f && h < 240.0f)
        {
            r = 0.0f; g = x; b = c;
        }
        else if (h >= 240.0f && h < 300.0f)
        {
            r = x; g = 0.0f; b = c;
        }
        else // h >= 300.0f && h < 360.0f
        {
            r = c; g = 0.0f; b = x;
        }

        return Color(r + m, g + m, b + m);
    }

    Color Color::HSLToRGB(const Color& hsl)
    {
        return HSLToRGB(hsl.R, hsl.G, hsl.B);
    }

    // RGB to HSL conversion
    Color Color::RGBToHSL(F32 r, F32 g, F32 b)
    {
        r = Util::Clamp01(r);
        g = Util::Clamp01(g);
        b = Util::Clamp01(b);

        F32 maxVal = Util::Max(r, Util::Max(g, b));
        F32 minVal = Util::Min(r, Util::Min(g, b));
        F32 delta = maxVal - minVal;

        F32 h = 0.0f; // Hue
        F32 s = 0.0f; // Saturation
        F32 l = (maxVal + minVal) / 2.0f; // Lightness

        // Calculate saturation
        if (delta > Constants::EpsilonF)
        {
            s = delta / (1.0f - Util::Abs(2.0f * l - 1.0f));
        }

        // Calculate hue (same as HSV)
        if (delta > Constants::EpsilonF)
        {
            if (Util::IsNearlyEqual(maxVal, r))
            {
                h = 60.0f * (Util::Mod((g - b) / delta, 6.0f));
            }
            else if (Util::IsNearlyEqual(maxVal, g))
            {
                h = 60.0f * (((b - r) / delta) + 2.0f);
            }
            else // maxVal == b
            {
                h = 60.0f * (((r - g) / delta) + 4.0f);
            }
        }

        if (h < 0.0f)
        {
            h += 360.0f;
        }

        return Color(h, s, l);
    }

    Color Color::RGBToHSL(const Color& rgb)
    {
        return RGBToHSL(rgb.R, rgb.G, rgb.B);
    }

    // ==================================================================================
    // Color Utility Functions
    // ==================================================================================

    // Convert RGB from [0-255] to [0-1] range
    Color Color::RGBFromBytes(U8 r, U8 g, U8 b)
    {
        return Color(r / 255.0f, g / 255.0f, b / 255.0f);
    }

    // Convert RGB from [0-1] to [0-255] range
    Color Color::RGBToBytes(const Color& rgb)
    {
        return Color(
            Util::Round(Util::Clamp01(rgb.R) * 255.0f),
            Util::Round(Util::Clamp01(rgb.G) * 255.0f),
            Util::Round(Util::Clamp01(rgb.B) * 255.0f)
        );
    }

    // Pack RGB to 32-bit integer (ARGB format)
    U32 Color::PackRGBA(const Color& rgb, F32 alpha)
    {
        U8 r = static_cast<U8>(Util::Round(Util::Clamp01(rgb.R) * 255.0f));
        U8 g = static_cast<U8>(Util::Round(Util::Clamp01(rgb.G) * 255.0f));
        U8 b = static_cast<U8>(Util::Round(Util::Clamp01(rgb.B) * 255.0f));
        U8 a = static_cast<U8>(Util::Round(Util::Clamp01(alpha) * 255.0f));

        return (static_cast<U32>(a) << 24) |
            (static_cast<U32>(r) << 16) |
            (static_cast<U32>(g) << 8) |
            static_cast<U32>(b);
    }

    // Unpack 32-bit integer to RGB + alpha
    Color Color::UnpackRGBA(U32 packedColor)
    {
        F32 a = ((packedColor >> 24) & 0xFF) / 255.0f;
        F32 r = ((packedColor >> 16) & 0xFF) / 255.0f;
        F32 g = ((packedColor >> 8) & 0xFF) / 255.0f;
        F32 b = (packedColor & 0xFF) / 255.0f;

        return Color(r, g, b, a);
    }

    // Gamma correction
    Color Color::LinearToGamma(const Color& linearColor, F32 gamma)
    {
        return Color(
            Util::Pow(linearColor.R, 1.0f / gamma),
            Util::Pow(linearColor.G, 1.0f / gamma),
            Util::Pow(linearColor.B, 1.0f / gamma)
        );
    }

    Color Color::GammaToLinear(const Color& gammaColor, F32 gamma)
    {
        return Color(
            Util::Pow(gammaColor.R, gamma),
            Util::Pow(gammaColor.G, gamma),
            Util::Pow(gammaColor.B, gamma)
        );
    }

    // Color interpolation
    Color Color::Lerp(const Color& colorA, const Color& colorB, F32 t)
    {
        return Color(colorA.ToVector3().Lerp(colorB.ToVector3(), t));
    }

    Color Color::LerpHSV(const Color& rgbA, const Color& rgbB, F32 t)
    {
        Color hsvA = RGBToHSV(rgbA);
        Color hsvB = RGBToHSV(rgbB);

        // Handle hue interpolation (shortest path around color wheel)
        F32 hue1 = hsvA.R;
        F32 hue2 = hsvB.R;
        F32 hueDiff = hue2 - hue1;

        if (hueDiff > 180.0f)
        {
            hue2 -= 360.0f;
        }
        else if (hueDiff < -180.0f)
        {
            hue2 += 360.0f;
        }

        Color hsvResult = Color(
            Util::Lerp(hue1, hue2, t),
            Util::Lerp(hsvA.G, hsvB.G, t),
            Util::Lerp(hsvA.B, hsvB.B, t)
        );

        return HSVToRGB(hsvResult);
    }

    // Color manipulation functions
    Color Color::AdjustBrightness(const Color& rgb, F32 brightness)
    {
        Color hsv = RGBToHSV(rgb);
        hsv.B = Util::Clamp01(hsv.B * brightness);
        return HSVToRGB(hsv);
    }

    Color Color::AdjustSaturation(const Color& rgb, F32 saturation)
    {
        Color hsv = RGBToHSV(rgb);
        hsv.G = Util::Clamp01(hsv.G * saturation);
        return HSVToRGB(hsv);
    }

    Color Color::AdjustHue(const Color& rgb, F32 hueShift)
    {
        Color hsv = RGBToHSV(rgb);
        hsv.R = Util::Mod(hsv.R + hueShift, 360.0f);
        return HSVToRGB(hsv);
    }

    Color Color::AdjustContrast(const Color& rgb, F32 contrast)
    {
        return Color(
            Util::Clamp01((rgb.R - 0.5f) * contrast + 0.5f),
            Util::Clamp01((rgb.G - 0.5f) * contrast + 0.5f),
            Util::Clamp01((rgb.B - 0.5f) * contrast + 0.5f)
        );
    }

    // Color blending modes
    Color Color::Multiply(const Color& base, const Color& blend)
    {
        return Color(base.ToVector3() * blend.ToVector3());
    }

    Color Color::Screen(const Color& base, const Color& blend)
    {
        return Color(Vector3::One - (Vector3::One - base.ToVector3()) * (Vector3::One - blend.ToVector3()));
    }

    Color Color::Overlay(const Color& base, const Color& blend)
    {
        return Color(
            base.R < 0.5f ? 2.0f * base.R * blend.R : 1.0f - 2.0f * (1.0f - base.R) * (1.0f - blend.R),
            base.G < 0.5f ? 2.0f * base.G * blend.G : 1.0f - 2.0f * (1.0f - base.G) * (1.0f - blend.G),
            base.B < 0.5f ? 2.0f * base.B * blend.B : 1.0f - 2.0f * (1.0f - base.B) * (1.0f - blend.B)
        );
    }

    Color Color::SoftLight(const Color& base, const Color& blend)
    {
        return Color(
            blend.R < 0.5f ? 2.0f * base.R * blend.R + base.R * base.R * (1.0f - 2.0f * blend.R) :
            Util::Sqrt(base.R) * (2.0f * blend.R - 1.0f) + 2.0f * base.R * (1.0f - blend.R),
            blend.G < 0.5f ? 2.0f * base.G * blend.G + base.G * base.G * (1.0f - 2.0f * blend.G) :
            Util::Sqrt(base.G) * (2.0f * blend.G - 1.0f) + 2.0f * base.G * (1.0f - blend.G),
            blend.B < 0.5f ? 2.0f * base.B * blend.B + base.B * base.B * (1.0f - 2.0f * blend.B) :
            Util::Sqrt(base.B) * (2.0f * blend.B - 1.0f) + 2.0f * base.B * (1.0f - blend.B)
        );
    }
}