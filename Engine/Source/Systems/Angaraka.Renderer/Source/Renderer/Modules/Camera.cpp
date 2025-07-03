// Angaraka.Camera.cpp
// Implementation of Angaraka.Camera module.

module;

#include "Angaraka/GraphicsBase.hpp"

module Angaraka.Camera;

namespace Angaraka {

    namespace {
        // Default camera values
        const F32 YAW = 90.0f;    // Yaw is initialized to 90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right (positive X).
        // We want to point towards the negative Z axis for a typical forward direction.
        const F32 PITCH = 0.0f;
        const F32 SPEED = 5.0f;   // Units per second
        const F32 SENSITIVITY = 0.1f; // Mouse sensitivity
        const F32 ZOOM = 45.0f;    // Initial FOV (zoom)
    }

    Camera::Camera()
        : m_position(0.0f, 0.0f, -10.0f)     // Default starting position
        , m_worldUp(Math::Vector3::Up)       // World's up vector (0, 1, 0)
        , m_yaw(YAW)
        , m_pitch(PITCH)
        , m_movementSpeed(SPEED)
        , m_mouseSensitivity(SENSITIVITY)
        , m_zoom(ZOOM)
        , m_fov(Math::Util::DegreesToRadians(ZOOM))       // Initial FOV in radians
        , m_aspectRatio(1.0f)                // Default aspect ratio (will be set by Initialize)
        , m_nearZ(0.1f)                      // Default near plane
        , m_farZ(100.0f)                     // Default far plane
        , m_isMovingForward(false)
        , m_isMovingBackward(false)
        , m_isMovingLeft(false)
        , m_isMovingRight(false)
        , m_isMovingUp(false)
        , m_isMovingDown(false)
        , m_viewDirty(true)
        , m_projectionDirty(true)
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
        InvalidateViewCache(); // Mark view matrix as needing update
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
        bool moved = false;

        if (m_isMovingForward) {
            m_position += m_front * velocity;
            moved = true;
        }
        if (m_isMovingBackward) {
            m_position -= m_front * velocity;
            moved = true;
        }
        if (m_isMovingLeft) {
            m_position -= m_right * velocity;
            moved = true;
        }
        if (m_isMovingRight) {
            m_position += m_right * velocity;
            moved = true;
        }
        if (m_isMovingUp) {
            m_position += m_worldUp * velocity;
            moved = true;
        }
        if (m_isMovingDown) {
            m_position -= m_worldUp * velocity;
            moved = true;
        }

        // Invalidate view cache if position changed
        if (moved) {
            InvalidateViewCache();
        }
    }

    // DEBUG ONLY
    void Camera::Reset() {
        m_position = Math::Vector3(0.0f, 0.0f, -10.0f);  // Default starting position
        m_worldUp = Math::Vector3::Up;                   // World's up vector
        m_yaw = YAW;
        m_pitch = PITCH;
        m_movementSpeed = SPEED;
        m_mouseSensitivity = SENSITIVITY;
        m_zoom = ZOOM;
        SetLens(Math::Util::DegreesToRadians(45.0f), m_aspectRatio, 0.1f, 100.0f);
        UpdateCameraVectors();
        InvalidateViewCache();     // Both caches need refresh after reset
        InvalidateProjectionCache();
    }
    // DEBUG ONLY

    void Camera::SetLens(F32 fov, F32 aspectRatio, F32 nearZ, F32 farZ) {
        m_fov = fov;
        m_aspectRatio = aspectRatio;
        m_nearZ = nearZ;
        m_farZ = farZ;
        InvalidateProjectionCache(); // Mark projection matrix as needing update
    }

    Math::Matrix4x4 Camera::GetViewMatrix() const {
        // LookAt takes: eye position, target position, up vector
        // Target is calculated as position + front
        Math::Vector3 target = m_position + m_front;
        return Math::Matrix4x4::LookAt(m_position, target, m_up);
    }

    Math::Matrix4x4 Camera::GetProjectionMatrix() const {
        // Convert zoom (in degrees) to radians for the perspective matrix
        F32 fovRadians = Math::Util::DegreesToRadians(m_zoom);
        return Math::Matrix4x4::Perspective(fovRadians, m_aspectRatio, m_nearZ, m_farZ);
    }

    void Camera::UpdateCameraVectors() {
        // Calculate the new Front vector
        F32 yawRad = Math::Util::DegreesToRadians(m_yaw);
        F32 pitchRad = Math::Util::DegreesToRadians(m_pitch);

        Math::Vector3 front;
        front.x = std::cos(yawRad) * std::cos(pitchRad);
        front.y = std::sin(pitchRad);
        front.z = std::sin(yawRad) * std::cos(pitchRad);

        m_front = front.Normalized();

        // Recalculate the Right and Up vectors
        // Right = Cross(Front, WorldUp)
        m_right = m_front.Cross(m_worldUp).Normalized();

        // Up = Cross(Right, Front)
        m_up = m_right.Cross(m_front).Normalized();
    }

    void Camera::InvalidateViewCache() {
        m_viewDirty = true;
    }

    void Camera::InvalidateProjectionCache() {
        m_projectionDirty = true;
    }

    void Camera::UpdateViewMatrix() const {
        Math::Matrix4x4 view = GetViewMatrix();
        m_cachedViewMatrixDX = Math::MathConversion::ToDirectXMatrix(view);
        m_viewDirty = false;
    }

    void Camera::UpdateProjectionMatrix() const {
        Math::Matrix4x4 proj = GetProjectionMatrix();
        m_cachedProjectionMatrixDX = Math::MathConversion::ToDirectXMatrix(proj);
        m_projectionDirty = false;
    }

} // namespace Angaraka