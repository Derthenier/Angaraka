#include "Angaraka/Math/Ray.hpp"
#include "Angaraka/Math/Matrix44.hpp"
#include <cmath>
#include <algorithm>

namespace Angaraka::Math {

    Ray::Ray()
        : origin(0, 0, 0), direction(0, 0, 1) {
    }

    Ray::Ray(const Vector3& origin, const Vector3& direction)
        : origin(origin), direction(direction.Normalized()) {
    }

    Ray Ray::FromPoints(const Vector3& from, const Vector3& to) {
        return Ray(from, (to - from).Normalized());
    }

    Vector3 Ray::GetPoint(F32 distance) const {
        return origin + direction * distance;
    }

    F32 Ray::ClosestParameterToPoint(const Vector3& point) const {
        Vector3 toPoint = point - origin;
        F32 t = toPoint.Dot(direction);
        return std::max(0.0f, t); // Clamp to positive values (ray, not line)
    }

    Vector3 Ray::ClosestPointToPoint(const Vector3& point) const {
        return GetPoint(ClosestParameterToPoint(point));
    }

    F32 Ray::DistanceToPoint(const Vector3& point) const {
        return (point - ClosestPointToPoint(point)).Length();
    }

    F32 Ray::DistanceSquaredToPoint(const Vector3& point) const {
        return (point - ClosestPointToPoint(point)).LengthSquared();
    }

    bool Ray::IntersectsPlane(const Vector3& planeNormal, F32 planeDistance, F32& t) const {
        F32 denom = planeNormal.Dot(direction);

        // Ray is parallel to plane
        if (std::abs(denom) < 1e-6f) {
            return false;
        }

        // Calculate intersection distance
        t = (planeDistance - planeNormal.Dot(origin)) / denom;

        // Ray points away from plane
        if (t < 0.0f) {
            return false;
        }

        return true;
    }

    bool Ray::IntersectsTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2,
        F32& t, F32& u, F32& v) const {
        // Moller-Trumbore intersection algorithm
        const F32 EPSILON = 1e-6f;

        Vector3 edge1 = v1 - v0;
        Vector3 edge2 = v2 - v0;
        Vector3 h = direction.Cross(edge2);
        F32 a = edge1.Dot(h);

        if (a > -EPSILON && a < EPSILON) {
            return false; // Ray is parallel to triangle
        }

        F32 f = 1.0f / a;
        Vector3 s = origin - v0;
        u = f * s.Dot(h);

        if (u < 0.0f || u > 1.0f) {
            return false;
        }

        Vector3 q = s.Cross(edge1);
        v = f * direction.Dot(q);

        if (v < 0.0f || u + v > 1.0f) {
            return false;
        }

        // Calculate t to find intersection point
        t = f * edge2.Dot(q);

        if (t > EPSILON) {
            return true; // Ray intersection
        }

        return false; // Line intersection but not ray intersection
    }

    bool Ray::IntersectsSphere(const Vector3& center, F32 radius, F32& t1, F32& t2) const {
        Vector3 m = origin - center;
        F32 b = m.Dot(direction);
        F32 c = m.Dot(m) - radius * radius;

        // Exit if ray's origin is outside sphere and ray is pointing away from sphere
        if (c > 0.0f && b > 0.0f) {
            return false;
        }

        F32 discr = b * b - c;

        // Negative discriminant means ray misses sphere
        if (discr < 0.0f) {
            return false;
        }

        // Ray intersects sphere
        F32 sqrtDiscr = std::sqrt(discr);
        t1 = -b - sqrtDiscr;
        t2 = -b + sqrtDiscr;

        // Make sure t1 is the smaller value
        if (t1 > t2) {
            std::swap(t1, t2);
        }

        return true;
    }

    bool Ray::IntersectsSphere(const Vector3& center, F32 radius) const {
        F32 t1, t2;
        return IntersectsSphere(center, radius, t1, t2) && t2 >= 0.0f;
    }

    Ray Ray::Transform(const Matrix4x4& matrix) const {
        Vector3 newOrigin = matrix.TransformPoint(origin);
        Vector3 newDirection = matrix.TransformDirection(direction).Normalized();
        return Ray(newOrigin, newDirection);
    }

    Ray Ray::FromScreenCoordinates(F32 screenX, F32 screenY,
        const Matrix4x4& viewMatrix,
        const Matrix4x4& projectionMatrix,
        const Vector4& viewport) {
        // Convert screen coordinates to NDC
        F32 ndcX = (2.0f * screenX - viewport.z) / viewport.z;
        F32 ndcY = -(2.0f * screenY - viewport.w) / viewport.w; // Flip Y

        // Create points in NDC space
        Vector4 nearPoint(ndcX, ndcY, 0.0f, 1.0f);  // Near plane
        Vector4 farPoint(ndcX, ndcY, 1.0f, 1.0f);   // Far plane

        // Get inverse matrices
        Matrix4x4 invProj = projectionMatrix.Inverted();
        Matrix4x4 invView = viewMatrix.Inverted();
        Matrix4x4 invViewProj = (projectionMatrix * viewMatrix).Inverted();

        // Transform to world space
        Vector4 nearWorld = invViewProj * nearPoint;
        Vector4 farWorld = invViewProj * farPoint;

        // Perspective divide
        nearWorld /= nearWorld.w;
        farWorld /= farWorld.w;

        // Create ray from near to far
        Vector3 origin(nearWorld.x, nearWorld.y, nearWorld.z);
        Vector3 direction = Vector3(farWorld.x, farWorld.y, farWorld.z) - origin;

        return Ray(origin, direction.Normalized());
    }

    bool Ray::operator==(const Ray& other) const {
        return origin == other.origin && direction == other.direction;
    }

    bool Ray::operator!=(const Ray& other) const {
        return !(*this == other);
    }

} // namespace Angaraka::Math