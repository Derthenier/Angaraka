module;

#include <Angaraka/Base.hpp>

export module Angaraka.Math.BoundingBox;

import Angaraka.Math.Vector3;
import Angaraka.Math.Matrix4x4;

namespace Angaraka::Math {

    /**
     * @brief Axis-Aligned Bounding Box (AABB)
     *
     * Represents a box aligned with the world axes, defined by
     * minimum and maximum corners. Used for collision detection,
     * frustum culling, and spatial queries.
     */
    export struct BoundingBox {
        Vector3 min;
        Vector3 max;

        // ==================== Constructors ====================

        /**
         * @brief Default constructor - creates invalid/empty box
         */
        BoundingBox();

        /**
         * @brief Create box from min and max corners
         */
        BoundingBox(const Vector3& min, const Vector3& max);

        /**
         * @brief Create box from center and half-extents
         */
        static BoundingBox FromCenterAndHalfExtents(const Vector3& center, const Vector3& halfExtents);

        /**
         * @brief Create box from array of points
         */
        static BoundingBox FromPoints(const Vector3* points, size_t count);

        // ==================== Properties ====================

        /**
         * @brief Get the center of the box
         */
        Vector3 GetCenter() const;

        /**
         * @brief Get the size (dimensions) of the box
         */
        Vector3 GetSize() const;

        /**
         * @brief Get half the size (half-extents) of the box
         */
        Vector3 GetHalfExtents() const;

        /**
         * @brief Get the volume of the box
         */
        F32 GetVolume() const;

        /**
         * @brief Get the surface area of the box
         */
        F32 GetSurfaceArea() const;

        /**
         * @brief Get diagonal length of the box
         */
        F32 GetDiagonalLength() const;

        /**
         * @brief Check if box is valid (min <= max)
         */
        bool IsValid() const;

        /**
         * @brief Check if box is empty (zero volume)
         */
        bool IsEmpty() const;

        // ==================== Operations ====================

        /**
         * @brief Expand box to include a point
         */
        void ExpandToInclude(const Vector3& point);

        /**
         * @brief Expand box to include another box
         */
        void ExpandToInclude(const BoundingBox& other);

        /**
         * @brief Scale the box around its center
         */
        void Scale(F32 factor);

        /**
         * @brief Translate the box
         */
        void Translate(const Vector3& offset);

        /**
         * @brief Reset to invalid/empty state
         */
        void Reset();

        // ==================== Intersection Tests ====================

        /**
         * @brief Test if point is inside the box
         */
        bool Contains(const Vector3& point) const;

        /**
         * @brief Test if another box is completely inside this box
         */
        bool Contains(const BoundingBox& other) const;

        /**
         * @brief Test if this box intersects another box
         */
        bool Intersects(const BoundingBox& other) const;

        /**
         * @brief Test intersection with ray
         * @param rayOrigin Ray starting point
         * @param rayDirection Ray direction (should be normalized)
         * @param tMin Output: distance to entry point
         * @param tMax Output: distance to exit point
         * @return True if ray intersects box
         */
        bool IntersectsRay(const Vector3& rayOrigin, const Vector3& rayDirection,
            F32& tMin, F32& tMax) const;

        /**
         * @brief Test intersection with sphere
         */
        bool IntersectsSphere(const Vector3& center, F32 radius) const;

        // ==================== Transformations ====================

        /**
         * @brief Transform box by a matrix (creates new AABB)
         * @note Result is axis-aligned in the new space
         */
        BoundingBox Transform(const Matrix4x4& matrix) const;

        // ==================== Corner Points ====================

        /**
         * @brief Get all 8 corner points of the box
         */
        std::array<Vector3, 8> GetCorners() const;

        /**
         * @brief Get a specific corner by index (0-7)
         * Corner indices:
         * 0: min
         * 1: (max.x, min.y, min.z)
         * 2: (min.x, max.y, min.z)
         * 3: (max.x, max.y, min.z)
         * 4: (min.x, min.y, max.z)
         * 5: (max.x, min.y, max.z)
         * 6: (min.x, max.y, max.z)
         * 7: max
         */
        Vector3 GetCorner(U32 index) const;

        // ==================== Static Operations ====================

        /**
         * @brief Compute union of two boxes
         */
        static BoundingBox Union(const BoundingBox& a, const BoundingBox& b);

        /**
         * @brief Compute intersection of two boxes
         * @return Invalid box if no intersection
         */
        static BoundingBox Intersection(const BoundingBox& a, const BoundingBox& b);

        // ==================== Operators ====================

        bool operator==(const BoundingBox& other) const;
        bool operator!=(const BoundingBox& other) const;
    };
} // namespace Angaraka::Math