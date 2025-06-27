module;

#include "Angaraka/Base.hpp"
#include <typeindex>
#include <string_view>

export module Angaraka.Scene.Component;

import Angaraka.Scene.Transform;

namespace Angaraka::SceneSystem {

    // Forward declarations
    export class Entity;
    export class Scene;

    /**
     * @brief Type ID for runtime component type identification
     */
    export using ComponentTypeID = std::type_index;

    /**
     * @brief Get the type ID for a component type
     */
    export template<typename T>
        inline ComponentTypeID GetComponentTypeID() {
        return std::type_index(typeid(T));
    }

    /**
     * @brief Base class for all components in the ECS
     *
     * Components are data containers that can be attached to entities.
     * They should primarily contain data with minimal logic.
     * Heavy logic should go in Systems that operate on components.
     *
     * Lifecycle:
     * 1. Constructor - Component is created
     * 2. OnAwake - Called immediately after attachment to entity
     * 3. OnEnable - Called when component or entity becomes active
     * 4. OnStart - Called before first update (once per component)
     * 5. OnUpdate - Called every frame while active
     * 6. OnLateUpdate - Called after all updates
     * 7. OnDisable - Called when component or entity becomes inactive
     * 8. OnDestroy - Called before component is destroyed
     * 9. Destructor - Component is destroyed
     */
    export class Component {
    public:
        Component() = default;
        virtual ~Component() = default;

        // Non-copyable, non-moveable
        // Components are owned by entities and should not be copied
        Component(const Component&) = delete;
        Component& operator=(const Component&) = delete;
        Component(Component&&) = delete;
        Component& operator=(Component&&) = delete;

        // ================== Entity Access ==================

        /**
         * @brief Get the entity that owns this component
         * @return Pointer to owning entity (never null after attachment)
         */
        Entity* GetEntity() const { return m_entity; }

        /**
         * @brief Get the scene this component belongs to
         * @return Pointer to scene (never null after attachment)
         */
        Scene* GetScene() const { return m_scene; }

        /**
         * @brief Get the transform of the owning entity
         * @return Reference to entity's transform
         */
        SceneTransform& GetTransform() const;

        // ================== Component State ==================

        /**
         * @brief Check if component is enabled
         * @return True if enabled and entity is active
         */
        bool IsEnabled() const;

        /**
         * @brief Enable or disable this component
         * @param enabled New enabled state
         */
        void SetEnabled(bool enabled);

        /**
         * @brief Check if OnStart has been called
         * @return True if component has started
         */
        bool HasStarted() const { return m_hasStarted; }

        // ================== Type Information ==================

        /**
         * @brief Get the runtime type ID of this component
         * @return Type ID for runtime type checking
         */
        virtual ComponentTypeID GetTypeID() const = 0;

        /**
         * @brief Get the type name for debugging
         * @return Human-readable type name
         */
        virtual std::string_view GetTypeName() const = 0;

        // ================== Lifecycle Methods ==================
        // Override these in derived components

        /**
         * @brief Called immediately after component is attached to entity
         * Use for one-time setup that needs entity reference
         */
        virtual void OnAwake() {}

        /**
         * @brief Called before first update frame
         * Use for initialization that depends on other components
         */
        virtual void OnStart() {}

        /**
         * @brief Called when component becomes active
         * May be called multiple times during component lifetime
         */
        virtual void OnEnable() {}

        /**
         * @brief Called when component becomes inactive
         * May be called multiple times during component lifetime
         */
        virtual void OnDisable() {}

        /**
         * @brief Called every frame while component is active
         * @param deltaTime Time since last frame in seconds
         */
        virtual void OnUpdate(F32 deltaTime) {}

        /**
         * @brief Called every frame after all Update calls
         * @param deltaTime Time since last frame in seconds
         */
        virtual void OnLateUpdate(F32 deltaTime) {}

        /**
         * @brief Called every fixed timestep for physics
         * @param fixedDeltaTime Fixed time step in seconds
         */
        virtual void OnFixedUpdate(F32 fixedDeltaTime) {}

        /**
         * @brief Called before component is destroyed
         * Use for cleanup
         */
        virtual void OnDestroy() {}

        // ================== Component Communication ==================

        /**
         * @brief Called when transform has changed
         * Override to respond to transform changes
         */
        virtual void OnTransformChanged() {}

        /**
         * @brief Send a message to this component
         * @param message Message identifier
         * @param data Optional message data
         */
        virtual void OnMessage(U32 message, void* data = nullptr) {}

    protected:
        friend class Entity;  // Entity manages component lifecycle
        friend class Scene;   // Scene calls update methods

        // Set by Entity when attached
        Entity* m_entity = nullptr;
        Scene* m_scene = nullptr;

    private:
        bool m_enabled = true;
        bool m_hasStarted = false;

        // Internal lifecycle management
        void InternalAwake(Entity* entity, Scene* scene);
        void InternalStart();
        void InternalDestroy();
    };

    /**
     * @brief CRTP base for components to automatically provide type info
     *
     * Usage:
     * class MeshRenderer : public ComponentBase<MeshRenderer> {
     *     // Your component implementation
     * };
     */
    export template<typename T>
        class ComponentBase : public Component {
        public:
            ComponentBase() = default;
            virtual ~ComponentBase() = default;

            ComponentTypeID GetTypeID() const override {
                return GetComponentTypeID<T>();
            }

            std::string_view GetTypeName() const override {
                return typeid(T).name();
            }

            /**
             * @brief Static method to get type ID without instance
             */
            static ComponentTypeID StaticTypeID() {
                return GetComponentTypeID<T>();
            }
    };

    // ================== Common Component Types ==================
    // These are forward declared here for use throughout the engine

    export class Transform;      // Transform component (usually built into Entity)
    export class MeshRenderer;   // Renders a mesh
    export class Light;          // Light source
    export class Camera;         // Camera for rendering
    export class AudioSource;    // 3D audio source
    export class AudioListener;  // Audio listener (usually on camera)
    export class Collider;       // Physics collider base
    export class RigidBody;      // Physics rigid body
    export class Animator;       // Animation controller
    export class ParticleSystem; // Particle effects
    export class Script;         // User scripts base
}