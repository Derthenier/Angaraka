module;

#include <cmath>

module Angaraka.Math.Frustum;

import Angaraka.Math.Vector3;
import Angaraka.Math.Vector4;

namespace Angaraka::Math {

    Frustum::Frustum() {
        // Initialize with default planes
        for (auto& plane : m_planes) {
            plane = Vector4(0, 0, 0, 0);
        }
    }

    Frustum::Frustum(const Matrix4x4& viewProjection) {
        ExtractFromMatrix(viewProjection);
    }

    Frustum::Frustum(const Matrix4x4& view, const Matrix4x4& projection) {
        ExtractFromMatrix(projection * view);
    }

    void Frustum::ExtractFromMatrix(const Matrix4x4& viewProjection) {
        // Extract frustum planes from the view-projection matrix
        // Planes are in the form: ax + by + cz + d = 0

        const F32* m = &(viewProjection.m[0]); // Assuming column-major storage

        // Left plane
        m_planes[Left] = Vector4(
            m[3] + m[0],
            m[7] + m[4],
            m[11] + m[8],
            m[15] + m[12]
        );

        // Right plane
        m_planes[Right] = Vector4(
            m[3] - m[0],
            m[7] - m[4],
            m[11] - m[8],
            m[15] - m[12]
        );

        // Bottom plane
        m_planes[Bottom] = Vector4(
            m[3] + m[1],
            m[7] + m[5],
            m[11] + m[9],
            m[15] + m[13]
        );

        // Top plane
        m_planes[Top] = Vector4(
            m[3] - m[1],
            m[7] - m[5],
            m[11] - m[9],
            m[15] - m[13]
        );

        // Near plane
        m_planes[Near] = Vector4(
            m[3] + m[2],
            m[7] + m[6],
            m[11] + m[10],
            m[15] + m[14]
        );

        // Far plane
        m_planes[Far] = Vector4(
            m[3] - m[2],
            m[7] - m[6],
            m[11] - m[10],
            m[15] - m[14]
        );

        // Normalize the planes
        NormalizePlanes();

        m_cornersValid = false;
    }

    void Frustum::SetFromPerspective(const Vector3& position, const Vector3& forward, const Vector3& up,
        F32 fovY, F32 aspectRatio, F32 nearPlane, F32 farPlane) {
        Vector3 right = forward.Cross(up).Normalized();
        Vector3 realUp = right.Cross(forward).Normalized();

        F32 halfVSide = farPlane * std::tan(fovY * 0.5f);
        F32 halfHSide = halfVSide * aspectRatio;

        Vector3 frontMultFar = forward * farPlane;

        // Calculate the 8 corners of the frustum
        m_corners[0] = position + forward * nearPlane - right * (halfHSide * nearPlane / farPlane) - realUp * (halfVSide * nearPlane / farPlane);
        m_corners[1] = position + forward * nearPlane + right * (halfHSide * nearPlane / farPlane) - realUp * (halfVSide * nearPlane / farPlane);
        m_corners[2] = position + forward * nearPlane - right * (halfHSide * nearPlane / farPlane) + realUp * (halfVSide * nearPlane / farPlane);
        m_corners[3] = position + forward * nearPlane + right * (halfHSide * nearPlane / farPlane) + realUp * (halfVSide * nearPlane / farPlane);

        m_corners[4] = position + frontMultFar - right * halfHSide - realUp * halfVSide;
        m_corners[5] = position + frontMultFar + right * halfHSide - realUp * halfVSide;
        m_corners[6] = position + frontMultFar - right * halfHSide + realUp * halfVSide;
        m_corners[7] = position + frontMultFar + right * halfHSide + realUp * halfVSide;

        m_cornersValid = true;

        // Compute planes from corners
        // Near plane
        Vector3 nearNormal = forward;
        m_planes[Near] = Vector4(nearNormal.x, nearNormal.y, nearNormal.z, -nearNormal.Dot(position + forward * nearPlane));

        // Far plane
        Vector3 farNormal = -forward;
        m_planes[Far] = Vector4(farNormal.x, farNormal.y, farNormal.z, -farNormal.Dot(position + frontMultFar));

        // Left plane
        Vector3 leftNormal = (m_corners[4] - position).Cross(m_corners[2] - position).Normalized();
        m_planes[Left] = Vector4(leftNormal.x, leftNormal.y, leftNormal.z, -leftNormal.Dot(position));

        // Right plane
        Vector3 rightNormal = (m_corners[3] - position).Cross(m_corners[5] - position).Normalized();
        m_planes[Right] = Vector4(rightNormal.x, rightNormal.y, rightNormal.z, -rightNormal.Dot(position));

        // Top plane
        Vector3 topNormal = (m_corners[6] - position).Cross(m_corners[3] - position).Normalized();
        m_planes[Top] = Vector4(topNormal.x, topNormal.y, topNormal.z, -topNormal.Dot(position));

        // Bottom plane
        Vector3 bottomNormal = (m_corners[1] - position).Cross(m_corners[4] - position).Normalized();
        m_planes[Bottom] = Vector4(bottomNormal.x, bottomNormal.y, bottomNormal.z, -bottomNormal.Dot(position));
    }

    void Frustum::SetFromOrthographic(const Vector3& position, const Vector3& forward, const Vector3& up,
        F32 width, F32 height, F32 nearPlane, F32 farPlane) {
        Vector3 right = forward.Cross(up).Normalized();
        Vector3 realUp = right.Cross(forward).Normalized();

        F32 halfWidth = width * 0.5f;
        F32 halfHeight = height * 0.5f;

        // Near plane
        m_planes[Near] = Vector4(forward.x, forward.y, forward.z, -forward.Dot(position + forward * nearPlane));

        // Far plane
        m_planes[Far] = Vector4(-forward.x, -forward.y, -forward.z, forward.Dot(position + forward * farPlane));

        // Left plane
        m_planes[Left] = Vector4(right.x, right.y, right.z, -right.Dot(position - right * halfWidth));

        // Right plane
        m_planes[Right] = Vector4(-right.x, -right.y, -right.z, right.Dot(position + right * halfWidth));

        // Top plane
        m_planes[Top] = Vector4(-realUp.x, -realUp.y, -realUp.z, realUp.Dot(position + realUp * halfHeight));

        // Bottom plane
        m_planes[Bottom] = Vector4(realUp.x, realUp.y, realUp.z, -realUp.Dot(position - realUp * halfHeight));

        m_cornersValid = false;
    }

    bool Frustum::Contains(const Vector3& point) const {
        for (const auto& plane : m_planes) {
            Vector3 normal(plane.x, plane.y, plane.z);
            F32 distance = plane.w;

            if (normal.Dot(point) + distance < 0) {
                return false;
            }
        }
        return true;
    }

    Frustum::IntersectionResult Frustum::IntersectsSphere(const Vector3& center, F32 radius) const {
        IntersectionResult result = IntersectionResult::Inside;

        for (const auto& plane : m_planes) {
            Vector3 normal(plane.x, plane.y, plane.z);
            F32 distance = plane.w;

            F32 signedDistance = normal.Dot(center) + distance;

            if (signedDistance < -radius) {
                return IntersectionResult::Outside;
            }
            else if (signedDistance < radius) {
                result = IntersectionResult::Intersect;
            }
        }

        return result;
    }

    Frustum::IntersectionResult Frustum::IntersectsBoundingBox(const BoundingBox& box) const {
        IntersectionResult result = IntersectionResult::Inside;

        for (const auto& plane : m_planes) {
            Vector3 normal(plane.x, plane.y, plane.z);
            F32 distance = plane.w;

            // Find the positive and negative vertices relative to the plane normal
            Vector3 positiveVertex = box.min;
            Vector3 negativeVertex = box.max;

            if (normal.x >= 0) {
                positiveVertex.x = box.max.x;
                negativeVertex.x = box.min.x;
            }
            if (normal.y >= 0) {
                positiveVertex.y = box.max.y;
                negativeVertex.y = box.min.y;
            }
            if (normal.z >= 0) {
                positiveVertex.z = box.max.z;
                negativeVertex.z = box.min.z;
            }

            // Check if the box is completely outside this plane
            if (normal.Dot(positiveVertex) + distance < 0) {
                return IntersectionResult::Outside;
            }

            // Check if the box intersects this plane
            if (normal.Dot(negativeVertex) + distance < 0) {
                result = IntersectionResult::Intersect;
            }
        }

        return result;
    }

    bool Frustum::Intersects(const BoundingBox& box) const {
        IntersectionResult result = IntersectsBoundingBox(box);
        return result != IntersectionResult::Outside;
    }

    bool Frustum::Intersects(const Vector3& center, F32 radius) const {
        IntersectionResult result = IntersectsSphere(center, radius);
        return result != IntersectionResult::Outside;
    }

    const Vector4& Frustum::GetPlane(PlaneIndex index) const {
        return m_planes[static_cast<size_t>(index)];
    }

    std::array<Vector3, 8> Frustum::GetCorners() const {
        if (!m_cornersValid) {
            ComputeCorners();
        }
        return m_corners;
    }

    F32 Frustum::DistanceToPlane(const Vector3& point, PlaneIndex planeIndex) const {
        const Vector4& plane = m_planes[static_cast<size_t>(planeIndex)];
        Vector3 normal(plane.x, plane.y, plane.z);
        return normal.Dot(point) + plane.w;
    }

    void Frustum::NormalizePlanes() {
        for (auto& plane : m_planes) {
            F32 length = std::sqrt(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
            if (length > 0.0f) {
                plane /= length;
            }
        }
    }

    void Frustum::ComputeCorners() const {
        // This is a placeholder - computing corners from planes is complex
        // For now, corners should be set when using SetFromPerspective/Orthographic
        m_cornersValid = true;
    }

} // namespace Angaraka::Math