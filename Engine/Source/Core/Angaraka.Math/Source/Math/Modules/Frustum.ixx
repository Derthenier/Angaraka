module;

#include <Angaraka/Base.hpp>

export module Angaraka.Math.Frustum;

import Angaraka.Math.Vector3;
import Angaraka.Math.Vector4;
import Angaraka.Math.Matrix4x4;
import Angaraka.Math.BoundingBox;

namespace Angaraka::Math {

    /**
     * @brief View frustum for frustum culling
     *
     * Represents the 6 planes of a view frustum (near, far, left, right, top, bottom).
     * Used to determine which objects are visible to the camera.
     */
    export class Frustum {
    public:
        /**
         * @brief Plane indices
         */
        enum PlaneIndex {
            Near = 0,
            Far = 1,
            Left = 2,
            Right = 3,
            Top = 4,
            Bottom = 5,
            Count = 6
        };

        /**
         * @brief Intersection result
         */
        enum class IntersectionResult {
            Outside,    // Completely outside frustum
            Inside,     // Completely inside frustum
            Intersect   // Partially inside frustum
        };

        // ==================== Constructors ====================

        /**
         * @brief Default constructor
         */
        Frustum();

        /**
         * @brief Create frustum from view-projection matrix
         * @param viewProjection Combined view * projection matrix
         */
        explicit Frustum(const Matrix4x4& viewProjection);

        /**
         * @brief Create frustum from separate view and projection matrices
         */
        Frustum(const Matrix4x4& view, const Matrix4x4& projection);

        // ==================== Setup ====================

        /**
         * @brief Extract frustum planes from view-projection matrix
         * @param viewProjection Combined view * projection matrix
         */
        void ExtractFromMatrix(const Matrix4x4& viewProjection);

        /**
         * @brief Create frustum from field of view parameters
         * @param position Camera position
         * @param forward Camera forward direction (normalized)
         * @param up Camera up direction (normalized)
         * @param fovY Vertical field of view in radians
         * @param aspectRatio Width / Height
         * @param nearPlane Near clipping distance
         * @param farPlane Far clipping distance
         */
        void SetFromPerspective(const Vector3& position, const Vector3& forward, const Vector3& up,
            F32 fovY, F32 aspectRatio, F32 nearPlane, F32 farPlane);

        /**
         * @brief Create frustum from orthographic parameters
         */
        void SetFromOrthographic(const Vector3& position, const Vector3& forward, const Vector3& up,
            F32 width, F32 height, F32 nearPlane, F32 farPlane);

        // ==================== Intersection Tests ====================

        /**
         * @brief Test if point is inside frustum
         */
        bool Contains(const Vector3& point) const;

        /**
         * @brief Test if sphere intersects frustum
         * @return Intersection result (Outside, Inside, or Intersect)
         */
        IntersectionResult IntersectsSphere(const Vector3& center, F32 radius) const;

        /**
         * @brief Test if bounding box intersects frustum
         * @return Intersection result (Outside, Inside, or Intersect)
         */
        IntersectionResult IntersectsBoundingBox(const BoundingBox& box) const;

        /**
         * @brief Quick test if bounding box intersects frustum
         * @return True if box is at least partially inside
         */
        bool Intersects(const BoundingBox& box) const;

        /**
         * @brief Quick test if sphere intersects frustum
         * @return True if sphere is at least partially inside
         */
        bool Intersects(const Vector3& center, F32 radius) const;

        // ==================== Plane Access ====================

        /**
         * @brief Get a specific plane
         * @return Plane as Vector4 (x,y,z = normal, w = distance)
         */
        const Vector4& GetPlane(PlaneIndex index) const;

        /**
         * @brief Get all planes
         */
        const std::array<Vector4, 6>& GetPlanes() const { return m_planes; }

        /**
         * @brief Get frustum corners in world space
         * @return 8 corners: near (bl, br, tl, tr), far (bl, br, tl, tr)
         */
        std::array<Vector3, 8> GetCorners() const;

        // ==================== Utilities ====================

        /**
         * @brief Get signed distance from point to plane
         * @param point Test point
         * @param planeIndex Which plane to test
         * @return Positive if in front of plane, negative if behind
         */
        F32 DistanceToPlane(const Vector3& point, PlaneIndex planeIndex) const;

        /**
         * @brief Normalize all plane equations
         * Call this after manually setting planes
         */
        void NormalizePlanes();

    private:
        // Planes stored as (normal.x, normal.y, normal.z, distance)
        // Plane equation: normal <dot> point + distance = 0
        std::array<Vector4, 6> m_planes;

        // Cached corner points (optional, computed on demand)
        mutable std::array<Vector3, 8> m_corners;
        mutable bool m_cornersValid = false;

        // Helper to compute corners
        void ComputeCorners() const;
    };
} // namespace Angaraka::Math