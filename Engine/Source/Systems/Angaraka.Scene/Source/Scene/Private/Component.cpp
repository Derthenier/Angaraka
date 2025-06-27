module;

#include "Angaraka/Base.hpp"

module Angaraka.Scene.Component;

import Angaraka.Scene.Transform;

namespace Angaraka::SceneSystem {

    // Forward declare Entity methods we need
    // (Entity module will import Component module)
    SceneTransform& GetEntityTransform(const Entity* entity);
    bool IsEntityActive(const Entity* entity);

    // ================== Component Implementation ==================

    SceneTransform& Component::GetTransform() const {
        AGK_ASSERT(m_entity, "Component::GetTransform - Component not attached to entity!");
        return GetEntityTransform(m_entity);
    }

    bool Component::IsEnabled() const {
        // Component is enabled if:
        // 1. Component itself is enabled
        // 2. Entity is active
        // 3. Entity exists
        if (!m_entity) {
            return false;
        }

        return m_enabled && IsEntityActive(m_entity);
    }

    void Component::SetEnabled(bool enabled) {
        if (m_enabled == enabled) {
            return;
        }

        bool wasEnabled = IsEnabled();
        m_enabled = enabled;
        bool isNowEnabled = IsEnabled();

        // Handle enable/disable callbacks
        if (wasEnabled && !isNowEnabled) {
            OnDisable();
        }
        else if (!wasEnabled && isNowEnabled) {
            OnEnable();

            // If this is the first enable after attachment, call Start
            if (!m_hasStarted) {
                InternalStart();
            }
        }
    }

    // ================== Internal Lifecycle ==================

    void Component::InternalAwake(Entity* entity, Scene* scene) {
        AGK_ASSERT(entity, "Component::InternalAwake - Entity is null!");
        AGK_ASSERT(scene, "Component::InternalAwake - Scene is null!");
        AGK_ASSERT(!m_entity, "Component::InternalAwake - Component already attached to an entity!");

        m_entity = entity;
        m_scene = scene;

        // Call user's OnAwake
        try {
            OnAwake();
        }
        catch (const std::exception& e) {
            AGK_ERROR("Component::OnAwake exception: {}", e.what());
        }

        // If component is enabled and entity is active, call OnEnable
        if (IsEnabled()) {
            try {
                OnEnable();
            }
            catch (const std::exception& e) {
                AGK_ERROR("Component::OnEnable exception: {}", e.what());
            }
        }
    }

    void Component::InternalStart() {
        if (m_hasStarted) {
            return;
        }

        m_hasStarted = true;

        // Call user's OnStart
        try {
            OnStart();
        }
        catch (const std::exception& e) {
            AGK_ERROR("Component::OnStart exception: {}", e.what());
        }
    }

    void Component::InternalDestroy() {
        // If enabled, call OnDisable first
        if (IsEnabled()) {
            try {
                OnDisable();
            }
            catch (const std::exception& e) {
                AGK_ERROR("Component::OnDisable exception: {}", e.what());
            }
        }

        // Call user's OnDestroy
        try {
            OnDestroy();
        }
        catch (const std::exception& e) {
            AGK_ERROR("Component::OnDestroy exception: {}", e.what());
        }

        // Clear references
        m_entity = nullptr;
        m_scene = nullptr;
    }

} // namespace Angaraka::Scene