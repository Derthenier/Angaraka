// ==================================================================================
// AngarakaMath/Private/AngarakaMath.cpp - Implementation
// ==================================================================================

#include "Angaraka/MathCore.hpp"
#include <cstring>
#include <cmath>

namespace Angaraka::Math
{
    // ==================================================================================
    // Utility Functions Implementation
    // ==================================================================================

    F32 SinDegrees(F32 degrees)
    {
        return std::sin(degrees * DegToRadF);
    }

    F32 CosDegrees(F32 degrees)
    {
        return std::cos(degrees * DegToRadF);
    }

    F32 TanDegrees(F32 degrees)
    {
        return std::tan(degrees * DegToRadF);
    }

    F32 Sqrt(F32 value)
    {
        return std::sqrt(value);
    }

    F32 InverseSqrt(F32 value)
    {
        // Fast inverse square root approximation
        if (IsNearlyZero(value))
            return 0.0f;
        return 1.0f / std::sqrt(value);
    }

    F32 Pow(F32 base, F32 exponent)
    {
        return std::pow(base, exponent);
    }

    F32 Log(F32 value)
    {
        return std::log(value);
    }

    F32 Log2(F32 value)
    {
        return std::log2(value);
    }

    F32 Log10(F32 value)
    {
        return std::log10(value);
    }

    F32 Exp(F32 value)
    {
        return std::exp(value);
    }

    F32 Floor(F32 value)
    {
        return std::floor(value);
    }

    F32 Ceil(F32 value)
    {
        return std::ceil(value);
    }

    F32 Round(F32 value)
    {
        return std::round(value);
    }

    F32 Frac(F32 value)
    {
        return value - Floor(value);
    }

    F32 Mod(F32 x, F32 y)
    {
        return std::fmod(x, y);
    }

    F32 NormalizeAngle(F32 angle)
    {
        while (angle > 180.0f) angle -= 360.0f;
        while (angle < -180.0f) angle += 360.0f;
        return angle;
    }

    F32 NormalizeAngle360(F32 angle)
    {
        while (angle >= 360.0f) angle -= 360.0f;
        while (angle < 0.0f) angle += 360.0f;
        return angle;
    }

    F32 NormalizeAngleRadians(F32 angle)
    {
        while (angle > PiF) angle -= TwoPiF;
        while (angle < -PiF) angle += TwoPiF;
        return angle;
    }

    F32 AngleDifference(F32 a, F32 b)
    {
        F32 diff = NormalizeAngle(b - a);
        return diff;
    }

    F32 RadiansToDegress(F32 radians)
    {
        return radians * RadToDegF;
    }

    F32 DegreesToRadians(F32 degrees)
    {
        return degrees * DegToRadF;
    }

    bool IsNearlyEqual(F32 a, F32 b, F32 epsilon)
    {
        return Abs(a - b) <= epsilon;
    }

    bool IsNearlyZero(F32 value, F32 epsilon)
    {
        return Abs(value) <= epsilon;
    }

    bool IsNearlyEqual(const Vector2& a, const Vector2& b, F32 epsilon)
    {
        return IsNearlyEqual(a.x, b.x, epsilon) && IsNearlyEqual(a.y, b.y, epsilon);
    }

    bool IsNearlyEqual(const Vector3& a, const Vector3& b, F32 epsilon)
    {
        return IsNearlyEqual(a.x, b.x, epsilon) && IsNearlyEqual(a.y, b.y, epsilon) && IsNearlyEqual(a.z, b.z, epsilon);
    }

    bool IsNearlyEqual(const Vector4& a, const Vector4& b, F32 epsilon)
    {
        return IsNearlyEqual(a.x, b.x, epsilon) && IsNearlyEqual(a.y, b.y, epsilon) &&
            IsNearlyEqual(a.z, b.z, epsilon) && IsNearlyEqual(a.w, b.w, epsilon);
    }

    F32 SmoothStep(F32 edge0, F32 edge1, F32 x)
    {
        F32 t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    F32 SmootherStep(F32 edge0, F32 edge1, F32 x)
    {
        F32 t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    // Easing functions
    F32 EaseInQuad(F32 t) { return t * t; }
    F32 EaseOutQuad(F32 t) { return 1.0f - (1.0f - t) * (1.0f - t); }
    F32 EaseInOutQuad(F32 t) { return t < 0.5f ? 2.0f * t * t : 1.0f - Pow(-2.0f * t + 2.0f, 2.0f) / 2.0f; }
    F32 EaseInCubic(F32 t) { return t * t * t; }
    F32 EaseOutCubic(F32 t) { return 1.0f - Pow(1.0f - t, 3.0f); }
    F32 EaseInOutCubic(F32 t) { return t < 0.5f ? 4.0f * t * t * t : 1.0f - Pow(-2.0f * t + 2.0f, 3.0f) / 2.0f; }


    // ==================================================================================
    // Distance Functions
    // ==================================================================================

    F32 PointToLineDistance(const Vector3& point, const Vector3& lineStart, const Vector3& lineEnd)
    {
        Vector3 lineDir = lineEnd - lineStart;
        Vector3 pointToStart = point - lineStart;

        // Handle degenerate case (line has zero length)
        F32 lineLengthSq = lineDir.LengthSquared();
        if (IsNearlyZero(lineLengthSq))
        {
            return pointToStart.Length();
        }

        // Project point onto line and find closest point
        F32 t = pointToStart.Dot(lineDir) / lineLengthSq;
        t = Clamp(t, 0.0f, 1.0f); // Clamp to line segment

        Vector3 closestPoint = lineStart + lineDir * t;
        return (point - closestPoint).Length();
    }

    F32 PointToPlaneDistance(const Vector3& point, const Vector3& planePoint, const Vector3& planeNormal)
    {
        Vector3 normalizedNormal = planeNormal.Normalized();
        return (point - planePoint).Dot(normalizedNormal);
    }

    F32 PointToPlaneDistanceAbs(const Vector3& point, const Vector3& planePoint, const Vector3& planeNormal)
    {
        return Abs(PointToPlaneDistance(point, planePoint, planeNormal));
    }

    Vector3 ClosestPointOnLine(const Vector3& point, const Vector3& lineStart, const Vector3& lineEnd)
    {
        Vector3 lineDir = lineEnd - lineStart;
        Vector3 pointToStart = point - lineStart;

        F32 lineLengthSq = lineDir.LengthSquared();
        if (IsNearlyZero(lineLengthSq))
        {
            return lineStart; // Degenerate line
        }

        F32 t = pointToStart.Dot(lineDir) / lineLengthSq;
        t = Clamp(t, 0.0f, 1.0f);

        return lineStart + lineDir * t;
    }

    Vector3 ClosestPointOnPlane(const Vector3& point, const Vector3& planePoint, const Vector3& planeNormal)
    {
        Vector3 normalizedNormal = planeNormal.Normalized();
        F32 distance = PointToPlaneDistance(point, planePoint, normalizedNormal);
        return point - normalizedNormal * distance;
    }

    // ==================================================================================
    // Intersection Tests
    // ==================================================================================

    bool RayIntersectsSphere(const Vector3& rayOrigin, const Vector3& rayDirection,
        const Vector3& sphereCenter, F32 sphereRadius,
        float& t1, float& t2)
    {
        Vector3 normalizedDir = rayDirection.Normalized();
        Vector3 oc = rayOrigin - sphereCenter;

        F32 a = normalizedDir.Dot(normalizedDir); // Should be 1.0 for normalized direction
        F32 b = 2.0f * oc.Dot(normalizedDir);
        F32 c = oc.Dot(oc) - sphereRadius * sphereRadius;

        F32 discriminant = b * b - 4.0f * a * c;

        if (discriminant < 0.0f)
        {
            return false; // No intersection
        }

        F32 sqrtDiscriminant = Sqrt(discriminant);
        t1 = (-b - sqrtDiscriminant) / (2.0f * a);
        t2 = (-b + sqrtDiscriminant) / (2.0f * a);

        return true;
    }

    bool RayIntersectsPlane(const Vector3& rayOrigin, const Vector3& rayDirection,
        const Vector3& planePoint, const Vector3& planeNormal,
        float& t)
    {
        Vector3 normalizedNormal = planeNormal.Normalized();
        Vector3 normalizedDir = rayDirection.Normalized();

        F32 denominator = normalizedDir.Dot(normalizedNormal);

        // Check if ray is parallel to plane
        if (IsNearlyZero(denominator))
        {
            return false;
        }

        Vector3 planeToRay = planePoint - rayOrigin;
        t = planeToRay.Dot(normalizedNormal) / denominator;

        return t >= 0.0f; // Only forward intersections
    }

    bool RayIntersectsTriangle(const Vector3& rayOrigin, const Vector3& rayDirection,
        const Vector3& v0, const Vector3& v1, const Vector3& v2,
        float& t, Vector3& barycentrics)
    {
        // Moller-Trumbore intersection algorithm
        Vector3 edge1 = v1 - v0;
        Vector3 edge2 = v2 - v0;
        Vector3 h = rayDirection.Cross(edge2);
        F32 a = edge1.Dot(h);

        // Check if ray is parallel to triangle
        if (IsNearlyZero(a))
        {
            return false;
        }

        F32 f = 1.0f / a;
        Vector3 s = rayOrigin - v0;
        F32 u = f * s.Dot(h);

        if (u < 0.0f || u > 1.0f)
        {
            return false;
        }

        Vector3 q = s.Cross(edge1);
        F32 v = f * rayDirection.Dot(q);

        if (v < 0.0f || u + v > 1.0f)
        {
            return false;
        }

        t = f * edge2.Dot(q);

        if (t > EpsilonF) // Ray intersection
        {
            barycentrics = Vector3(1.0f - u - v, u, v);
            return true;
        }

        return false; // Line intersection but not ray intersection
    }

    bool LineSegmentIntersect2D(const Vector2& a1, const Vector2& a2,
        const Vector2& b1, const Vector2& b2,
        Vector2& intersection)
    {
        Vector2 s1 = a2 - a1;
        Vector2 s2 = b2 - b1;

        F32 denominator = (-s2.x * s1.y + s1.x * s2.y);

        // Check if lines are parallel
        if (IsNearlyZero(denominator))
        {
            return false;
        }

        F32 s = (-s1.y * (a1.x - b1.x) + s1.x * (a1.y - b1.y)) / denominator;
        F32 t = (s2.x * (a1.y - b1.y) - s2.y * (a1.x - b1.x)) / denominator;

        if (s >= 0.0f && s <= 1.0f && t >= 0.0f && t <= 1.0f)
        {
            // Collision detected
            intersection = a1 + t * s1;
            return true;
        }

        return false; // No collision
    }

    bool SphereSphereIntersect(const Vector3& center1, F32 radius1,
        const Vector3& center2, F32 radius2)
    {
        F32 distanceSquared = (center1 - center2).LengthSquared();
        F32 radiusSum = radius1 + radius2;
        return distanceSquared <= (radiusSum * radiusSum);
    }

    bool SphereAABBIntersect(const Vector3& sphereCenter, F32 sphereRadius,
        const Vector3& aabbMin, const Vector3& aabbMax)
    {
        // Find closest point on AABB to sphere center
        Vector3 closestPoint = Vector3(
            Clamp(sphereCenter.x, aabbMin.x, aabbMax.x),
            Clamp(sphereCenter.y, aabbMin.y, aabbMax.y),
            Clamp(sphereCenter.z, aabbMin.z, aabbMax.z)
        );

        F32 distanceSquared = (sphereCenter - closestPoint).LengthSquared();
        return distanceSquared <= (sphereRadius * sphereRadius);
    }

    bool AABBAABBIntersect(const Vector3& min1, const Vector3& max1,
        const Vector3& min2, const Vector3& max2)
    {
        return (min1.x <= max2.x && max1.x >= min2.x) &&
            (min1.y <= max2.y && max1.y >= min2.y) &&
            (min1.z <= max2.z && max1.z >= min2.z);
    }

    // ==================================================================================
    // Area and Volume Calculations
    // ==================================================================================

    F32 TriangleArea(const Vector3& a, const Vector3& b, const Vector3& c)
    {
        Vector3 ab = b - a;
        Vector3 ac = c - a;
        return ab.Cross(ac).Length() * 0.5f;
    }

    F32 TriangleArea2D(const Vector2& a, const Vector2& b, const Vector2& c)
    {
        return Abs((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y)) * 0.5f;
    }

    Vector3 TriangleNormal(const Vector3& a, const Vector3& b, const Vector3& c)
    {
        Vector3 ab = b - a;
        Vector3 ac = c - a;
        return ab.Cross(ac).Normalized();
    }

    Vector3 TriangleCenter(const Vector3& a, const Vector3& b, const Vector3& c)
    {
        return (a + b + c) / 3.0f;
    }

    F32 TetrahedronVolume(const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& d)
    {
        Vector3 ab = b - a;
        Vector3 ac = c - a;
        Vector3 ad = d - a;
        return Abs(ab.Dot(ac.Cross(ad))) / 6.0f;
    }

    F32 SphereVolume(F32 radius)
    {
        return (4.0f / 3.0f) * PiF * radius * radius * radius;
    }

    F32 SphereSurfaceArea(F32 radius)
    {
        return 4.0f * PiF * radius * radius;
    }

    F32 CylinderVolume(F32 radius, F32 height)
    {
        return PiF * radius * radius * height;
    }

    F32 CylinderSurfaceArea(F32 radius, F32 height)
    {
        return 2.0f * PiF * radius * (radius + height);
    }

    // ==================================================================================
    // Barycentric Coordinates
    // ==================================================================================

    Vector3 BarycentricCoordinates(const Vector3& point, const Vector3& a, const Vector3& b, const Vector3& c)
    {
        Vector3 ab = b - a;
        Vector3 ac = c - a;
        Vector3 ap = point - a;

        F32 d00 = ab.Dot(ab);
        F32 d01 = ab.Dot(ac);
        F32 d11 = ac.Dot(ac);
        F32 d20 = ap.Dot(ab);
        F32 d21 = ap.Dot(ac);

        F32 denominator = d00 * d11 - d01 * d01;

        if (IsNearlyZero(denominator))
        {
            // Degenerate triangle
            return Vector3(1.0f, 0.0f, 0.0f);
        }

        F32 v = (d11 * d20 - d01 * d21) / denominator;
        F32 w = (d00 * d21 - d01 * d20) / denominator;
        F32 u = 1.0f - v - w;

        return Vector3(u, v, w);
    }

    Vector2 BarycentricCoordinates2D(const Vector2& point, const Vector2& a, const Vector2& b, const Vector2& c)
    {
        Vector2 ab = b - a;
        Vector2 ac = c - a;
        Vector2 ap = point - a;

        F32 d00 = ab.Dot(ab);
        F32 d01 = ab.Dot(ac);
        F32 d11 = ac.Dot(ac);
        F32 d20 = ap.Dot(ab);
        F32 d21 = ap.Dot(ac);

        F32 denominator = d00 * d11 - d01 * d01;

        if (IsNearlyZero(denominator))
        {
            return Vector2(0.0f, 0.0f);
        }

        F32 v = (d11 * d20 - d01 * d21) / denominator;
        F32 w = (d00 * d21 - d01 * d20) / denominator;

        return Vector2(v, w); // u = 1 - v - w
    }

    bool IsInsideTriangle(const Vector3& barycentrics)
    {
        return barycentrics.x >= 0.0f && barycentrics.y >= 0.0f && barycentrics.z >= 0.0f;
    }

    bool IsInsideTriangle2D(const Vector2& barycentrics)
    {
        F32 u = 1.0f - barycentrics.x - barycentrics.y;
        return u >= 0.0f && barycentrics.x >= 0.0f && barycentrics.y >= 0.0f;
    }

    Vector3 InterpolateBarycentric(const Vector3& barycentrics, const Vector3& valueA, const Vector3& valueB, const Vector3& valueC)
    {
        return valueA * barycentrics.x + valueB * barycentrics.y + valueC * barycentrics.z;
    }

    // ==================================================================================
    // Point-in-Shape Tests
    // ==================================================================================

    bool PointInSphere(const Vector3& point, const Vector3& sphereCenter, F32 sphereRadius)
    {
        return (point - sphereCenter).LengthSquared() <= (sphereRadius * sphereRadius);
    }

    bool PointInAABB(const Vector3& point, const Vector3& aabbMin, const Vector3& aabbMax)
    {
        return point.x >= aabbMin.x && point.x <= aabbMax.x &&
            point.y >= aabbMin.y && point.y <= aabbMax.y &&
            point.z >= aabbMin.z && point.z <= aabbMax.z;
    }

    bool PointInTriangle2D(const Vector2& point, const Vector2& a, const Vector2& b, const Vector2& c)
    {
        Vector2 bary = BarycentricCoordinates2D(point, a, b, c);
        return IsInsideTriangle2D(bary);
    }

    bool PointInCircle(const Vector2& point, const Vector2& circleCenter, F32 circleRadius)
    {
        return (point - circleCenter).LengthSquared() <= (circleRadius * circleRadius);
    }

    bool PointInCylinder(const Vector3& point, const Vector3& cylinderBase, const Vector3& cylinderTop, F32 cylinderRadius)
    {
        Vector3 axis = cylinderTop - cylinderBase;
        Vector3 pointToBase = point - cylinderBase;

        // Project point onto cylinder axis
        F32 t = pointToBase.Dot(axis) / axis.LengthSquared();

        // Check if point is within cylinder height
        if (t < 0.0f || t > 1.0f)
        {
            return false;
        }

        // Check if point is within cylinder radius
        Vector3 closestOnAxis = cylinderBase + axis * t;
        return (point - closestOnAxis).LengthSquared() <= (cylinderRadius * cylinderRadius);
    }

    // ==================================================================================
    // Utility Functions for Collision Detection
    // ==================================================================================

    Vector3 GetAABBCenter(const Vector3& aabbMin, const Vector3& aabbMax)
    {
        return (aabbMin + aabbMax) * 0.5f;
    }

    Vector3 GetAABBExtents(const Vector3& aabbMin, const Vector3& aabbMax)
    {
        return (aabbMax - aabbMin) * 0.5f;
    }

    Vector3 ExpandAABB(const Vector3& aabbMin, const Vector3& aabbMax, F32 expansion)
    {
        Vector3 expansionVec(expansion, expansion, expansion);
        return aabbMax + expansionVec; // Returns new max, min would be aabbMin - expansionVec
    }

    void GetAABBFromPoints(const Vector3* points, size_t count, Vector3& outMin, Vector3& outMax)
    {
        if (count == 0)
        {
            outMin = outMax = Vector3::Zero;
            return;
        }

        outMin = outMax = points[0];

        for (size_t i = 1; i < count; ++i)
        {
            outMin.x = Min(outMin.x, points[i].x);
            outMin.y = Min(outMin.y, points[i].y);
            outMin.z = Min(outMin.z, points[i].z);

            outMax.x = Max(outMax.x, points[i].x);
            outMax.y = Max(outMax.y, points[i].y);
            outMax.z = Max(outMax.z, points[i].z);
        }
    }
    // ==================================================================================
    // HSV to RGB Conversion
    // ==================================================================================

    Vector3 HSVToRGB(F32 h, F32 s, F32 v)
    {
        // Clamp input values to valid ranges
        h = Mod(h, 360.0f); // Hue wraps around at 360
        if (h < 0.0f) h += 360.0f;
        s = Clamp01(s);     // Saturation [0, 1]
        v = Clamp01(v);     // Value [0, 1]

        F32 c = v * s;    // Chroma
        F32 x = c * (1.0f - Abs(Mod(h / 60.0f, 2.0f) - 1.0f));
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

        return Vector3(r + m, g + m, b + m);
    }

    Vector3 HSVToRGB(const Vector3& hsv)
    {
        return HSVToRGB(hsv.x, hsv.y, hsv.z);
    }

    // ==================================================================================
    // RGB to HSV Conversion
    // ==================================================================================

    Vector3 RGBToHSV(F32 r, F32 g, F32 b)
    {
        // Clamp input values to valid range [0, 1]
        r = Clamp01(r);
        g = Clamp01(g);
        b = Clamp01(b);

        F32 maxVal = Max(r, Max(g, b));
        F32 minVal = Min(r, Min(g, b));
        F32 delta = maxVal - minVal;

        F32 h = 0.0f; // Hue
        F32 s = 0.0f; // Saturation
        F32 v = maxVal; // Value

        // Calculate saturation
        if (maxVal > EpsilonF)
        {
            s = delta / maxVal;
        }

        // Calculate hue
        if (delta > EpsilonF)
        {
            if (IsNearlyEqual(maxVal, r))
            {
                h = 60.0f * (Mod((g - b) / delta, 6.0f));
            }
            else if (IsNearlyEqual(maxVal, g))
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

        return Vector3(h, s, v);
    }

    Vector3 RGBToHSV(const Vector3& rgb)
    {
        return RGBToHSV(rgb.x, rgb.y, rgb.z);
    }

    // ==================================================================================
    // Additional Color Space Conversions
    // ==================================================================================

    // HSL (Hue, Saturation, Lightness) to RGB
    Vector3 HSLToRGB(F32 h, F32 s, F32 l)
    {
        h = Mod(h, 360.0f);
        if (h < 0.0f) h += 360.0f;
        s = Clamp01(s);
        l = Clamp01(l);

        F32 c = (1.0f - Abs(2.0f * l - 1.0f)) * s; // Chroma
        F32 x = c * (1.0f - Abs(Mod(h / 60.0f, 2.0f) - 1.0f));
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

        return Vector3(r + m, g + m, b + m);
    }

    Vector3 HSLToRGB(const Vector3& hsl)
    {
        return HSLToRGB(hsl.x, hsl.y, hsl.z);
    }

    // RGB to HSL conversion
    Vector3 RGBToHSL(F32 r, F32 g, F32 b)
    {
        r = Clamp01(r);
        g = Clamp01(g);
        b = Clamp01(b);

        F32 maxVal = Max(r, Max(g, b));
        F32 minVal = Min(r, Min(g, b));
        F32 delta = maxVal - minVal;

        F32 h = 0.0f; // Hue
        F32 s = 0.0f; // Saturation
        F32 l = (maxVal + minVal) / 2.0f; // Lightness

        // Calculate saturation
        if (delta > EpsilonF)
        {
            s = delta / (1.0f - Abs(2.0f * l - 1.0f));
        }

        // Calculate hue (same as HSV)
        if (delta > EpsilonF)
        {
            if (IsNearlyEqual(maxVal, r))
            {
                h = 60.0f * (Mod((g - b) / delta, 6.0f));
            }
            else if (IsNearlyEqual(maxVal, g))
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

        return Vector3(h, s, l);
    }

    Vector3 RGBToHSL(const Vector3& rgb)
    {
        return RGBToHSL(rgb.x, rgb.y, rgb.z);
    }

    // ==================================================================================
    // Color Utility Functions
    // ==================================================================================

    // Convert RGB from [0-255] to [0-1] range
    Vector3 RGBFromBytes(U8 r, U8 g, U8 b)
    {
        return Vector3(r / 255.0f, g / 255.0f, b / 255.0f);
    }

    // Convert RGB from [0-1] to [0-255] range
    Vector3 RGBToBytes(const Vector3& rgb)
    {
        return Vector3(
            Round(Clamp01(rgb.x) * 255.0f),
            Round(Clamp01(rgb.y) * 255.0f),
            Round(Clamp01(rgb.z) * 255.0f)
        );
    }

    // Pack RGB to 32-bit integer (ARGB format)
    U32 PackRGBA(const Vector3& rgb, F32 alpha)
    {
        U8 r = static_cast<U8>(Round(Clamp01(rgb.x) * 255.0f));
        U8 g = static_cast<U8>(Round(Clamp01(rgb.y) * 255.0f));
        U8 b = static_cast<U8>(Round(Clamp01(rgb.z) * 255.0f));
        U8 a = static_cast<U8>(Round(Clamp01(alpha) * 255.0f));

        return (static_cast<U32>(a) << 24) |
            (static_cast<U32>(r) << 16) |
            (static_cast<U32>(g) << 8) |
            static_cast<U32>(b);
    }

    // Unpack 32-bit integer to RGB + alpha
    Vector4 UnpackRGBA(U32 packedColor)
    {
        F32 a = ((packedColor >> 24) & 0xFF) / 255.0f;
        F32 r = ((packedColor >> 16) & 0xFF) / 255.0f;
        F32 g = ((packedColor >> 8) & 0xFF) / 255.0f;
        F32 b = (packedColor & 0xFF) / 255.0f;

        return Vector4(r, g, b, a);
    }

    // Gamma correction
    Vector3 LinearToGamma(const Vector3& linearColor, F32 gamma)
    {
        return Vector3(
            Pow(linearColor.x, 1.0f / gamma),
            Pow(linearColor.y, 1.0f / gamma),
            Pow(linearColor.z, 1.0f / gamma)
        );
    }

    Vector3 GammaToLinear(const Vector3& gammaColor, F32 gamma)
    {
        return Vector3(
            Pow(gammaColor.x, gamma),
            Pow(gammaColor.y, gamma),
            Pow(gammaColor.z, gamma)
        );
    }

    // Color interpolation
    Vector3 ColorLerp(const Vector3& colorA, const Vector3& colorB, F32 t)
    {
        return colorA.Lerp(colorB, t);
    }

    Vector3 ColorLerpHSV(const Vector3& rgbA, const Vector3& rgbB, F32 t)
    {
        Vector3 hsvA = RGBToHSV(rgbA);
        Vector3 hsvB = RGBToHSV(rgbB);

        // Handle hue interpolation (shortest path around color wheel)
        F32 hue1 = hsvA.x;
        F32 hue2 = hsvB.x;
        F32 hueDiff = hue2 - hue1;

        if (hueDiff > 180.0f)
        {
            hue2 -= 360.0f;
        }
        else if (hueDiff < -180.0f)
        {
            hue2 += 360.0f;
        }

        Vector3 hsvResult = Vector3(
            Lerp(hue1, hue2, t),
            Lerp(hsvA.y, hsvB.y, t),
            Lerp(hsvA.z, hsvB.z, t)
        );

        return HSVToRGB(hsvResult);
    }

    // Color manipulation functions
    Vector3 AdjustBrightness(const Vector3& rgb, F32 brightness)
    {
        Vector3 hsv = RGBToHSV(rgb);
        hsv.z = Clamp01(hsv.z * brightness);
        return HSVToRGB(hsv);
    }

    Vector3 AdjustSaturation(const Vector3& rgb, F32 saturation)
    {
        Vector3 hsv = RGBToHSV(rgb);
        hsv.y = Clamp01(hsv.y * saturation);
        return HSVToRGB(hsv);
    }

    Vector3 AdjustHue(const Vector3& rgb, F32 hueShift)
    {
        Vector3 hsv = RGBToHSV(rgb);
        hsv.x = Mod(hsv.x + hueShift, 360.0f);
        return HSVToRGB(hsv);
    }

    Vector3 AdjustContrast(const Vector3& rgb, F32 contrast)
    {
        return Vector3(
            Clamp01((rgb.x - 0.5f) * contrast + 0.5f),
            Clamp01((rgb.y - 0.5f) * contrast + 0.5f),
            Clamp01((rgb.z - 0.5f) * contrast + 0.5f)
        );
    }

    // Color blending modes
    Vector3 ColorMultiply(const Vector3& base, const Vector3& blend)
    {
        return base * blend;
    }

    Vector3 ColorScreen(const Vector3& base, const Vector3& blend)
    {
        return Vector3::One - (Vector3::One - base) * (Vector3::One - blend);
    }

    Vector3 ColorOverlay(const Vector3& base, const Vector3& blend)
    {
        return Vector3(
            base.x < 0.5f ? 2.0f * base.x * blend.x : 1.0f - 2.0f * (1.0f - base.x) * (1.0f - blend.x),
            base.y < 0.5f ? 2.0f * base.y * blend.y : 1.0f - 2.0f * (1.0f - base.y) * (1.0f - blend.y),
            base.z < 0.5f ? 2.0f * base.z * blend.z : 1.0f - 2.0f * (1.0f - base.z) * (1.0f - blend.z)
        );
    }

    Vector3 ColorSoftLight(const Vector3& base, const Vector3& blend)
    {
        return Vector3(
            blend.x < 0.5f ? 2.0f * base.x * blend.x + base.x * base.x * (1.0f - 2.0f * blend.x) :
            Sqrt(base.x) * (2.0f * blend.x - 1.0f) + 2.0f * base.x * (1.0f - blend.x),
            blend.y < 0.5f ? 2.0f * base.y * blend.y + base.y * base.y * (1.0f - 2.0f * blend.y) :
            Sqrt(base.y) * (2.0f * blend.y - 1.0f) + 2.0f * base.y * (1.0f - blend.y),
            blend.z < 0.5f ? 2.0f * base.z * blend.z + base.z * base.z * (1.0f - 2.0f * blend.z) :
            Sqrt(base.z) * (2.0f * blend.z - 1.0f) + 2.0f * base.z * (1.0f - blend.z)
        );
    }
}