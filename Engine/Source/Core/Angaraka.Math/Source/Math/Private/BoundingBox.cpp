module;

#include <algorithm>
#include <cmath>

module Angaraka.Math.BoundingBox;

import Angaraka.Math.Vector3;
import Angaraka.Math.Matrix4x4;

namespace Angaraka::Math {

    BoundingBox::BoundingBox()
        : min(std::numeric_limits<F32>::max(),
            std::numeric_limits<F32>::max(),
            std::numeric_limits<F32>::max())
        , max(-std::numeric_limits<F32>::max(),
            -std::numeric_limits<F32>::max(),
            -std::numeric_limits<F32>::max()) {
    }

    BoundingBox::BoundingBox(const Vector3& min, const Vector3& max)
        : min(min), max(max) {
    }

    BoundingBox BoundingBox::FromCenterAndHalfExtents(const Vector3& center, const Vector3& halfExtents) {
        return BoundingBox(center - halfExtents, center + halfExtents);
    }

    BoundingBox BoundingBox::FromPoints(const Vector3* points, size_t count) {
        if (count == 0) {
            return BoundingBox();
        }

        BoundingBox box(points[0], points[0]);
        for (size_t i = 1; i < count; ++i) {
            box.ExpandToInclude(points[i]);
        }
        return box;
    }

    Vector3 BoundingBox::GetCenter() const {
        return (min + max) * 0.5f;
    }

    Vector3 BoundingBox::GetSize() const {
        return max - min;
    }

    Vector3 BoundingBox::GetHalfExtents() const {
        return (max - min) * 0.5f;
    }

    F32 BoundingBox::GetVolume() const {
        Vector3 size = GetSize();
        return size.x * size.y * size.z;
    }

    F32 BoundingBox::GetSurfaceArea() const {
        Vector3 size = GetSize();
        return 2.0f * (size.x * size.y + size.x * size.z + size.y * size.z);
    }

    F32 BoundingBox::GetDiagonalLength() const {
        return (max - min).Length();
    }

    bool BoundingBox::IsValid() const {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    bool BoundingBox::IsEmpty() const {
        return min.x >= max.x || min.y >= max.y || min.z >= max.z;
    }

    void BoundingBox::ExpandToInclude(const Vector3& point) {
        min.x = std::min(min.x, point.x);
        min.y = std::min(min.y, point.y);
        min.z = std::min(min.z, point.z);
        max.x = std::max(max.x, point.x);
        max.y = std::max(max.y, point.y);
        max.z = std::max(max.z, point.z);
    }

    void BoundingBox::ExpandToInclude(const BoundingBox& other) {
        min.x = std::min(min.x, other.min.x);
        min.y = std::min(min.y, other.min.y);
        min.z = std::min(min.z, other.min.z);
        max.x = std::max(max.x, other.max.x);
        max.y = std::max(max.y, other.max.y);
        max.z = std::max(max.z, other.max.z);
    }

    void BoundingBox::Scale(F32 factor) {
        Vector3 center = GetCenter();
        Vector3 halfExtents = GetHalfExtents() * factor;
        min = center - halfExtents;
        max = center + halfExtents;
    }

    void BoundingBox::Translate(const Vector3& offset) {
        min += offset;
        max += offset;
    }

    void BoundingBox::Reset() {
        *this = BoundingBox();
    }

    bool BoundingBox::Contains(const Vector3& point) const {
        return point.x >= min.x && point.x <= max.x &&
            point.y >= min.y && point.y <= max.y &&
            point.z >= min.z && point.z <= max.z;
    }

    bool BoundingBox::Contains(const BoundingBox& other) const {
        return other.min.x >= min.x && other.max.x <= max.x &&
            other.min.y >= min.y && other.max.y <= max.y &&
            other.min.z >= min.z && other.max.z <= max.z;
    }

    bool BoundingBox::Intersects(const BoundingBox& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
            min.y <= other.max.y && max.y >= other.min.y &&
            min.z <= other.max.z && max.z >= other.min.z;
    }

    bool BoundingBox::IntersectsRay(const Vector3& rayOrigin, const Vector3& rayDirection,
        F32& tMin, F32& tMax) const {
        // Slab intersection algorithm
        tMin = 0.0f;
        tMax = std::numeric_limits<F32>::max();

        for (int i = 0; i < 3; ++i) {
            F32 origin = rayOrigin[i];
            F32 dir = rayDirection[i];
            F32 minVal = min[i];
            F32 maxVal = max[i];

            if (std::abs(dir) < 1e-6f) {
                // Ray is parallel to slab
                if (origin < minVal || origin > maxVal) {
                    return false;
                }
            }
            else {
                // Compute intersection distances
                F32 invDir = 1.0f / dir;
                F32 t1 = (minVal - origin) * invDir;
                F32 t2 = (maxVal - origin) * invDir;

                if (t1 > t2) {
                    std::swap(t1, t2);
                }

                tMin = std::max(tMin, t1);
                tMax = std::min(tMax, t2);

                if (tMin > tMax) {
                    return false;
                }
            }
        }

        return true;
    }

    bool BoundingBox::IntersectsSphere(const Vector3& center, F32 radius) const {
        // Find closest point on box to sphere center
        Vector3 closest;
        closest.x = std::max(min.x, std::min(center.x, max.x));
        closest.y = std::max(min.y, std::min(center.y, max.y));
        closest.z = std::max(min.z, std::min(center.z, max.z));

        // Check if distance is less than radius
        F32 distSq = (center - closest).LengthSquared();
        return distSq <= radius * radius;
    }

    BoundingBox BoundingBox::Transform(const Matrix4x4& matrix) const {
        // Transform all 8 corners and create new AABB
        std::array<Vector3, 8> corners = GetCorners();

        BoundingBox result;
        result.min = result.max = matrix.TransformPoint(corners[0]);

        for (size_t i = 1; i < 8; ++i) {
            result.ExpandToInclude(matrix.TransformPoint(corners[i]));
        }

        return result;
    }

    std::array<Vector3, 8> BoundingBox::GetCorners() const {
        return {
            Vector3(min.x, min.y, min.z),
            Vector3(max.x, min.y, min.z),
            Vector3(min.x, max.y, min.z),
            Vector3(max.x, max.y, min.z),
            Vector3(min.x, min.y, max.z),
            Vector3(max.x, min.y, max.z),
            Vector3(min.x, max.y, max.z),
            Vector3(max.x, max.y, max.z)
        };
    }

    Vector3 BoundingBox::GetCorner(U32 index) const {
        switch (index) {
        case 0: return Vector3(min.x, min.y, min.z);
        case 1: return Vector3(max.x, min.y, min.z);
        case 2: return Vector3(min.x, max.y, min.z);
        case 3: return Vector3(max.x, max.y, min.z);
        case 4: return Vector3(min.x, min.y, max.z);
        case 5: return Vector3(max.x, min.y, max.z);
        case 6: return Vector3(min.x, max.y, max.z);
        case 7: return Vector3(max.x, max.y, max.z);
        default: return min;
        }
    }

    BoundingBox BoundingBox::Union(const BoundingBox& a, const BoundingBox& b) {
        return BoundingBox(
            Vector3(std::min(a.min.x, b.min.x),
                std::min(a.min.y, b.min.y),
                std::min(a.min.z, b.min.z)),
            Vector3(std::max(a.max.x, b.max.x),
                std::max(a.max.y, b.max.y),
                std::max(a.max.z, b.max.z))
        );
    }

    BoundingBox BoundingBox::Intersection(const BoundingBox& a, const BoundingBox& b) {
        return BoundingBox(
            Vector3(std::max(a.min.x, b.min.x),
                std::max(a.min.y, b.min.y),
                std::max(a.min.z, b.min.z)),
            Vector3(std::min(a.max.x, b.max.x),
                std::min(a.max.y, b.max.y),
                std::min(a.max.z, b.max.z))
        );
    }

    bool BoundingBox::operator==(const BoundingBox& other) const {
        return min == other.min && max == other.max;
    }

    bool BoundingBox::operator!=(const BoundingBox& other) const {
        return !(*this == other);
    }

} // namespace Angaraka::Math