// ==================================================================================
// AngarakaMath/Public/Math/Transform.cpp
// ==================================================================================
module;

#include <Angaraka/Base.hpp>
#include <cmath>

module Angaraka.Math.Transform;

import Angaraka.Math.Vector3;
import Angaraka.Math.Matrix4x4;
import Angaraka.Math.Quaternion;

namespace Angaraka::Math {

    // ==================================================================================
    // Transform Implementation
    // ==================================================================================

    Transform::Transform()
        : position(Vector3::Zero), rotation(Quaternion::Identity()), scale(Vector3::One)
    {
    }

    Transform::Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale)
        : position(position), rotation(rotation), scale(scale)
    {
    }

    Matrix4x4 Transform::ToMatrix() const
    {
        return Matrix4x4::TRS(position, rotation, scale);
    }

    Transform Transform::Inverted() const
    {
        Quaternion invRotation = rotation.Inverted();
        Vector3 invScale = Vector3(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z);
        Vector3 invPosition = invRotation.RotateVector(-position * invScale);

        return Transform(invPosition, invRotation, invScale);
    }

    Vector3 Transform::TransformPoint(const Vector3& point) const
    {
        Vector3 scaledPoint = point * scale;
        Vector3 rotatedPoint = rotation.RotateVector(scaledPoint);
        return rotatedPoint + position;
    }

    Vector3 Transform::TransformDirection(const Vector3& direction) const
    {
        Vector3 scaledDirection = direction * scale;
        return rotation.RotateVector(scaledDirection);
    }

    Vector3 Transform::InverseTransformPoint(const Vector3& point) const
    {
        Vector3 translatedPoint = point - position;
        Vector3 rotatedPoint = rotation.Inverted().RotateVector(translatedPoint);
        return rotatedPoint / scale;
    }

    Vector3 Transform::InverseTransformDirection(const Vector3& direction) const
    {
        Vector3 rotatedDirection = rotation.Inverted().RotateVector(direction);
        return rotatedDirection / scale;
    }

    Transform Transform::operator*(const Transform& rhs) const
    {
        Vector3 newPosition = TransformPoint(rhs.position);
        Quaternion newRotation = rotation * rhs.rotation;
        Vector3 newScale = scale * rhs.scale;

        return Transform(newPosition, newRotation, newScale);
    }

    Transform& Transform::operator*=(const Transform& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    Transform Transform::Lerp(const Transform& target, F32 t) const
    {
        Vector3 lerpPosition = position.Lerp(target.position, t);
        Quaternion slerpRotation = Quaternion::Slerp(rotation, target.rotation, t);
        Vector3 lerpScale = scale.Lerp(target.scale, t);

        return Transform(lerpPosition, slerpRotation, lerpScale);
    }

    Transform Transform::Identity()
    {
        return Transform();
    }

    Transform Transform::Translation(const Vector3& position)
    {
        return Transform(position, Quaternion::Identity(), Vector3::One);
    }

    Transform Transform::Rotation(const Quaternion& rotation)
    {
        return Transform(Vector3::Zero, rotation, Vector3::One);
    }

    Transform Transform::Scale(const Vector3& scale)
    {
        return Transform(Vector3::Zero, Quaternion::Identity(), scale);
    }

    Transform Transform::TRS(const Vector3& translation, const Quaternion& rotation, const Vector3& scale)
    {
        return Transform(translation, rotation, scale);
    }
}