// ==================================================================================
// AngarakaMath/Private/Matrix44.cpp
// ==================================================================================

#include "Angaraka/MathCore.hpp"
#include "Angaraka/Math/Matrix44.hpp"
#include <cmath>

namespace Angaraka::Math
{
    // ==================================================================================
    // Matrix4x4 Implementation
    // ==================================================================================

    Matrix4x4::Matrix4x4()
    {
        m.fill(0.0f);
    }

    Matrix4x4::Matrix4x4(F32 diagonal)
    {
        m.fill(0.0f);
        m[0] = m[5] = m[10] = m[15] = diagonal;
    }

    Matrix4x4::Matrix4x4(F32 m00, F32 m01, F32 m02, F32 m03,
        F32 m10, F32 m11, F32 m12, F32 m13,
        F32 m20, F32 m21, F32 m22, F32 m23,
        F32 m30, F32 m31, F32 m32, F32 m33)
    {
        // Store in column-major order for OpenGL compatibility
        m[0] = m00; m[4] = m01; m[8] = m02; m[12] = m03;
        m[1] = m10; m[5] = m11; m[9] = m12; m[13] = m13;
        m[2] = m20; m[6] = m21; m[10] = m22; m[14] = m23;
        m[3] = m30; m[7] = m31; m[11] = m32; m[15] = m33;
    }

    Vector4 Matrix4x4::GetRow(size_t row) const
    {
        return { m[row], m[row + 4], m[row + 8], m[row + 12] };
    }

    Vector4 Matrix4x4::GetColumn(size_t col) const
    {
        size_t offset = col * 4;
        return { m[offset], m[offset + 1], m[offset + 2], m[offset + 3] };
    }

    void Matrix4x4::SetRow(size_t row, const Vector4& vec)
    {
        m[row] = vec.x;
        m[row + 4] = vec.y;
        m[row + 8] = vec.z;
        m[row + 12] = vec.w;
    }

    void Matrix4x4::SetColumn(size_t col, const Vector4& vec)
    {
        size_t offset = col * 4;
        m[offset] = vec.x;
        m[offset + 1] = vec.y;
        m[offset + 2] = vec.z;
        m[offset + 3] = vec.w;
    }

    // ==================================================================================
    // Matrix Operations
    // ==================================================================================

    Matrix4x4 Matrix4x4::operator+(const Matrix4x4& rhs) const
    {
        Matrix4x4 result;
        for (int i = 0; i < 16; ++i)
        {
            result.m[i] = m[i] + rhs.m[i];
        }
        return result;
    }

    Matrix4x4 Matrix4x4::operator-(const Matrix4x4& rhs) const
    {
        Matrix4x4 result;
        for (int i = 0; i < 16; ++i)
        {
            result.m[i] = m[i] - rhs.m[i];
        }
        return result;
    }

    Matrix4x4 Matrix4x4::operator*(const Matrix4x4& rhs) const
    {
        Matrix4x4 result;

        // Optimized matrix multiplication
        for (int col = 0; col < 4; ++col)
        {
            for (int row = 0; row < 4; ++row)
            {
                result(row, col) =
                    (*this)(row, 0) * rhs(0, col) +
                    (*this)(row, 1) * rhs(1, col) +
                    (*this)(row, 2) * rhs(2, col) +
                    (*this)(row, 3) * rhs(3, col);
            }
        }

        return result;
    }

    Matrix4x4 Matrix4x4::operator*(F32 scalar) const
    {
        Matrix4x4 result;
        for (int i = 0; i < 16; ++i)
        {
            result.m[i] = m[i] * scalar;
        }
        return result;
    }

    Matrix4x4& Matrix4x4::operator+=(const Matrix4x4& rhs)
    {
        for (int i = 0; i < 16; ++i)
        {
            m[i] += rhs.m[i];
        }
        return *this;
    }

    Matrix4x4& Matrix4x4::operator-=(const Matrix4x4& rhs)
    {
        for (int i = 0; i < 16; ++i)
        {
            m[i] -= rhs.m[i];
        }
        return *this;
    }

    Matrix4x4& Matrix4x4::operator*=(const Matrix4x4& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    Matrix4x4& Matrix4x4::operator*=(F32 scalar)
    {
        for (int i = 0; i < 16; ++i)
        {
            m[i] *= scalar;
        }
        return *this;
    }

    bool Matrix4x4::operator==(const Matrix4x4& rhs) const
    {
        for (int i = 0; i < 16; ++i)
        {
            if (!IsNearlyEqual(m[i], rhs.m[i]))
                return false;
        }
        return true;
    }

    // ==================================================================================
    // Vector Transformation
    // ==================================================================================

    Vector4 Matrix4x4::operator*(const Vector4& vec) const
    {
        return {
            m[0] * vec.x + m[4] * vec.y + m[8] * vec.z + m[12] * vec.w,
            m[1] * vec.x + m[5] * vec.y + m[9] * vec.z + m[13] * vec.w,
            m[2] * vec.x + m[6] * vec.y + m[10] * vec.z + m[14] * vec.w,
            m[3] * vec.x + m[7] * vec.y + m[11] * vec.z + m[15] * vec.w
        };
    }

    Vector3 Matrix4x4::TransformPoint(const Vector3& point) const
    {
        Vector4 result = *this * Vector4(point, 1.0f);
        if (IsNearlyZero(result.w))
            return result.xyz();
        return result.xyz() / result.w;
    }

    Vector3 Matrix4x4::TransformDirection(const Vector3& direction) const
    {
        Vector4 result = *this * Vector4(direction, 0.0f);
        return result.xyz();
    }

    Vector3 Matrix4x4::TransformNormal(const Vector3& normal) const
    {
        // For normals, we need to use the inverse transpose to handle non-uniform scaling
        Matrix4x4 invTranspose = Inverted().Transposed();
        return invTranspose.TransformDirection(normal).Normalized();
    }

    // ==================================================================================
    // Matrix Properties
    // ==================================================================================

    Matrix4x4 Matrix4x4::Transposed() const
    {
        return Matrix4x4(
            m[0], m[1], m[2], m[3],
            m[4], m[5], m[6], m[7],
            m[8], m[9], m[10], m[11],
            m[12], m[13], m[14], m[15]
        );
    }

    void Matrix4x4::Transpose()
    {
        *this = Transposed();
    }

    F32 Matrix4x4::Determinant() const
    {
        F32 det =
            m[12] * m[9] * m[6] * m[3] - m[8] * m[13] * m[6] * m[3] - m[12] * m[5] * m[10] * m[3] + m[4] * m[13] * m[10] * m[3] +
            m[8] * m[5] * m[14] * m[3] - m[4] * m[9] * m[14] * m[3] - m[12] * m[9] * m[2] * m[7] + m[8] * m[13] * m[2] * m[7] +
            m[12] * m[1] * m[10] * m[7] - m[0] * m[13] * m[10] * m[7] - m[8] * m[1] * m[14] * m[7] + m[0] * m[9] * m[14] * m[7] +
            m[12] * m[5] * m[2] * m[11] - m[4] * m[13] * m[2] * m[11] - m[12] * m[1] * m[6] * m[11] + m[0] * m[13] * m[6] * m[11] +
            m[4] * m[1] * m[14] * m[11] - m[0] * m[5] * m[14] * m[11] - m[8] * m[5] * m[2] * m[15] + m[4] * m[9] * m[2] * m[15] +
            m[8] * m[1] * m[6] * m[15] - m[0] * m[9] * m[6] * m[15] - m[4] * m[1] * m[10] * m[15] + m[0] * m[5] * m[10] * m[15];

        return det;
    }

    Matrix4x4 Matrix4x4::Inverted() const
    {
        Matrix4x4 inv;

        inv.m[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] +
            m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];

        inv.m[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] -
            m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];

        inv.m[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] +
            m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];

        inv.m[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] -
            m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];

        inv.m[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] -
            m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];

        inv.m[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] +
            m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];

        inv.m[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] -
            m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];

        inv.m[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] +
            m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];

        inv.m[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] +
            m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];

        inv.m[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] -
            m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];

        inv.m[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] +
            m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];

        inv.m[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] -
            m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];

        inv.m[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] -
            m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];

        inv.m[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] +
            m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];

        inv.m[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] -
            m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];

        inv.m[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] +
            m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

        F32 det = Determinant();

        if (IsNearlyZero(det))
        {
            // Matrix is not invertible, return identity
            return Identity();
        }

        F32 invDet = 1.0f / det;
        for (int i = 0; i < 16; ++i)
        {
            inv.m[i] *= invDet;
        }

        return inv;
    }

    bool Matrix4x4::Invert()
    {
        Matrix4x4 inv = Inverted();
        if (inv == Identity() && *this != Identity())
        {
            return false; // Matrix was not invertible
        }

        *this = inv;
        return true;
    }

    // ==================================================================================
    // Transform Extraction
    // ==================================================================================

    Vector3 Matrix4x4::GetTranslation() const
    {
        return { m[12], m[13], m[14] };
    }

    Vector3 Matrix4x4::GetScale() const
    {
        Vector3 scaleX = Vector3(m[0], m[1], m[2]);
        Vector3 scaleY = Vector3(m[4], m[5], m[6]);
        Vector3 scaleZ = Vector3(m[8], m[9], m[10]);

        return { scaleX.Length(), scaleY.Length(), scaleZ.Length() };
    }

    Quaternion Matrix4x4::GetRotation() const
    {
        // Remove scale from the matrix first
        Vector3 scale = GetScale();

        Matrix4x4 rotMatrix = *this;
        rotMatrix(0, 0) /= scale.x; rotMatrix(0, 1) /= scale.y; rotMatrix(0, 2) /= scale.z;
        rotMatrix(1, 0) /= scale.x; rotMatrix(1, 1) /= scale.y; rotMatrix(1, 2) /= scale.z;
        rotMatrix(2, 0) /= scale.x; rotMatrix(2, 1) /= scale.y; rotMatrix(2, 2) /= scale.z;

        // Convert rotation matrix to quaternion
        F32 trace = rotMatrix(0, 0) + rotMatrix(1, 1) + rotMatrix(2, 2);

        if (trace > 0.0f)
        {
            F32 s = Sqrt(trace + 1.0f) * 2.0f; // s = 4 * qw
            F32 w = 0.25f * s;
            F32 x = (rotMatrix(2, 1) - rotMatrix(1, 2)) / s;
            F32 y = (rotMatrix(0, 2) - rotMatrix(2, 0)) / s;
            F32 z = (rotMatrix(1, 0) - rotMatrix(0, 1)) / s;
            return { x, y, z, w };
        }
        else if (rotMatrix(0, 0) > rotMatrix(1, 1) && rotMatrix(0, 0) > rotMatrix(2, 2))
        {
            F32 s = Sqrt(1.0f + rotMatrix(0, 0) - rotMatrix(1, 1) - rotMatrix(2, 2)) * 2.0f; // s = 4 * qx
            F32 w = (rotMatrix(2, 1) - rotMatrix(1, 2)) / s;
            F32 x = 0.25f * s;
            F32 y = (rotMatrix(0, 1) + rotMatrix(1, 0)) / s;
            F32 z = (rotMatrix(0, 2) + rotMatrix(2, 0)) / s;
            return { x, y, z, w };
        }
        else if (rotMatrix(1, 1) > rotMatrix(2, 2))
        {
            F32 s = Sqrt(1.0f + rotMatrix(1, 1) - rotMatrix(0, 0) - rotMatrix(2, 2)) * 2.0f; // s = 4 * qy
            F32 w = (rotMatrix(0, 2) - rotMatrix(2, 0)) / s;
            F32 x = (rotMatrix(0, 1) + rotMatrix(1, 0)) / s;
            F32 y = 0.25f * s;
            F32 z = (rotMatrix(1, 2) + rotMatrix(2, 1)) / s;
            return { x, y, z, w };
        }
        else
        {
            F32 s = Sqrt(1.0f + rotMatrix(2, 2) - rotMatrix(0, 0) - rotMatrix(1, 1)) * 2.0f; // s = 4 * qz
            F32 w = (rotMatrix(1, 0) - rotMatrix(0, 1)) / s;
            F32 x = (rotMatrix(0, 2) + rotMatrix(2, 0)) / s;
            F32 y = (rotMatrix(1, 2) + rotMatrix(2, 1)) / s;
            F32 z = 0.25f * s;
            return { x, y, z, w };
        }
    }

    void Matrix4x4::Decompose(Vector3& translation, Quaternion& rotation, Vector3& scale) const
    {
        translation = GetTranslation();
        scale = GetScale();
        rotation = GetRotation();
    }

    // ==================================================================================
    // Static Factory Methods
    // ==================================================================================

    Matrix4x4 Matrix4x4::Identity()
    {
        return Matrix4x4(1.0f);
    }

    Matrix4x4 Matrix4x4::Zero()
    {
        return Matrix4x4();
    }

    Matrix4x4 Matrix4x4::Translation(const Vector3& translation)
    {
        Matrix4x4 result = Identity();
        result(0, 3) = translation.x;
        result(1, 3) = translation.y;
        result(2, 3) = translation.z;
        return result;
    }

    Matrix4x4 Matrix4x4::Translation(F32 x, F32 y, F32 z)
    {
        return Translation(Vector3(x, y, z));
    }

    Matrix4x4 Matrix4x4::Rotation(const Vector3& axis, F32 angle)
    {
        Vector3 normalizedAxis = axis.Normalized();
        F32 c = std::cos(angle);
        F32 s = std::sin(angle);
        F32 oneMinusC = 1.0f - c;

        F32 x = normalizedAxis.x;
        F32 y = normalizedAxis.y;
        F32 z = normalizedAxis.z;

        return Matrix4x4(
            c + x * x * oneMinusC, x * y * oneMinusC - z * s, x * z * oneMinusC + y * s, 0.0f,
            y * x * oneMinusC + z * s, c + y * y * oneMinusC, y * z * oneMinusC - x * s, 0.0f,
            z * x * oneMinusC - y * s, z * y * oneMinusC + x * s, c + z * z * oneMinusC, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );
    }

    Matrix4x4 Matrix4x4::RotationX(F32 angle)
    {
        F32 c = std::cos(angle);
        F32 s = std::sin(angle);

        return Matrix4x4(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, c, -s, 0.0f,
            0.0f, s, c, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );
    }

    Matrix4x4 Matrix4x4::RotationY(F32 angle)
    {
        F32 c = std::cos(angle);
        F32 s = std::sin(angle);

        return Matrix4x4(
            c, 0.0f, s, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            -s, 0.0f, c, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );
    }

    Matrix4x4 Matrix4x4::RotationZ(F32 angle)
    {
        F32 c = std::cos(angle);
        F32 s = std::sin(angle);

        return Matrix4x4(
            c, -s, 0.0f, 0.0f,
            s, c, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );
    }

    Matrix4x4 Matrix4x4::RotationEuler(F32 pitch, F32 yaw, F32 roll)
    {
        return RotationZ(roll) * RotationY(yaw) * RotationX(pitch);
    }

    Matrix4x4 Matrix4x4::RotationEuler(const Vector3& eulerAngles)
    {
        return RotationEuler(eulerAngles.x, eulerAngles.y, eulerAngles.z);
    }

    Matrix4x4 Matrix4x4::RotationQuaternion(const Quaternion& q)
    {
        return q.ToMatrix();
    }

    Matrix4x4 Matrix4x4::Scale(const Vector3& scale)
    {
        Matrix4x4 result;
        result(0, 0) = scale.x;
        result(1, 1) = scale.y;
        result(2, 2) = scale.z;
        result(3, 3) = 1.0f;
        return result;
    }

    Matrix4x4 Matrix4x4::Scale(F32 x, F32 y, F32 z)
    {
        return Scale(Vector3(x, y, z));
    }

    Matrix4x4 Matrix4x4::Scale(F32 uniformScale)
    {
        return Scale(Vector3(uniformScale));
    }

    Matrix4x4 Matrix4x4::TRS(const Vector3& translation, const Quaternion& rotation, const Vector3& scale)
    {
        Matrix4x4 t = Translation(translation);
        Matrix4x4 r = rotation.ToMatrix();
        Matrix4x4 s = Scale(scale);

        return t * r * s;
    }

    Matrix4x4 Matrix4x4::LookAt(const Vector3& eye, const Vector3& target, const Vector3& up)
    {
        Vector3 forward = (target - eye).Normalized();
        return LookTo(eye, forward, up);
    }

    Matrix4x4 Matrix4x4::LookTo(const Vector3& eye, const Vector3& direction, const Vector3& up)
    {
        Vector3 forward = direction.Normalized();
        Vector3 right = forward.Cross(up.Normalized()).Normalized();
        Vector3 newUp = right.Cross(forward);

        Matrix4x4 result = Identity();
        result(0, 0) = right.x;
        result(0, 1) = right.y;
        result(0, 2) = right.z;
        result(1, 0) = newUp.x;
        result(1, 1) = newUp.y;
        result(1, 2) = newUp.z;
        result(2, 0) = -forward.x;
        result(2, 1) = -forward.y;
        result(2, 2) = -forward.z;
        result(0, 3) = -right.Dot(eye);
        result(1, 3) = -newUp.Dot(eye);
        result(2, 3) = forward.Dot(eye);

        return result;
    }

    Matrix4x4 Matrix4x4::Perspective(F32 fovY, F32 aspect, F32 nearPlane, F32 farPlane)
    {
        F32 tanHalfFovy = std::tan(fovY * 0.5f);

        Matrix4x4 result;
        result(0, 0) = 1.0f / (aspect * tanHalfFovy);
        result(1, 1) = 1.0f / tanHalfFovy;
        result(2, 2) = -(farPlane + nearPlane) / (farPlane - nearPlane);
        result(2, 3) = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
        result(3, 2) = -1.0f;

        return result;
    }

    Matrix4x4 Matrix4x4::PerspectiveReversedZ(F32 fovY, F32 aspect, F32 nearPlane, F32 farPlane)
    {
        F32 tanHalfFovy = std::tan(fovY * 0.5f);

        Matrix4x4 result;
        result(0, 0) = 1.0f / (aspect * tanHalfFovy);
        result(1, 1) = 1.0f / tanHalfFovy;
        result(2, 2) = nearPlane / (nearPlane - farPlane);
        result(2, 3) = (farPlane * nearPlane) / (nearPlane - farPlane);
        result(3, 2) = -1.0f;

        return result;
    }

    Matrix4x4 Matrix4x4::Orthographic(F32 left, F32 right, F32 bottom, F32 top, F32 nearPlane, F32 farPlane)
    {
        Matrix4x4 result;

        // Calculate scaling factors
        F32 width = right - left;
        F32 height = top - bottom;
        F32 depth = farPlane - nearPlane;

        // Validate input parameters
        if (IsNearlyZero(width) || IsNearlyZero(height) || IsNearlyZero(depth))
        {
            // Return identity matrix for invalid parameters
            return Identity();
        }

        // Set diagonal elements (scaling)
        result(0, 0) = 2.0f / width;          // Scale X
        result(1, 1) = 2.0f / height;         // Scale Y
        result(2, 2) = -2.0f / depth;         // Scale Z (negative for OpenGL)
        result(3, 3) = 1.0f;                  // Homogeneous coordinate

        // Set translation elements
        result(0, 3) = -(right + left) / width;       // Translate X to center
        result(1, 3) = -(top + bottom) / height;      // Translate Y to center
        result(2, 3) = -(farPlane + nearPlane) / depth; // Translate Z to center

        // All other elements are already zero (from default constructor)

        return result;
    }

    Matrix4x4 Matrix4x4::OrthographicCentered(F32 width, F32 height, F32 nearPlane, F32 farPlane)
    {
        // Calculate symmetric bounds around origin
        F32 halfWidth = width * 0.5f;
        F32 halfHeight = height * 0.5f;

        // Use the general orthographic function with symmetric bounds
        return Orthographic(-halfWidth, halfWidth, -halfHeight, halfHeight, nearPlane, farPlane);
    }

    // Additional orthographic utility functions

    // Create orthographic matrix from center point and dimensions
    Matrix4x4 Matrix4x4::OrthographicOffCenter(const Vector3& center, F32 width, F32 height, F32 nearPlane, F32 farPlane)
    {
        F32 halfWidth = width * 0.5f;
        F32 halfHeight = height * 0.5f;

        return Orthographic(
            center.x - halfWidth,  // left
            center.x + halfWidth,  // right
            center.y - halfHeight, // bottom
            center.y + halfHeight, // top
            nearPlane,
            farPlane
        );
    }

    // Create orthographic matrix for 2D rendering (Z from -1 to 1)
    Matrix4x4 Matrix4x4::Orthographic2D(F32 left, F32 right, F32 bottom, F32 top)
    {
        return Orthographic(left, right, bottom, top, -1.0f, 1.0f);
    }

    // Create orthographic matrix for screen/pixel coordinates
    Matrix4x4 Matrix4x4::OrthographicScreen(F32 screenWidth, F32 screenHeight)
    {
        // Maps screen coordinates (0,0) at top-left to (-1,-1) at bottom-left in NDC
        return Orthographic(0.0f, screenWidth, screenHeight, 0.0f, -1.0f, 1.0f);
    }

    // Create orthographic matrix with aspect ratio preservation
    Matrix4x4 Matrix4x4::OrthographicAspect(F32 size, F32 aspectRatio, F32 nearPlane, F32 farPlane)
    {
        F32 width, height;

        if (aspectRatio >= 1.0f)
        {
            // Wide aspect ratio
            width = size * aspectRatio;
            height = size;
        }
        else
        {
            // Tall aspect ratio
            width = size;
            height = size / aspectRatio;
        }

        return OrthographicCentered(width, height, nearPlane, farPlane);
    }
}