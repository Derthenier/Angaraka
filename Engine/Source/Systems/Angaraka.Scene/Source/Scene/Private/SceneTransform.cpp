module;

#include "Angaraka/Base.hpp"
#include <algorithm>
#include <cmath>

module Angaraka.Scene.Transform;

namespace Angaraka::SceneSystem {

    SceneTransform::SceneTransform()
        : m_localTransform(Math::Transform::Identity())
        , m_localMatrix(Math::Matrix4x4::Identity())
        , m_worldMatrix(Math::Matrix4x4::Identity())
        , m_worldToLocalMatrix(Math::Matrix4x4::Identity()) {
    }

    SceneTransform::~SceneTransform() {
        // Remove from parent if we have one
        if (m_parent) {
            m_parent->RemoveChild(this);
        }

        // Reparent all children to our parent (or null)
        for (auto* child : m_children) {
            child->m_parent = m_parent;
            if (m_parent) {
                m_parent->m_children.push_back(child);
            }
        }
        m_children.clear();
    }

    // ================== Local Transform ==================

    void SceneTransform::SetLocalPosition(const Math::Vector3& position) {
        if (m_localTransform.position != position) {
            m_localTransform.position = position;
            MarkLocalDirty();
        }
    }

    void SceneTransform::SetLocalPosition(F32 x, F32 y, F32 z) {
        SetLocalPosition(Math::Vector3(x, y, z));
    }

    void SceneTransform::SetLocalRotation(const Math::Quaternion& rotation) {
        if (m_localTransform.rotation != rotation) {
            m_localTransform.rotation = rotation;
            MarkLocalDirty();
        }
    }

    void SceneTransform::SetLocalRotationEuler(const Math::Vector3& eulerAngles) {
        SetLocalRotation(Math::Quaternion::FromEuler(eulerAngles));
    }

    void SceneTransform::SetLocalRotationEuler(F32 x, F32 y, F32 z) {
        SetLocalRotationEuler(Math::Vector3(x, y, z));
    }

    Math::Vector3 SceneTransform::GetLocalRotationEuler() const {
        return m_localTransform.rotation.ToEulerAngles();
    }

    void SceneTransform::SetLocalScale(const Math::Vector3& scale) {
        if (m_localTransform.scale != scale) {
            m_localTransform.scale = scale;
            MarkLocalDirty();
        }
    }

    void SceneTransform::SetLocalScale(F32 uniformScale) {
        SetLocalScale(Math::Vector3(uniformScale, uniformScale, uniformScale));
    }

    void SceneTransform::SetLocalScale(F32 x, F32 y, F32 z) {
        SetLocalScale(Math::Vector3(x, y, z));
    }

    void SceneTransform::SetLocalTransform(const Math::Transform& transform) {
        if (m_localTransform.position != transform.position ||
            m_localTransform.rotation != transform.rotation ||
            m_localTransform.scale != transform.scale) {
            m_localTransform = transform;
            MarkLocalDirty();
        }
    }

    // ================== World Transform ==================

    Math::Vector3 SceneTransform::GetWorldPosition() const {
        return GetWorldTransform().position;
    }

    void SceneTransform::SetWorldPosition(const Math::Vector3& position) {
        if (m_parent) {
            // Convert world position to local position
            Math::Vector3 localPos = m_parent->InverseTransformPoint(position);
            SetLocalPosition(localPos);
        }
        else {
            SetLocalPosition(position);
        }
    }

    Math::Quaternion SceneTransform::GetWorldRotation() const {
        return GetWorldTransform().rotation;
    }

    void SceneTransform::SetWorldRotation(const Math::Quaternion& rotation) {
        if (m_parent) {
            // Convert world rotation to local rotation
            Math::Quaternion parentWorldRot = m_parent->GetWorldRotation();
            Math::Quaternion localRot = parentWorldRot.Inverted() * rotation;
            SetLocalRotation(localRot);
        }
        else {
            SetLocalRotation(rotation);
        }
    }

    Math::Vector3 SceneTransform::GetWorldScale() const {
        return GetWorldTransform().scale;
    }

    void SceneTransform::SetWorldScale(const Math::Vector3& scale) {
        if (m_parent) {
            // Convert world scale to local scale
            Math::Vector3 parentWorldScale = m_parent->GetWorldScale();
            Math::Vector3 localScale(
                scale.x / parentWorldScale.x,
                scale.y / parentWorldScale.y,
                scale.z / parentWorldScale.z
            );
            SetLocalScale(localScale);
        }
        else {
            SetLocalScale(scale);
        }
    }

    Math::Transform SceneTransform::GetWorldTransform() const {
        if (m_worldDirty) {
            UpdateWorldMatrix();
        }

        // Decompose world matrix back to transform
        Math::Transform worldTransform;
        m_worldMatrix.Decompose(worldTransform.position, worldTransform.rotation, worldTransform.scale);
        return worldTransform;
    }

    // ================== Matrix Access ==================

    const Math::Matrix4x4& SceneTransform::GetLocalMatrix() const {
        if (m_localDirty) {
            UpdateLocalMatrix();
        }
        return m_localMatrix;
    }

    const Math::Matrix4x4& SceneTransform::GetWorldMatrix() const {
        if (m_worldDirty) {
            UpdateWorldMatrix();
        }
        return m_worldMatrix;
    }

    const Math::Matrix4x4& SceneTransform::GetWorldToLocalMatrix() const {
        if (m_worldDirty) {
            UpdateWorldMatrix();
            UpdateWorldToLocalMatrix();
        }
        return m_worldToLocalMatrix;
    }

    // ================== Hierarchy ==================

    void SceneTransform::SetParent(SceneTransform* parent, bool worldPositionStays) {
        if (parent == this) {
            AGK_ERROR("SceneTransform: Cannot set self as parent!");
            return;
        }

        if (parent && IsChildOf(parent)) {
            AGK_ERROR("SceneTransform: Cannot set child as parent (would create cycle)!");
            return;
        }

        // Store world transform if needed
        Math::Transform worldTransform;
        if (worldPositionStays) {
            worldTransform = GetWorldTransform();
        }

        // Remove from current parent
        if (m_parent) {
            m_parent->RemoveChild(this);
        }

        // Set new parent
        m_parent = parent;

        // Add to new parent
        if (m_parent) {
            m_parent->AddChild(this);
        }

        // Restore world transform if needed
        if (worldPositionStays) {
            if (m_parent) {
                // Convert world transform to local space of new parent
                Math::Transform parentWorld = m_parent->GetWorldTransform();
                Math::Transform parentWorldInv = parentWorld.Inverted();
                m_localTransform = parentWorldInv * worldTransform;
            }
            else {
                m_localTransform = worldTransform;
            }
            MarkLocalDirty();
        }
        else {
            MarkWorldDirty();
        }
    }

    SceneTransform* SceneTransform::GetRoot() const {
        const SceneTransform* root = this;
        while (root->m_parent) {
            root = root->m_parent;
        }
        return const_cast<SceneTransform*>(root);
    }

    bool SceneTransform::IsChildOf(const SceneTransform* potentialParent) const {
        const SceneTransform* current = m_parent;
        while (current) {
            if (current == potentialParent) {
                return true;
            }
            current = current->m_parent;
        }
        return false;
    }

    SceneTransform* SceneTransform::GetChild(size_t index) const {
        if (index >= m_children.size()) {
            return nullptr;
        }
        return m_children[index];
    }

    SceneTransform* SceneTransform::FindChild(const String& name) const {
        for (auto* child : m_children) {
            if (child->GetName() == name) {
                return child;
            }
        }
        return nullptr;
    }

    void SceneTransform::GetAllDescendants(std::vector<SceneTransform*>& outDescendants) const {
        for (auto* child : m_children) {
            outDescendants.push_back(child);
            child->GetAllDescendants(outDescendants);
        }
    }

    size_t SceneTransform::GetDepth() const {
        size_t depth = 0;
        const SceneTransform* current = m_parent;
        while (current) {
            depth++;
            current = current->m_parent;
        }
        return depth;
    }

    // ================== Transform Operations ==================

    void SceneTransform::LookAt(const Math::Vector3& worldTarget, const Math::Vector3& worldUp) {
        Math::Vector3 worldPos = GetWorldPosition();
        Math::Vector3 forward = (worldTarget - worldPos).Normalized();

        if (forward.LengthSquared() < 0.0001f) {
            return; // Too close to target
        }

        // Build rotation from forward and up vectors
        Math::Vector3 right = worldUp.Cross(forward).Normalized();
        Math::Vector3 up = forward.Cross(right);

        Math::Matrix4x4 lookMatrix;
        lookMatrix.SetColumn(0, Math::Vector4(right.x, right.y, right.z, 0));
        lookMatrix.SetColumn(1, Math::Vector4(up.x, up.y, up.z, 0));
        lookMatrix.SetColumn(2, Math::Vector4(forward.x, forward.y, forward.z, 0));
        lookMatrix.SetColumn(3, Math::Vector4(0, 0, 0, 1));

        Math::Quaternion worldRotation = lookMatrix.GetRotation();
        SetWorldRotation(worldRotation);
    }

    void SceneTransform::LookAt(const SceneTransform* target, const Math::Vector3& worldUp) {
        if (target) {
            LookAt(target->GetWorldPosition(), worldUp);
        }
    }

    void SceneTransform::Translate(const Math::Vector3& translation, bool worldSpace) {
        if (worldSpace) {
            SetWorldPosition(GetWorldPosition() + translation);
        }
        else {
            SetLocalPosition(GetLocalPosition() + translation);
        }
    }

    void SceneTransform::Rotate(const Math::Quaternion& rotation, bool worldSpace) {
        if (worldSpace) {
            SetWorldRotation(rotation * GetWorldRotation());
        }
        else {
            SetLocalRotation(GetLocalRotation() * rotation);
        }
    }

    void SceneTransform::RotateEuler(const Math::Vector3& eulerAngles, bool worldSpace) {
        Rotate(Math::Quaternion::FromEuler(eulerAngles), worldSpace);
    }

    void SceneTransform::RotateAround(const Math::Vector3& point, const Math::Vector3& axis, F32 angle) {
        Math::Vector3 worldPos = GetWorldPosition();
        Math::Vector3 offset = worldPos - point;

        // Rotate the offset
        Math::Quaternion rotation = Math::Quaternion::AngleAxis(axis, angle);
        offset = rotation.RotateVector(offset);

        // Update position and rotation
        SetWorldPosition(point + offset);
        Rotate(rotation, true);
    }

    Math::Vector3 SceneTransform::GetForward() const {
        return GetWorldRotation().RotateVector(Math::Vector3(0, 0, 1));
    }

    Math::Vector3 SceneTransform::GetRight() const {
        return GetWorldRotation().RotateVector(Math::Vector3(1, 0, 0));
    }

    Math::Vector3 SceneTransform::GetUp() const {
        return GetWorldRotation().RotateVector(Math::Vector3(0, 1, 0));
    }

    // ================== Coordinate Conversion ==================

    Math::Vector3 SceneTransform::TransformPoint(const Math::Vector3& localPoint) const {
        return GetWorldMatrix().TransformPoint(localPoint);
    }

    Math::Vector3 SceneTransform::InverseTransformPoint(const Math::Vector3& worldPoint) const {
        return GetWorldToLocalMatrix().TransformPoint(worldPoint);
    }

    Math::Vector3 SceneTransform::TransformDirection(const Math::Vector3& localDirection) const {
        return GetWorldMatrix().TransformDirection(localDirection);
    }

    Math::Vector3 SceneTransform::InverseTransformDirection(const Math::Vector3& worldDirection) const {
        return GetWorldToLocalMatrix().TransformDirection(worldDirection);
    }

    Math::Vector3 SceneTransform::TransformVector(const Math::Vector3& localVector) const {
        return GetWorldRotation().RotateVector(localVector);
    }

    Math::Vector3 SceneTransform::InverseTransformVector(const Math::Vector3& worldVector) const {
        return GetWorldRotation().Inverted().RotateVector(worldVector);
    }

    // ================== Change Notification ==================

    void SceneTransform::SetDirty() {
        MarkLocalDirty();
    }

    // ================== Internal Methods ==================

    void SceneTransform::AddChild(SceneTransform* child) {
        if (child && std::find(m_children.begin(), m_children.end(), child) == m_children.end()) {
            m_children.push_back(child);
        }
    }

    void SceneTransform::RemoveChild(SceneTransform* child) {
        auto it = std::find(m_children.begin(), m_children.end(), child);
        if (it != m_children.end()) {
            m_children.erase(it);
        }
    }

    void SceneTransform::MarkLocalDirty() {
        m_localDirty = true;
        MarkWorldDirty();
    }

    void SceneTransform::MarkWorldDirty() {
        m_worldDirty = true;
        PropagateWorldDirty();
        NotifyTransformChanged();
    }

    void SceneTransform::PropagateWorldDirty() {
        for (auto* child : m_children) {
            child->MarkWorldDirty();
        }
    }

    void SceneTransform::UpdateLocalMatrix() const {
        m_localMatrix = m_localTransform.ToMatrix();
        m_localDirty = false;
    }

    void SceneTransform::UpdateWorldMatrix() const {
        if (m_parent) {
            m_worldMatrix = m_parent->GetWorldMatrix() * GetLocalMatrix();
        }
        else {
            m_worldMatrix = GetLocalMatrix();
        }
        m_worldDirty = false;
    }

    void SceneTransform::UpdateWorldToLocalMatrix() const {
        m_worldToLocalMatrix = m_worldMatrix.Inverted();
    }

    void SceneTransform::NotifyTransformChanged() {
        if (m_onTransformChanged) {
            m_onTransformChanged(this);
        }
    }

} // namespace Angaraka::Scene