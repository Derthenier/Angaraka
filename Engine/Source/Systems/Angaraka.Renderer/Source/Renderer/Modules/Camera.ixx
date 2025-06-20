// Angaraka.Camera.ixx
// Module for managing the camera (view and projection matrices).
module;

#include "Angaraka/GraphicsBase.hpp"

export module Angaraka.Camera;

namespace Angaraka {

    // Define movement directions for the camera
    export enum class Camera_Movement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,    // For flying camera (vertical movement)
        DOWN   // For flying camera (vertical movement)
    };

    export enum class Camera_Yaw_Pitch {
        YAW,
        PITCH
    };

    export class Camera {
    public:
        Camera();
        ~Camera();

        // Initializes the camera's properties.
        void Initialize(float fovY, float aspectRatio, float nearZ, float farZ);

        // Process input (now primarily for direct mouse look, and setting movement flags)
        void ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);

        // This method will now *set flags* rather than directly move the camera
        void SetMoveState(Camera_Movement direction, bool active);

        // Sets the camera's projection properties (field of view, aspect ratio, near/far planes).
        void SetLens(float fovY, float aspectRatio, float nearZ, float farZ);

        // Updates the camera's internal state (e.g., recalculates matrices, usually after input).
        void Update(float deltaTime);

        // DEBUG ONLY
        void Reset();
        // DEBUG ONLY

        // Accessors for the view and projection matrices.
        DirectX::XMMATRIX GetViewMatrix() const;
        DirectX::XMMATRIX GetProjectionMatrix() const;

    private:
        // Camera Attributes
        DirectX::XMVECTOR m_position;
        DirectX::XMVECTOR m_front;      // Normalized vector pointing forward from camera
        DirectX::XMVECTOR m_up;         // Normalized camera's up vector (relative to camera)
        DirectX::XMVECTOR m_right;      // Normalized camera's right vector
        DirectX::XMVECTOR m_worldUp;    // World's up vector (e.g., (0,1,0))

        // Euler Angles (in degrees)
        float m_yaw;
        float m_pitch;

        // Camera options
        float m_movementSpeed;
        float m_mouseSensitivity;
        float m_zoom; // FOV, acts like a zoom level

        // Projection properties
        float m_fov;
        float m_aspectRatio;
        float m_nearZ;
        float m_farZ;

        // Internal movement flags
        bool m_isMovingForward;
        bool m_isMovingBackward;
        bool m_isMovingLeft;
        bool m_isMovingRight;
        bool m_isMovingUp;
        bool m_isMovingDown;

        // Calculates the front, right, and up vectors from the camera's (updated) Euler Angles
        void UpdateCameraVectors();
    };

} // namespace Angaraka