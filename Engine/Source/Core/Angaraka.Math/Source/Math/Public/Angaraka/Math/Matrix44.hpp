// ==================================================================================
// AngarakaMath/Public/Math/Matrix44.hpp - 4x4 Matrix
// ==================================================================================

#pragma once

#include <array>

namespace Angaraka::Math
{
    struct Quaternion;

    // ==================================================================================
    // Matrix4x4 - 4x4 Matrix (Column-major for OpenGL compatibility)
    // ==================================================================================

    struct Matrix4x4
    {
        // Column-major storage (OpenGL style)
        // [0 4 8  12]
        // [1 5 9  13]
        // [2 6 10 14]
        // [3 7 11 15]
        std::array<float, 16> m;

        // Constructors
        Matrix4x4();
        explicit Matrix4x4(float diagonal);
        Matrix4x4(float m00, float m01, float m02, float m03,
            float m10, float m11, float m12, float m13,
            float m20, float m21, float m22, float m23,
            float m30, float m31, float m32, float m33);

        // Element access (row, column)
        float& operator()(size_t row, size_t col) { return m[col * 4 + row]; }
        const float& operator()(size_t row, size_t col) const { return m[col * 4 + row]; }

        // Column access
        float* GetColumnPtr(size_t col) { return &m[col * 4]; }
        const float* GetColumnPtr(size_t col) const { return &m[col * 4]; }

        // Row/Column access
        Vector4 GetRow(size_t row) const;
        Vector4 GetColumn(size_t col) const;
        void SetRow(size_t row, const Vector4& vec);
        void SetColumn(size_t col, const Vector4& vec);

        // Matrix operations
        Matrix4x4 operator+(const Matrix4x4& rhs) const;
        Matrix4x4 operator-(const Matrix4x4& rhs) const;
        Matrix4x4 operator*(const Matrix4x4& rhs) const;
        Matrix4x4 operator*(float scalar) const;

        Matrix4x4& operator+=(const Matrix4x4& rhs);
        Matrix4x4& operator-=(const Matrix4x4& rhs);
        Matrix4x4& operator*=(const Matrix4x4& rhs);
        Matrix4x4& operator*=(float scalar);

        bool operator==(const Matrix4x4& rhs) const;
        bool operator!=(const Matrix4x4& rhs) const { return !(*this == rhs); }

        // Vector transformation
        Vector4 operator*(const Vector4& vec) const;
        Vector3 TransformPoint(const Vector3& point) const;      // Treats as position (w=1)
        Vector3 TransformDirection(const Vector3& direction) const; // Treats as direction (w=0)
        Vector3 TransformNormal(const Vector3& normal) const;    // Uses inverse transpose

        // Matrix properties
        Matrix4x4 Transposed() const;
        Matrix4x4 Inverted() const;
        float Determinant() const;
        void Transpose();
        bool Invert();

        // Transform extraction
        Vector3 GetTranslation() const;
        Vector3 GetScale() const;
        Quaternion GetRotation() const;
        void Decompose(Vector3& translation, Quaternion& rotation, Vector3& scale) const;

        // Static factory methods
        static Matrix4x4 Identity();
        static Matrix4x4 Zero();

        // Translation matrices
        static Matrix4x4 Translation(const Vector3& translation);
        static Matrix4x4 Translation(float x, float y, float z);

        // Rotation matrices
        static Matrix4x4 Rotation(const Vector3& axis, float angle);
        static Matrix4x4 RotationX(float angle);
        static Matrix4x4 RotationY(float angle);
        static Matrix4x4 RotationZ(float angle);
        static Matrix4x4 RotationEuler(float pitch, float yaw, float roll);
        static Matrix4x4 RotationEuler(const Vector3& eulerAngles);
        static Matrix4x4 RotationQuaternion(const Quaternion& q);

        // Scale matrices
        static Matrix4x4 Scale(const Vector3& scale);
        static Matrix4x4 Scale(float x, float y, float z);
        static Matrix4x4 Scale(float uniformScale);

        // Transform matrices
        static Matrix4x4 TRS(const Vector3& translation, const Quaternion& rotation, const Vector3& scale);
        static Matrix4x4 LookAt(const Vector3& eye, const Vector3& target, const Vector3& up);
        static Matrix4x4 LookTo(const Vector3& eye, const Vector3& direction, const Vector3& up);

        // Projection matrices
        static Matrix4x4 Perspective(float fovY, float aspect, float nearPlane, float farPlane);
        static Matrix4x4 PerspectiveReversedZ(float fovY, float aspect, float nearPlane, float farPlane);
        static Matrix4x4 Orthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);
        static Matrix4x4 OrthographicCentered(float width, float height, float nearPlane, float farPlane);
        static Matrix4x4 OrthographicOffCenter(const Vector3& center, float width, float height, float nearPlane, float farPlane);
        static Matrix4x4 Orthographic2D(float left, float right, float bottom, float top);
        static Matrix4x4 OrthographicScreen(float screenWidth, float screenHeight);
        static Matrix4x4 OrthographicAspect(float size, float aspectRatio, float nearPlane, float farPlane);
    };
}