module;

#include <Angaraka/Base.hpp>

export module Angaraka.Math.Transform;

import Angaraka.Math.Vector3;
import Angaraka.Math.Matrix4x4;
import Angaraka.Math.Quaternion;

namespace Angaraka::Math {

    // ==================================================================================
    // Transform - Combined Translation, Rotation, Scale
    // ==================================================================================
    export struct Transform
    {
        Vector3 position;
        Quaternion rotation;
        Vector3 scale;

        // Constructors
        Transform();
        Transform(const Vector3& position, const Quaternion& rotation = Quaternion::Identity(), const Vector3& scale = Vector3::One);

        // Transform operations
        Matrix4x4 ToMatrix() const;
        Transform Inverted() const;

        // Transform points and vectors
        Vector3 TransformPoint(const Vector3& point) const;
        Vector3 TransformDirection(const Vector3& direction) const;
        Vector3 InverseTransformPoint(const Vector3& point) const;
        Vector3 InverseTransformDirection(const Vector3& direction) const;

        // Combine transforms
        Transform operator*(const Transform& rhs) const;
        Transform& operator*=(const Transform& rhs);

        // Interpolation
        Transform Lerp(const Transform& target, F32 t) const;

        // Static factory methods
        static Transform Identity();
        static Transform Translation(const Vector3& position);
        static Transform Rotation(const Quaternion& rotation);
        static Transform Scale(const Vector3& scale);
        static Transform TRS(const Vector3& translation, const Quaternion& rotation, const Vector3& scale);
    };
} // namespace Angaraka::Math