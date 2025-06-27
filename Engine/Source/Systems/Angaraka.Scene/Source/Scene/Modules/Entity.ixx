module;

#include "Angaraka/Base.hpp"
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <typeindex>

export module Angaraka.Scene.Entity;

import Angaraka.Scene.Transform;
import Angaraka.Scene.Component;

namespace Angaraka::SceneSystem {

    // Forward declarations
    export class Scene;

    /**
     * @brief Unique identifier for entities
     */
    export using EntityID = U64;

    /**
     * @brief Invalid entity ID constant
     */
    export constexpr EntityID InvalidEntityID = 0;

    /**
     * @brief Entity tags for grouping and identification
     */
    export using EntityTag = U32;

    /**
     * @brief Entity represents a game object in the scene
     *
     * Entities are containers for components and have a transform.
     * They follow a hierarchical structure through their transforms.
     *
     * Lifecycle:
     * 1. Constructor - Entity created
     * 2. Start - Called before first update
     * 3. Update - Called every frame while active
     * 4. LateUpdate - Called after all updates
     * 5. OnDestroy - Called before destruction
     * 6. Destructor - Entity destroyed
     */
    export class Entity {
    public:
        /**
         * @brief Construct entity with ID and scene reference
         * @param id Unique identifier for this entity
         * @param scene Scene that owns this entity
         */
        Entity(EntityID id, Scene* scene);
        ~Entity();

        // Non-copyable, non-moveable
        Entity(const Entity&) = delete;
        Entity& operator=(const Entity&) = delete;
        Entity(Entity&&) = delete;
        Entity& operator=(Entity&&) = delete;

        // ================== Properties ==================

        /**
         * @brief Get the unique ID of this entity
         */
        EntityID GetID() const { return m_id; }

        /**
         * @brief Get/set the entity name for debugging
         */
        const String& GetName() const { return m_name; }
        void SetName(const String& name);

        /**
         * @brief Get the scene this entity belongs to
         */
        Scene* GetScene() const { return m_scene; }

        // ================== Active State ==================

        /**
         * @brief Check if entity is active in scene
         * @return True if active and all parents are active
         */
        bool IsActive() const;

        /**
         * @brief Check if entity is active in hierarchy
         * @return True if this entity is active (ignores parents)
         */
        bool IsActiveSelf() const { return m_isActive; }

        /**
         * @brief Set active state of this entity
         * @param active New active state
         */
        void SetActive(bool active);

        // ================== Transform ==================

        /**
         * @brief Get the transform component (always exists)
         */
        SceneTransform& GetTransform() { return m_transform; }
        const SceneTransform& GetTransform() const { return m_transform; }

        // ================== Tags & Layers ==================

        /**
         * @brief Get/set entity tag for grouping
         */
        EntityTag GetTag() const { return m_tag; }
        void SetTag(EntityTag tag) { m_tag = tag; }

        /**
         * @brief Get/set rendering layer
         */
        U32 GetLayer() const { return m_layer; }
        void SetLayer(U32 layer) { m_layer = layer; }

        // ================== Component Management ==================

        /**
         * @brief Add a component of type T
         * @tparam T Component type (must derive from Component)
         * @tparam Args Constructor argument types
         * @param args Constructor arguments
         * @return Pointer to the created component
         */
        template<typename T, typename... Args>
        T* AddComponent(Args&&... args) {
            static_assert(std::is_base_of_v<Component, T>,
                "T must derive from Component");

            // Check if component already exists
            ComponentTypeID typeId = GetComponentTypeID<T>();
            if (m_components.find(typeId) != m_components.end()) {
                AGK_WARN("Entity::AddComponent - Component of type {} already exists on entity '{}'",
                    typeid(T).name(), m_name);
                return nullptr;
            }

            // Create component
            auto component = CreateScope<T>(std::forward<Args>(args)...);
            T* componentPtr = component.get();

            // Initialize component
            componentPtr->InternalAwake(this, m_scene);

            // Store component
            m_components[typeId] = std::move(component);
            m_componentCache.push_back(componentPtr);

            // If entity is active and started, start the component
            if (IsActive() && m_hasStarted && componentPtr->IsEnabled()) {
                componentPtr->InternalStart();
            }

            return componentPtr;
        }

        /**
         * @brief Get component of type T
         * @tparam T Component type
         * @return Component pointer or nullptr if not found
         */
        template<typename T>
        T* GetComponent() {
            static_assert(std::is_base_of_v<Component, T>,
                "T must derive from Component");

            ComponentTypeID typeId = GetComponentTypeID<T>();
            auto it = m_components.find(typeId);
            if (it != m_components.end()) {
                return static_cast<T*>(it->second.get());
            }
            return nullptr;
        }

        /**
         * @brief Get component of type T (const version)
         */
        template<typename T>
        const T* GetComponent() const {
            return const_cast<Entity*>(this)->GetComponent<T>();
        }

        /**
         * @brief Check if entity has component of type T
         */
        template<typename T>
        bool HasComponent() const {
            static_assert(std::is_base_of_v<Component, T>,
                "T must derive from Component");

            ComponentTypeID typeId = GetComponentTypeID<T>();
            return m_components.find(typeId) != m_components.end();
        }

        /**
         * @brief Remove component of type T
         */
        template<typename T>
        void RemoveComponent() {
            static_assert(std::is_base_of_v<Component, T>,
                "T must derive from Component");

            ComponentTypeID typeId = GetComponentTypeID<T>();
            auto it = m_components.find(typeId);
            if (it != m_components.end()) {
                // Remove from cache
                Component* component = it->second.get();
                auto cacheIt = std::find(m_componentCache.begin(),
                    m_componentCache.end(), component);
                if (cacheIt != m_componentCache.end()) {
                    m_componentCache.erase(cacheIt);
                }

                // Destroy component
                component->InternalDestroy();
                m_components.erase(it);
            }
        }

        /**
         * @brief Get all components
         */
        const std::vector<Component*>& GetComponents() const {
            return m_componentCache;
        }

        /**
         * @brief Get component by type ID
         */
        Component* GetComponent(ComponentTypeID typeId);

        // ================== Component Queries ==================

        /**
         * @brief Get component in children
         */
        template<typename T>
        T* GetComponentInChildren(bool includeInactive = false) {
            // Check self first
            if (T* component = GetComponent<T>()) {
                return component;
            }

            // Check children recursively
            return GetComponentInChildrenRecursive<T>(this, includeInactive);
        }

        /**
         * @brief Get component in parent
         */
        template<typename T>
        T* GetComponentInParent() {
            Entity* current = this;
            while (current) {
                if (T* component = current->GetComponent<T>()) {
                    return component;
                }
                // Get parent entity through transform
                if (SceneTransform* parentTransform = current->m_transform.GetParent()) {
                    current = GetEntityFromTransform(parentTransform);
                }
                else {
                    break;
                }
            }
            return nullptr;
        }

        /**
         * @brief Get all components of type in children
         */
        template<typename T>
        void GetComponentsInChildren(std::vector<T*>& outComponents,
            bool includeInactive = false) {
            GetComponentsInChildrenRecursive<T>(this, outComponents, includeInactive);
        }

        // ================== Lifecycle ==================

        /**
         * @brief Called before first update
         */
        void Start();

        /**
         * @brief Called every frame
         * @param deltaTime Time since last frame
         */
        void Update(F32 deltaTime);

        /**
         * @brief Called after all updates
         * @param deltaTime Time since last frame
         */
        void LateUpdate(F32 deltaTime);

        /**
         * @brief Called for physics updates
         * @param fixedDeltaTime Fixed timestep
         */
        void FixedUpdate(F32 fixedDeltaTime);

        /**
         * @brief Called before entity is destroyed
         */
        void OnDestroy();

        // ================== Messages ==================

        /**
         * @brief Send message to all components
         * @param message Message ID
         * @param data Optional message data
         */
        void SendMessage(U32 message, void* data = nullptr);

        /**
         * @brief Send message to specific component type
         */
        template<typename T>
        void SendMessageToComponent(U32 message, void* data = nullptr) {
            if (T* component = GetComponent<T>()) {
                component->OnMessage(message, data);
            }
        }

    private:
        friend class Scene;
        friend class Component;

        // Core properties
        EntityID m_id;
        String m_name;
        Scene* m_scene;
        bool m_isActive = true;
        bool m_hasStarted = false;

        // Transform (always exists)
        SceneTransform m_transform;

        // Components
        std::unordered_map<ComponentTypeID, Scope<Component>> m_components;
        std::vector<Component*> m_componentCache; // For fast iteration

        // Tags and layers
        EntityTag m_tag = 0;
        U32 m_layer = 0;

        // Helper methods
        void NotifyTransformChanged();

        template<typename T>
        T* GetComponentInChildrenRecursive(Entity* entity, bool includeInactive);

        template<typename T>
        void GetComponentsInChildrenRecursive(Entity* entity,
            std::vector<T*>& outComponents, bool includeInactive);

        Entity* GetEntityFromTransform(SceneTransform* transform) const;
    };

    // ================== Helper Functions ==================
    // These are implemented in Entity.cpp but needed by Component.cpp

    /**
     * @brief Get entity's transform (for Component access)
     */
    SceneTransform& GetEntityTransform(const Entity* entity);

    /**
     * @brief Check if entity is active (for Component access)
     */
    bool IsEntityActive(const Entity* entity);

} // namespace Angaraka::Scene