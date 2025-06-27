module;

#include "Angaraka/Base.hpp"
#include <algorithm>

module Angaraka.Scene.Entity;

import Angaraka.Scene.Transform;
import Angaraka.Scene.Component;

namespace Angaraka::SceneSystem {

    // ================== Entity Implementation ==================

    Entity::Entity(EntityID id, Scene* scene)
        : m_id(id)
        , m_scene(scene) {
        AGK_ASSERT(scene, "Entity::Entity - Scene cannot be null!");
        AGK_ASSERT(id != InvalidEntityID, "Entity::Entity - Invalid entity ID!");

        // Set default name
        m_name = "Entity_" + std::to_string(id);

        // Set transform name to match entity
        m_transform.SetName(m_name);

        // Register transform change callback
        m_transform.SetOnTransformChanged([this](SceneTransform*) {
            NotifyTransformChanged();
            });
    }

    Entity::~Entity() {
        // Call OnDestroy if not already called
        if (m_hasStarted) {
            OnDestroy();
        }

        // Destroy all components in reverse order
        for (auto it = m_componentCache.rbegin(); it != m_componentCache.rend(); ++it) {
            (*it)->InternalDestroy();
        }
        m_components.clear();
        m_componentCache.clear();
    }

    // ================== Properties ==================

    void Entity::SetName(const String& name) {
        m_name = name;
        m_transform.SetName(name);
    }

    // ================== Active State ==================

    bool Entity::IsActive() const {
        if (!m_isActive) {
            return false;
        }

        // Check if all parents are active
        if (SceneTransform* parentTransform = m_transform.GetParent()) {
            if (Entity* parentEntity = GetEntityFromTransform(parentTransform)) {
                return parentEntity->IsActive();
            }
        }

        return true;
    }

    void Entity::SetActive(bool active) {
        if (m_isActive == active) {
            return;
        }

        bool wasActive = IsActive();
        m_isActive = active;
        bool isNowActive = IsActive();

        // Notify components of state change
        if (wasActive && !isNowActive) {
            // Became inactive - disable all components
            for (Component* component : m_componentCache) {
                if (component->m_enabled) {
                    component->OnDisable();
                }
            }
        }
        else if (!wasActive && isNowActive) {
            // Became active - enable all enabled components
            for (Component* component : m_componentCache) {
                if (component->m_enabled) {
                    component->OnEnable();

                    // Start components if entity has started
                    if (m_hasStarted && !component->HasStarted()) {
                        component->InternalStart();
                    }
                }
            }
        }

        // Recursively update children
        std::vector<SceneTransform*> childTransforms;
        m_transform.GetAllDescendants(childTransforms);

        for (SceneTransform* childTransform : childTransforms) {
            if (Entity* childEntity = GetEntityFromTransform(childTransform)) {
                // Trigger state update on children
                childEntity->SetActive(childEntity->m_isActive);
            }
        }
    }

    // ================== Component Management ==================

    Component* Entity::GetComponent(ComponentTypeID typeId) {
        auto it = m_components.find(typeId);
        if (it != m_components.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    // ================== Lifecycle ==================

    void Entity::Start() {
        if (m_hasStarted) {
            return;
        }

        m_hasStarted = true;

        // Start all enabled components
        if (IsActive()) {
            for (Component* component : m_componentCache) {
                if (component->IsEnabled() && !component->HasStarted()) {
                    component->InternalStart();
                }
            }
        }
    }

    void Entity::Update(F32 deltaTime) {
        if (!IsActive()) {
            return;
        }

        // Update all enabled components
        for (Component* component : m_componentCache) {
            if (component->IsEnabled()) {
                try {
                    component->OnUpdate(deltaTime);
                }
                catch (const std::exception& e) {
                    AGK_ERROR("Entity::Update - Component {} exception: {}",
                        component->GetTypeName(), e.what());
                }
            }
        }
    }

    void Entity::LateUpdate(F32 deltaTime) {
        if (!IsActive()) {
            return;
        }

        // Late update all enabled components
        for (Component* component : m_componentCache) {
            if (component->IsEnabled()) {
                try {
                    component->OnLateUpdate(deltaTime);
                }
                catch (const std::exception& e) {
                    AGK_ERROR("Entity::LateUpdate - Component {} exception: {}",
                        component->GetTypeName(), e.what());
                }
            }
        }
    }

    void Entity::FixedUpdate(F32 fixedDeltaTime) {
        if (!IsActive()) {
            return;
        }

        // Fixed update all enabled components
        for (Component* component : m_componentCache) {
            if (component->IsEnabled()) {
                try {
                    component->OnFixedUpdate(fixedDeltaTime);
                }
                catch (const std::exception& e) {
                    AGK_ERROR("Entity::FixedUpdate - Component {} exception: {}",
                        component->GetTypeName(), e.what());
                }
            }
        }
    }

    void Entity::OnDestroy() {
        // Notify all components
        for (Component* component : m_componentCache) {
            try {
                component->OnDestroy();
            }
            catch (const std::exception& e) {
                AGK_ERROR("Entity::OnDestroy - Component {} exception: {}",
                    component->GetTypeName(), e.what());
            }
        }
    }

    // ================== Messages ==================

    void Entity::SendMessage(U32 message, void* data) {
        for (Component* component : m_componentCache) {
            if (component->IsEnabled()) {
                try {
                    component->OnMessage(message, data);
                }
                catch (const std::exception& e) {
                    AGK_ERROR("Entity::SendMessage - Component {} exception: {}",
                        component->GetTypeName(), e.what());
                }
            }
        }
    }

    // ================== Private Helper Methods ==================

    void Entity::NotifyTransformChanged() {
        // Notify all components of transform change
        for (Component* component : m_componentCache) {
            if (component->IsEnabled()) {
                try {
                    component->OnTransformChanged();
                }
                catch (const std::exception& e) {
                    AGK_ERROR("Entity::NotifyTransformChanged - Component {} exception: {}",
                        component->GetTypeName(), e.what());
                }
            }
        }
    }

    template<typename T>
    T* Entity::GetComponentInChildrenRecursive(Entity* entity, bool includeInactive) {
        if (!entity) {
            return nullptr;
        }

        // Skip inactive entities unless requested
        if (!includeInactive && !entity->IsActive()) {
            return nullptr;
        }

        // Check this entity
        if (T* component = entity->GetComponent<T>()) {
            return component;
        }

        // Check children
        const auto& children = entity->GetTransform().GetChildren();
        for (SceneTransform* childTransform : children) {
            if (Entity* childEntity = GetEntityFromTransform(childTransform)) {
                if (T* component = GetComponentInChildrenRecursive<T>(childEntity, includeInactive)) {
                    return component;
                }
            }
        }

        return nullptr;
    }

    template<typename T>
    void Entity::GetComponentsInChildrenRecursive(Entity* entity,
        std::vector<T*>& outComponents, bool includeInactive) {
        if (!entity) {
            return;
        }

        // Skip inactive entities unless requested
        if (!includeInactive && !entity->IsActive()) {
            return;
        }

        // Add components from this entity
        if (T* component = entity->GetComponent<T>()) {
            outComponents.push_back(component);
        }

        // Recurse to children
        const auto& children = entity->GetTransform().GetChildren();
        for (SceneTransform* childTransform : children) {
            if (Entity* childEntity = GetEntityFromTransform(childTransform)) {
                GetComponentsInChildrenRecursive<T>(childEntity, outComponents, includeInactive);
            }
        }
    }

    Entity* Entity::GetEntityFromTransform(SceneTransform* transform) const {
        // This is a placeholder - the actual implementation would need
        // the Scene to look up entities by transform
        // For now, we'll return nullptr
        // TODO: Implement proper transform-to-entity lookup
        return nullptr;
    }

    // ================== Helper Functions for Component ==================

    SceneTransform& GetEntityTransform(const Entity* entity) {
        return const_cast<Entity*>(entity)->GetTransform();
    }

    bool IsEntityActive(const Entity* entity) {
        return entity->IsActive();
    }

} // namespace Angaraka::Scene