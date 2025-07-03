module;

#include <Angaraka/Base.hpp>
#include <array>
export module Angaraka.Math.Matrix4x4;

import Angaraka.Math.Vector3;
import Angaraka.Math.Vector4;

namespace Angaraka::Math {

    export struct Quaternion; // Forward declaration

    // ==================================================================================
    // Matrix4x4 - 4x4 Matrix (Column-major for OpenGL compatibility)
    // ==================================================================================
    export struct Matrix4x4
    {
        // Column-major storage (OpenGL style)
        // [0 4 8  12]
        // [1 5 9  13]
        // [2 6 10 14]
        // [3 7 11 15]
        std::array<F32, 16> m;

        // Constructors
        Matrix4x4();
        explicit Matrix4x4(F32 diagonal);
        Matrix4x4(F32 m00, F32 m01, F32 m02, F32 m03,
            F32 m10, F32 m11, F32 m12, F32 m13,
            F32 m20, F32 m21, F32 m22, F32 m23,
            F32 m30, F32 m31, F32 m32, F32 m33);

        // Element access (row, column)
        inline F32& operator()(size_t row, size_t col) { return m[col * 4 + row]; }
        inline const F32& operator()(size_t row, size_t col) const { return m[col * 4 + row]; }

        // Column access
        inline F32* GetColumnPtr(size_t col) { return &m[col * 4]; }
        inline const F32* GetColumnPtr(size_t col) const { return &m[col * 4]; }

        // Row/Column access
        Vector4 GetRow(size_t row) const;
        Vector4 GetColumn(size_t col) const;
        void SetRow(size_t row, const Vector4& vec);
        void SetColumn(size_t col, const Vector4& vec);

        // Matrix operations
        Matrix4x4 operator+(const Matrix4x4& rhs) const;
        Matrix4x4 operator-(const Matrix4x4& rhs) const;
        Matrix4x4 operator*(const Matrix4x4& rhs) const;
        Matrix4x4 operator*(F32 scalar) const;

        Matrix4x4& operator+=(const Matrix4x4& rhs);
        Matrix4x4& operator-=(const Matrix4x4& rhs);
        Matrix4x4& operator*=(const Matrix4x4& rhs);
        Matrix4x4& operator*=(F32 scalar);

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
        F32 Determinant() const;
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
        static Matrix4x4 Translation(F32 x, F32 y, F32 z);

        // Rotation matrices
        static Matrix4x4 Rotation(const Vector3& axis, F32 angle);
        static Matrix4x4 RotationX(F32 angle);
        static Matrix4x4 RotationY(F32 angle);
        static Matrix4x4 RotationZ(F32 angle);
        static Matrix4x4 RotationEuler(F32 pitch, F32 yaw, F32 roll);
        static Matrix4x4 RotationEuler(const Vector3& eulerAngles);
        static Matrix4x4 RotationQuaternion(const Quaternion& q);

        // Scale matrices
        static Matrix4x4 Scale(const Vector3& scale);
        static Matrix4x4 Scale(F32 x, F32 y, F32 z);
        static Matrix4x4 Scale(F32 uniformScale);

        // Transform matrices
        static Matrix4x4 TRS(const Vector3& translation, const Quaternion& rotation, const Vector3& scale);
        static Matrix4x4 LookAt(const Vector3& eye, const Vector3& target, const Vector3& up);
        static Matrix4x4 LookTo(const Vector3& eye, const Vector3& direction, const Vector3& up);

        // Projection matrices
        static Matrix4x4 Perspective(F32 fovY, F32 aspect, F32 nearPlane, F32 farPlane);
        static Matrix4x4 PerspectiveReversedZ(F32 fovY, F32 aspect, F32 nearPlane, F32 farPlane);
        static Matrix4x4 Orthographic(F32 left, F32 right, F32 bottom, F32 top, F32 nearPlane, F32 farPlane);
        static Matrix4x4 OrthographicCentered(F32 width, F32 height, F32 nearPlane, F32 farPlane);
        static Matrix4x4 OrthographicOffCenter(const Vector3& center, F32 width, F32 height, F32 nearPlane, F32 farPlane);
        static Matrix4x4 Orthographic2D(F32 left, F32 right, F32 bottom, F32 top);
        static Matrix4x4 OrthographicScreen(F32 screenWidth, F32 screenHeight);
        static Matrix4x4 OrthographicAspect(F32 size, F32 aspectRatio, F32 nearPlane, F32 farPlane);
    };
} // namespace Angaraka::Math