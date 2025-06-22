// Angaraka.Camera.cpp
// Implementation of Angaraka.Camera module.

module;

#include "Angaraka/GraphicsBase.hpp"

module Angaraka.Camera;

namespace Angaraka {

    namespace {
        // Helper to convert degrees to radians
        constexpr F32 ToRadians(F32 degrees) {
            return degrees * (DirectX::XM_PI / 180.0f);
        }

        // Default camera values
        const F32 YAW = 90.0f;    // Yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right (positive X).
                                    // We want to point towards the negative Z axis for a typical forward direction.
        const F32 PITCH = 0.0f;
        const F32 SPEED = 5.0f;   // Units per second
        const F32 SENSITIVITY = 0.1f; // Mouse sensitivity
        const F32 ZOOM = 45.0f;    // Initial FOV (zoom)
    }

    Camera::Camera()
        : m_position(0.0f, 0.0f, -10.0f) // Default starting position
        , m_worldUp(0.0f, 1.0f, 0.0f)    // World's up vector
        , m_yaw(YAW)
        , m_pitch(PITCH)
        , m_movementSpeed(SPEED)
        , m_mouseSensitivity(SENSITIVITY)
        , m_zoom(ZOOM)
        , m_fov(ToRadians(ZOOM))      // Initial FOV in radians
        , m_aspectRatio(1.0f)          // Default aspect ratio (will be set by Initialize)
        , m_nearZ(0.1f)                // Default near plane
        , m_farZ(100.0f)               // Default far plane
        , m_isMovingForward(false)
        , m_isMovingBackward(false)
        , m_isMovingLeft(false)
        , m_isMovingRight(false)
        , m_isMovingUp(false)
        , m_isMovingDown(false)
    {
        AGK_INFO("Camera: Constructor called. Initializing camera vectors...");
        UpdateCameraVectors(); // Calculate initial front, right, up vectors
    }

    Camera::~Camera() {
        AGK_INFO("Camera: Destructor called.");
    }

    void Camera::Initialize(F32 fov, F32 aspectRatio, F32 nearZ, F32 farZ) {
        SetLens(fov, aspectRatio, nearZ, farZ);
        UpdateCameraVectors(); // Ensure vectors are updated for initial state
        AGK_INFO("Camera: Initialized with FOV: {0}, AspectRatio: {1}, NearZ: {2}, FarZ: {3}",
            fov, aspectRatio, nearZ, farZ);
    }

    void Camera::ProcessMouseMovement(F32 xOffset, F32 yOffset, bool constrainPitch) {
        xOffset *= m_mouseSensitivity;
        yOffset *= m_mouseSensitivity;

        m_yaw += xOffset;
        m_pitch += yOffset;

        // Constrain pitch
        if (constrainPitch) {
            m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
        }

        UpdateCameraVectors(); // Recalculate Front, Right, Up vectors based on new Yaw/Pitch
    }

    // New method to set continuous movement states
    void Camera::SetMoveState(Camera_Movement direction, bool active) {
        switch (direction) {
        case Camera_Movement::FORWARD:   m_isMovingForward = active; break;
        case Camera_Movement::BACKWARD:  m_isMovingBackward = active; break;
        case Camera_Movement::LEFT:      m_isMovingLeft = active; break;
        case Camera_Movement::RIGHT:     m_isMovingRight = active; break;
        case Camera_Movement::UP:        m_isMovingUp = active; break;
        case Camera_Movement::DOWN:      m_isMovingDown = active; break;
        }
    }

    // New Update method to apply movement based on internal flags and deltaTime
    void Camera::Update(F32 deltaTime) {
        F32 velocity = m_movementSpeed * deltaTime;

        if (m_isMovingForward)  {
            m_position = DirectX::XMVectorAdd(m_position, DirectX::XMVectorScale(m_front, velocity)); 
        }
        if (m_isMovingBackward) {
            m_position = DirectX::XMVectorSubtract(m_position, DirectX::XMVectorScale(m_front, velocity)); 
        }
        if (m_isMovingLeft)     {
            m_position = DirectX::XMVectorSubtract(m_position, DirectX::XMVectorScale(m_right, velocity)); 
        }
        if (m_isMovingRight)    {
            m_position = DirectX::XMVectorAdd(m_position, DirectX::XMVectorScale(m_right, velocity)); 
        }
        if (m_isMovingUp)       {
            m_position = DirectX::XMVectorAdd(m_position, DirectX::XMVectorScale(m_worldUp, velocity)); 
        }
        if (m_isMovingDown)     {
            m_position = DirectX::XMVectorSubtract(m_position, DirectX::XMVectorScale(m_worldUp, velocity));
        }

        // No need to call UpdateCameraVectors here unless position affects orientation (it doesn't typically)
        // Only call GetViewMatrix() when needed, which relies on the updated m_position.
    }

    // DEBUG ONLY
    void Camera::Reset() {
        m_position = DirectX::XMVectorSet(0.0f, 0.0f, -10.0f, 1.0f); // Default starting position
        m_worldUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);    // World's up vector
        m_yaw = (YAW);
        m_pitch = (PITCH);
        m_movementSpeed = (SPEED);
        m_mouseSensitivity = (SENSITIVITY);
        m_zoom = (ZOOM);
        SetLens(DirectX::XM_PIDIV4, m_aspectRatio, 0.1f, 100.0f);
        UpdateCameraVectors();
    }
    // DEBUG ONLY

    void Camera::SetLens(F32 fov, F32 aspectRatio, F32 nearZ, F32 farZ) {
        m_fov = fov;
        m_aspectRatio = aspectRatio;
        m_nearZ = nearZ;
        m_farZ = farZ;
        // Projection matrix is recalculated in GetProjectionMatrix()
    }

    DirectX::XMMATRIX Camera::GetViewMatrix() const {
        return DirectX::XMMatrixLookAtLH(m_position, DirectX::XMVectorAdd(m_position, m_front), m_up);
    }

    DirectX::XMMATRIX Camera::GetProjectionMatrix() const {
        return DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(m_zoom), m_aspectRatio, m_nearZ, m_farZ);
    }

    void Camera::UpdateCameraVectors() {
        // Calculate the new Front vector
        DirectX::XMVECTOR front_vec;
        front_vec = DirectX::XMVectorSet(
            cos(DirectX::XMConvertToRadians(m_yaw)) * cos(DirectX::XMConvertToRadians(m_pitch)),
            sin(DirectX::XMConvertToRadians(m_pitch)),
            sin(DirectX::XMConvertToRadians(m_yaw)) * cos(DirectX::XMConvertToRadians(m_pitch)),
            0.0f
        );
        m_front = DirectX::XMVector3Normalize(front_vec);
        // Also recalculate the Right and Up vector
        m_right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(m_front, m_worldUp)); // Normalize the vectors, because their length gets closer to 0 the more you look up or down.
        m_up = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(m_right, m_front));
    }

} // namespace Angaraka