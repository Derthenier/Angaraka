module;

#include <Angaraka/Base.hpp>
#include <Angaraka/MathCore.hpp>

export module Angaraka.Scene.Transform;

namespace Angaraka::SceneSystem {

    /**
     * @brief Hierarchical transform component for scene graph
     *
     * This class wraps Math::Transform and adds scene graph functionality:
     * - Parent-child relationships
     * - Automatic dirty flag propagation
     * - Cached world transformation matrices
     * - Transform change notifications
     */
    export class SceneTransform {
    public:
        // Transform change callback
        using OnTransformChanged = std::function<void(SceneTransform*)>;

        SceneTransform();
        ~SceneTransform();

        // Non-copyable but moveable
        SceneTransform(const SceneTransform&) = delete;
        SceneTransform& operator=(const SceneTransform&) = delete;
        SceneTransform(SceneTransform&&) noexcept = default;
        SceneTransform& operator=(SceneTransform&&) noexcept = default;

        // ================== Local Transform ==================

        // Position
        void SetLocalPosition(const Math::Vector3& position);
        void SetLocalPosition(F32 x, F32 y, F32 z);
        const Math::Vector3& GetLocalPosition() const { return m_localTransform.position; }

        // Rotation
        void SetLocalRotation(const Math::Quaternion& rotation);
        void SetLocalRotationEuler(const Math::Vector3& eulerAngles);
        void SetLocalRotationEuler(F32 x, F32 y, F32 z);
        const Math::Quaternion& GetLocalRotation() const { return m_localTransform.rotation; }
        Math::Vector3 GetLocalRotationEuler() const;

        // Scale
        void SetLocalScale(const Math::Vector3& scale);
        void SetLocalScale(F32 uniformScale);
        void SetLocalScale(F32 x, F32 y, F32 z);
        const Math::Vector3& GetLocalScale() const { return m_localTransform.scale; }

        // Get the entire local transform
        const Math::Transform& GetLocalTransform() const { return m_localTransform; }
        void SetLocalTransform(const Math::Transform& transform);

        // ================== World Transform ==================

        // World position (computed from hierarchy)
        Math::Vector3 GetWorldPosition() const;
        void SetWorldPosition(const Math::Vector3& position);

        // World rotation (computed from hierarchy)
        Math::Quaternion GetWorldRotation() const;
        void SetWorldRotation(const Math::Quaternion& rotation);

        // World scale (computed from hierarchy)
        Math::Vector3 GetWorldScale() const;
        void SetWorldScale(const Math::Vector3& scale);

        // Get the entire world transform
        Math::Transform GetWorldTransform() const;

        // ================== Matrix Access ==================

        // Get matrices (cached for performance)
        const Math::Matrix4x4& GetLocalMatrix() const;
        const Math::Matrix4x4& GetWorldMatrix() const;
        const Math::Matrix4x4& GetWorldToLocalMatrix() const;

        // ================== Hierarchy ==================

        // Parent management
        void SetParent(SceneTransform* parent, bool worldPositionStays = true);
        SceneTransform* GetParent() const { return m_parent; }
        SceneTransform* GetRoot() const;
        bool IsChildOf(const SceneTransform* potentialParent) const;

        // Children access
        const std::vector<SceneTransform*>& GetChildren() const { return m_children; }
        SceneTransform* GetChild(size_t index) const;
        size_t GetChildCount() const { return m_children.size(); }
        SceneTransform* FindChild(const String& name) const;

        // Hierarchy queries
        void GetAllDescendants(std::vector<SceneTransform*>& outDescendants) const;
        size_t GetDepth() const;

        // ================== Transform Operations ==================

        // Look at target
        void LookAt(const Math::Vector3& worldTarget, const Math::Vector3& worldUp = Math::Vector3(0, 1, 0));
        void LookAt(const SceneTransform* target, const Math::Vector3& worldUp = Math::Vector3(0, 1, 0));

        // Movement helpers
        void Translate(const Math::Vector3& translation, bool worldSpace = true);
        void Rotate(const Math::Quaternion& rotation, bool worldSpace = true);
        void RotateEuler(const Math::Vector3& eulerAngles, bool worldSpace = true);
        void RotateAround(const Math::Vector3& point, const Math::Vector3& axis, F32 angle);

        // Direction vectors (in world space)
        Math::Vector3 GetForward() const;
        Math::Vector3 GetRight() const;
        Math::Vector3 GetUp() const;

        // ================== Coordinate Conversion ==================

        // Transform points between coordinate spaces
        Math::Vector3 TransformPoint(const Math::Vector3& localPoint) const;
        Math::Vector3 InverseTransformPoint(const Math::Vector3& worldPoint) const;

        // Transform directions (ignores position)
        Math::Vector3 TransformDirection(const Math::Vector3& localDirection) const;
        Math::Vector3 InverseTransformDirection(const Math::Vector3& worldDirection) const;

        // Transform vectors (ignores position and scale)
        Math::Vector3 TransformVector(const Math::Vector3& localVector) const;
        Math::Vector3 InverseTransformVector(const Math::Vector3& worldVector) const;

        // ================== Change Notification ==================

        // Register callback for transform changes
        void SetOnTransformChanged(OnTransformChanged callback) { m_onTransformChanged = callback; }

        // Manually mark as dirty (forces recalculation)
        void SetDirty();
        bool IsDirty() const { return m_localDirty || m_worldDirty; }

        // ================== Utility ==================

        // Name for debugging/searching
        void SetName(const String& name) { m_name = name; }
        const String& GetName() const { return m_name; }

    private:
        // Core transform data
        Math::Transform m_localTransform;

        // Cached matrices (mutable for lazy evaluation)
        mutable Math::Matrix4x4 m_localMatrix;
        mutable Math::Matrix4x4 m_worldMatrix;
        mutable Math::Matrix4x4 m_worldToLocalMatrix;

        // Dirty flags
        mutable bool m_localDirty = true;
        mutable bool m_worldDirty = true;

        // Hierarchy
        SceneTransform* m_parent = nullptr;
        std::vector<SceneTransform*> m_children;

        // Identity
        String m_name;

        // Callbacks
        OnTransformChanged m_onTransformChanged;

        // ================== Internal Methods ==================

        // Hierarchy management
        void AddChild(SceneTransform* child);
        void RemoveChild(SceneTransform* child);

        // Dirty flag propagation
        void MarkLocalDirty();
        void MarkWorldDirty();
        void PropagateWorldDirty();

        // Matrix updates
        void UpdateLocalMatrix() const;
        void UpdateWorldMatrix() const;
        void UpdateWorldToLocalMatrix() const;

        // Notify listeners
        void NotifyTransformChanged();
    };
}
