module;

#include <Angaraka/Base.hpp>

export module Angaraka.Math.Ray;

import Angaraka.Math.Vector3;
import Angaraka.Math.Vector4;
import Angaraka.Math.Matrix4x4;

namespace Angaraka::Math {

    /**
     * @brief 3D Ray for raycasting and intersection tests
     *
     * A ray is defined by an origin point and a direction vector.
     * Used for mouse picking, collision detection, and visibility tests.
     */
    export struct Ray {
        Vector3 origin;
        Vector3 direction;  // Should be normalized for most operations

        // ==================== Constructors ====================

        /**
         * @brief Default constructor
         */
        Ray();

        /**
         * @brief Create ray from origin and direction
         * @param origin Starting point of the ray
         * @param direction Direction of the ray (will be normalized)
         */
        Ray(const Vector3& origin, const Vector3& direction);

        /**
         * @brief Create ray from two points
         * @param from Starting point
         * @param to Target point (ray points from 'from' to 'to')
         */
        static Ray FromPoints(const Vector3& from, const Vector3& to);

        // ==================== Operations ====================

        /**
         * @brief Get a point along the ray
         * @param distance Distance from origin
         * @return Point at origin + direction * distance
         */
        Vector3 GetPoint(F32 distance) const;

        /**
         * @brief Find closest point on ray to another point
         * @param point Point to find closest point to
         * @return Parameter t such that GetPoint(t) is closest to point
         */
        F32 ClosestParameterToPoint(const Vector3& point) const;

        /**
         * @brief Find closest point on ray to another point
         * @param point Point to find closest point to
         * @return Closest point on the ray
         */
        Vector3 ClosestPointToPoint(const Vector3& point) const;

        /**
         * @brief Distance from ray to point
         */
        F32 DistanceToPoint(const Vector3& point) const;

        /**
         * @brief Distance squared from ray to point (faster)
         */
        F32 DistanceSquaredToPoint(const Vector3& point) const;

        // ==================== Intersection Tests ====================

        /**
         * @brief Test intersection with plane
         * @param planeNormal Normal of the plane (must be normalized)
         * @param planeDistance Distance of plane from origin along normal
         * @param t Output: distance along ray to intersection
         * @return True if ray intersects plane
         */
        bool IntersectsPlane(const Vector3& planeNormal, F32 planeDistance, F32& t) const;

        /**
         * @brief Test intersection with triangle
         * @param v0, v1, v2 Triangle vertices
         * @param t Output: distance along ray to intersection
         * @param u, v Output: barycentric coordinates of hit point
         * @return True if ray intersects triangle
         */
        bool IntersectsTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2,
            F32& t, F32& u, F32& v) const;

        /**
         * @brief Test intersection with sphere
         * @param center Sphere center
         * @param radius Sphere radius
         * @param t1, t2 Output: distances to entry and exit points
         * @return True if ray intersects sphere
         */
        bool IntersectsSphere(const Vector3& center, F32 radius, F32& t1, F32& t2) const;

        /**
         * @brief Test intersection with sphere (simple version)
         * @return True if ray intersects sphere
         */
        bool IntersectsSphere(const Vector3& center, F32 radius) const;

        // ==================== Transformations ====================

        /**
         * @brief Transform ray by a matrix
         */
        Ray Transform(const Matrix4x4& matrix) const;

        // ==================== Screen to World ====================

        /**
         * @brief Create ray from screen coordinates
         * @param screenX, screenY Screen coordinates (0 to 1)
         * @param viewMatrix Camera view matrix
         * @param projectionMatrix Camera projection matrix
         * @param viewport Viewport dimensions (x, y, width, height)
         * @return Ray in world space
         */
        static Ray FromScreenCoordinates(F32 screenX, F32 screenY,
            const Matrix4x4& viewMatrix,
            const Matrix4x4& projectionMatrix,
            const Vector4& viewport);

        // ==================== Operators ====================

        bool operator==(const Ray& other) const;
        bool operator!=(const Ray& other) const;
    };
} // namespace Angaraka::Math