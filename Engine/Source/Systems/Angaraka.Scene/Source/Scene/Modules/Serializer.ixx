module;

#include "Angaraka/Base.hpp"
#include "Angaraka/MathCore.hpp"
#include <yaml-cpp/yaml.h>
#include <functional>
#include <unordered_map>
#include <fstream>

export module Angaraka.Scene.Serializer;

import Angaraka.Scene;
import Angaraka.Scene.Entity;
import Angaraka.Scene.Component;
import Angaraka.Scene.Transform;
import Angaraka.Scene.Components.MeshRenderer;

namespace Angaraka::SceneSystem {

    /**
     * @brief Handles serialization and deserialization of scenes
     *
     * Supports:
     * - Full scene hierarchy with transforms
     * - Component serialization (extensible)
     * - Prefab support for reusable entities
     * - Version compatibility checking
     */
    export class SceneSerializer {
    public:
        /**
         * @brief Component serialization function types
         */
        using ComponentSerializer = std::function<YAML::Node(const Component*)>;
        using ComponentDeserializer = std::function<Component* (Entity*, const YAML::Node&)>;

        /**
         * @brief Scene file version for compatibility
         */
        static constexpr U32 SCENE_VERSION = 1;

        SceneSerializer();
        ~SceneSerializer() = default;

        // ================== Scene Serialization ==================

        /**
         * @brief Save entire scene to file
         * @param scene Scene to save
         * @param filePath Output file path
         * @return True if successful
         */
        bool SaveScene(const Scene* scene, const String& filePath);

        /**
         * @brief Load scene from file
         * @param scene Scene to load into (will be cleared first)
         * @param filePath Input file path
         * @return True if successful
         */
        bool LoadScene(Scene* scene, const String& filePath);

        /**
         * @brief Save scene to YAML node (for embedding)
         */
        YAML::Node SerializeScene(const Scene* scene);

        /**
         * @brief Load scene from YAML node
         */
        bool DeserializeScene(Scene* scene, const YAML::Node& node);

        // ================== Entity Serialization ==================

        /**
         * @brief Serialize single entity to YAML
         * @param scene Scene context (for resource paths)
         * @param entity Entity to serialize
         * @param includeChildren Include child entities
         * @return YAML representation
         */
        YAML::Node SerializeEntity(const Scene* scene, const Entity* entity, bool includeChildren = true);

        /**
         * @brief Create entity from YAML
         * @param scene Scene to create entity in
         * @param node YAML data
         * @param parentTransform Optional parent transform
         * @return Created entity or nullptr on failure
         */
        Entity* DeserializeEntity(Scene* scene, const YAML::Node& node,
            SceneTransform* parentTransform = nullptr);

        // ================== Prefab Support ==================

        /**
         * @brief Save entity as prefab
         * @param scene Scene context (for resource paths)
         * @param entity Root entity of prefab
         * @param filePath Output file path
         * @return True if successful
         */
        bool SavePrefab(const Scene* scene, const Entity* entity, const String& filePath);

        /**
         * @brief Load prefab into scene
         * @param scene Target scene
         * @param filePath Prefab file path
         * @param position Optional spawn position
         * @return Root entity of instantiated prefab
         */
        Entity* LoadPrefab(Scene* scene, const String& filePath,
            const Math::Vector3& position = Math::Vector3(0, 0, 0));

        // ================== Component Registration ==================

        /**
         * @brief Register custom component serializer
         * @param typeName Component type name
         * @param serializer Function to convert component to YAML
         * @param deserializer Function to create component from YAML
         */
        void RegisterComponentSerializer(const String& typeName,
            ComponentSerializer serializer,
            ComponentDeserializer deserializer);

        /**
         * @brief Unregister component serializer
         */
        void UnregisterComponentSerializer(const String& typeName);

        /**
         * @brief Check if component type can be serialized
         */
        bool CanSerializeComponent(const String& typeName) const;

        // ================== Utilities ==================

        /**
         * @brief Set whether to include editor-only data
         */
        void SetIncludeEditorData(bool include) { m_includeEditorData = include; }

        /**
         * @brief Set whether to use readable format
         */
        void SetPrettyPrint(bool pretty) { m_prettyPrint = pretty; }

        /**
         * @brief Get last error message
         */
        const String& GetLastError() const { return m_lastError; }

    private:
        // Component serializers
        std::unordered_map<String, ComponentSerializer> m_serializers;
        std::unordered_map<String, ComponentDeserializer> m_deserializers;

        // Settings
        bool m_includeEditorData = false;
        bool m_prettyPrint = true;

        // Error handling
        mutable String m_lastError;

        // Built-in serializers
        void RegisterBuiltInSerializers();

        // Transform serialization
        YAML::Node SerializeTransform(const SceneTransform& transform);
        void DeserializeTransform(SceneTransform& transform, const YAML::Node& node);

        // Component serialization
        YAML::Node SerializeComponent(const Component* component);
        Component* DeserializeComponent(Entity* entity, const YAML::Node& node);

        // MeshRenderer serialization
        static YAML::Node SerializeMeshRenderer(const Component* component);
        static Component* DeserializeMeshRenderer(Entity* entity, const YAML::Node& node);

        // Light serialization
        static YAML::Node SerializeLight(const Component* component);
        static Component* DeserializeLight(Entity* entity, const YAML::Node& node);

        // Helpers
        void SetError(const String& error) const;
        bool ValidateVersion(const YAML::Node& root);

        // Entity ID mapping for hierarchy reconstruction
        using EntityIDMap = std::unordered_map<String, Entity*>;
        void BuildEntityHierarchy(const YAML::Node& entities, Scene* scene, EntityIDMap& idMap);
    };

    // ================== YAML Converters ==================
    // These allow direct YAML serialization of math types

} // namespace Angaraka::Scene

// YAML converters for math types
namespace YAML {

    template<>
    struct convert<Angaraka::Math::Vector3> {
        static Node encode(const Angaraka::Math::Vector3& v) {
            Node node;
            node.push_back(v.x);
            node.push_back(v.y);
            node.push_back(v.z);
            return node;
        }

        static bool decode(const Node& node, Angaraka::Math::Vector3& v) {
            if (!node.IsSequence() || node.size() != 3) {
                return false;
            }
            v.x = node[0].as<Angaraka::F32>();
            v.y = node[1].as<Angaraka::F32>();
            v.z = node[2].as<Angaraka::F32>();
            return true;
        }
    };

    template<>
    struct convert<Angaraka::Math::Quaternion> {
        static Node encode(const Angaraka::Math::Quaternion& q) {
            Node node;
            node.push_back(q.x);
            node.push_back(q.y);
            node.push_back(q.z);
            node.push_back(q.w);
            return node;
        }

        static bool decode(const Node& node, Angaraka::Math::Quaternion& q) {
            if (!node.IsSequence() || node.size() != 4) {
                return false;
            }
            q.x = node[0].as<Angaraka::F32>();
            q.y = node[1].as<Angaraka::F32>();
            q.z = node[2].as<Angaraka::F32>();
            q.w = node[3].as<Angaraka::F32>();
            return true;
        }
    };

} // namespace YAML