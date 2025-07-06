module;

#include "Angaraka/Base.hpp"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sstream>

module Angaraka.Scene.Serializer;

import Angaraka.Scene;
import Angaraka.Scene.Entity;
import Angaraka.Scene.Component;
import Angaraka.Scene.Transform;
import Angaraka.Scene.Components.MeshRenderer;
import Angaraka.Scene.Components.Light;

import Angaraka.Math;
import Angaraka.Math.Vector3;
import Angaraka.Math.Color;

namespace Angaraka::SceneSystem {

    SceneSerializer::SceneSerializer() {
        RegisterBuiltInSerializers();
    }

    // ================== Scene Serialization ==================

    bool SceneSerializer::SaveScene(const Scene* scene, const String& filePath) {
        try {
            YAML::Node root = SerializeScene(scene);

            std::ofstream file(filePath);
            if (!file.is_open()) {
                SetError("Failed to open file for writing: " + filePath);
                return false;
            }

            if (m_prettyPrint) {
                file << root;
            }
            else {
                YAML::Emitter emitter;
                emitter << YAML::Flow << root;
                file << emitter.c_str();
            }

            file.close();
            AGK_INFO("SceneSerializer: Saved scene to '{}'", filePath);
            return true;

        }
        catch (const std::exception& e) {
            SetError("Exception during save: " + String(e.what()));
            return false;
        }
    }

    bool SceneSerializer::LoadScene(Scene* scene, const String& filePath) {
        try {
            std::ifstream file(filePath);
            if (!file.is_open()) {
                SetError("Failed to open file for reading: " + filePath);
                return false;
            }

            YAML::Node root = YAML::Load(file);
            file.close();

            if (!DeserializeScene(scene, root)) {
                return false;
            }

            AGK_INFO("SceneSerializer: Loaded scene from '{}'", filePath);
            return true;

        }
        catch (const std::exception& e) {
            SetError("Exception during load: " + String(e.what()));
            return false;
        }
    }

    YAML::Node SceneSerializer::SerializeScene(const Scene* scene) {
        YAML::Node root;

        // Scene metadata
        root["version"] = SCENE_VERSION;
        root["name"] = scene->GetName();
        root["timestamp"] = std::time(nullptr);

        // Scene settings
        YAML::Node settings;
        settings["collectStatistics"] = true; // Could expose this in Scene
        root["settings"] = settings;

        // Serialize all root entities (no parent)
        YAML::Node entities;
        std::vector<Entity*> rootEntities;
        scene->GetRootEntities(rootEntities);

        for (Entity* entity : rootEntities) {
            entities.push_back(SerializeEntity(scene, entity, true));
        }

        root["entities"] = entities;

        // Editor data if enabled
        if (m_includeEditorData) {
            YAML::Node editorData;
            // Could add camera position, selection, etc.
            root["editorData"] = editorData;
        }

        return root;
    }

    bool SceneSerializer::DeserializeScene(Scene* scene, const YAML::Node& root) {
        // Validate version
        if (!ValidateVersion(root)) {
            return false;
        }

        // Clear existing scene
        scene->Clear();

        // Load scene metadata
        if (root["name"]) {
            scene->SetName(root["name"].as<String>());
        }

        // Load entities
        if (root["entities"]) {
            EntityIDMap idMap;
            BuildEntityHierarchy(root["entities"], scene, idMap);
        }

        // Start the scene
        scene->Start();

        return true;
    }

    // ================== Entity Serialization ==================

    YAML::Node SceneSerializer::SerializeEntity(const Scene* scene, const Entity* entity, bool includeChildren) {
        YAML::Node node;

        // Entity properties
        node["id"] = "Entity_" + std::to_string(entity->GetID());
        node["name"] = entity->GetName();
        node["active"] = entity->IsActiveSelf();
        node["tag"] = entity->GetTag();
        node["layer"] = entity->GetLayer();

        // Transform
        node["transform"] = SerializeTransform(entity->GetTransform());

        // Components
        YAML::Node components;
        for (Component* component : entity->GetComponents()) {
            YAML::Node compNode = SerializeComponent(component);
            if (!compNode.IsNull()) {
                components.push_back(compNode);
            }
        }
        if (components.size() > 0) {
            node["components"] = components;
        }

        // Children
        if (includeChildren) {
            const auto& transform = entity->GetTransform();
            const auto& children = transform.GetChildren();

            if (!children.empty()) {
                YAML::Node childNodes;
                for (SceneTransform* childTransform : children) {
                    // Find entity for this transform
                    if (Entity* childEntity = scene->GetEntityFromTransform(childTransform)) {
                        childNodes.push_back(SerializeEntity(scene, childEntity, true));
                    }
                }
                if (childNodes.size() > 0) {
                    node["children"] = childNodes;
                }
            }
        }

        return node;
    }

    Entity* SceneSerializer::DeserializeEntity(Scene* scene, const YAML::Node& node,
        SceneTransform* parentTransform) {
        // Create entity
        String name = node["name"] ? node["name"].as<String>() : "Entity";
        Entity* entity = scene->CreateEntity(name);

        if (!entity) {
            SetError("Failed to create entity: " + name);
            return nullptr;
        }

        // Set properties
        if (node["active"]) {
            entity->SetActive(node["active"].as<bool>());
        }

        if (node["tag"]) {
            entity->SetTag(node["tag"].as<EntityTag>());
        }

        if (node["layer"]) {
            entity->SetLayer(node["layer"].as<U32>());
        }

        // Set transform
        if (node["transform"]) {
            DeserializeTransform(entity->GetTransform(), node["transform"]);
        }

        // Set parent
        if (parentTransform) {
            entity->GetTransform().SetParent(parentTransform, false);
        }

        // Add components
        if (node["components"]) {
            for (const auto& compNode : node["components"]) {
                DeserializeComponent(entity, compNode);
            }
        }

        // Process children recursively
        if (node["children"]) {
            for (const auto& childNode : node["children"]) {
                DeserializeEntity(scene, childNode, &entity->GetTransform());
            }
        }

        return entity;
    }

    // ================== Prefab Support ==================

    bool SceneSerializer::SavePrefab(const Scene* scene, const Entity* entity, const String& filePath) {
        try {
            YAML::Node root;
            root["version"] = SCENE_VERSION;
            root["type"] = "prefab";
            root["entity"] = SerializeEntity(scene, entity, true);

            std::ofstream file(filePath);
            if (!file.is_open()) {
                SetError("Failed to open prefab file for writing: " + filePath);
                return false;
            }

            file << root;
            file.close();

            AGK_INFO("SceneSerializer: Saved prefab to '{}'", filePath);
            return true;

        }
        catch (const std::exception& e) {
            SetError("Exception during prefab save: " + String(e.what()));
            return false;
        }
    }

    Entity* SceneSerializer::LoadPrefab(Scene* scene, const String& filePath,
        const Math::Vector3& position) {
        try {
            std::ifstream file(filePath);
            if (!file.is_open()) {
                SetError("Failed to open prefab file: " + filePath);
                return nullptr;
            }

            YAML::Node root = YAML::Load(file);
            file.close();

            if (!ValidateVersion(root)) {
                return nullptr;
            }

            if (!root["entity"]) {
                SetError("Invalid prefab file - missing entity data");
                return nullptr;
            }

            Entity* entity = DeserializeEntity(scene, root["entity"]);
            if (entity) {
                entity->GetTransform().SetLocalPosition(position);
                AGK_INFO("SceneSerializer: Loaded prefab from '{}'", filePath);
            }

            return entity;

        }
        catch (const std::exception& e) {
            SetError("Exception during prefab load: " + String(e.what()));
            return nullptr;
        }
    }

    // ================== Component Registration ==================

    void SceneSerializer::RegisterComponentSerializer(const String& typeName,
        ComponentSerializer serializer,
        ComponentDeserializer deserializer) {
        m_serializers[typeName] = serializer;
        m_deserializers[typeName] = deserializer;
        AGK_TRACE("SceneSerializer: Registered serializer for component type '{}'", typeName);
    }

    void SceneSerializer::UnregisterComponentSerializer(const String& typeName) {
        m_serializers.erase(typeName);
        m_deserializers.erase(typeName);
        AGK_TRACE("SceneSerializer: Unregistered serializer for component type '{}'", typeName);
    }

    bool SceneSerializer::CanSerializeComponent(const String& typeName) const {
        return m_serializers.find(typeName) != m_serializers.end();
    }

    // ================== Private Implementation ==================

    void SceneSerializer::RegisterBuiltInSerializers() {
        // Register MeshRenderer
        RegisterComponentSerializer("MeshRenderer",
            SerializeMeshRenderer,
            DeserializeMeshRenderer
        );

        // Register Light
        RegisterComponentSerializer("Light",
            SerializeLight,
            DeserializeLight);

        // Add more built-in component serializers here as they're created
        // RegisterComponentSerializer("Light", SerializeLight, DeserializeLight);
        // RegisterComponentSerializer("Camera", SerializeCamera, DeserializeCamera);
    }

    YAML::Node SceneSerializer::SerializeTransform(const SceneTransform& transform) {
        YAML::Node node;

        // Only save non-default values to reduce file size
        const auto& pos = transform.GetLocalPosition();
        const auto& rot = transform.GetLocalRotation();
        const auto& scale = transform.GetLocalScale();

        if (pos.x != 0 || pos.y != 0 || pos.z != 0) {
            node["position"] = pos;
        }

        // Convert quaternion to euler for readability
        Math::Vector3 euler = transform.GetLocalRotationEuler();
        if (euler.x != 0 || euler.y != 0 || euler.z != 0) {
            // Convert radians to degrees for readability
            euler.x = Math::Util::RadiansToDegrees(euler.x);
            euler.y = Math::Util::RadiansToDegrees(euler.y);
            euler.z = Math::Util::RadiansToDegrees(euler.z);
            node["rotation"] = euler;
        }

        if (scale.x != 1 || scale.y != 1 || scale.z != 1) {
            node["scale"] = scale;
        }

        return node;
    }

    void SceneSerializer::DeserializeTransform(SceneTransform& transform, const YAML::Node& node) {
        if (node["position"]) {
            transform.SetLocalPosition(node["position"].as<Math::Vector3>());
        }

        if (node["rotation"]) {
            // Convert degrees back to radians
            Math::Vector3 euler = node["rotation"].as<Math::Vector3>();
            euler.x = Math::Util::DegreesToRadians(euler.x);
            euler.y = Math::Util::DegreesToRadians(euler.y);
            euler.z = Math::Util::DegreesToRadians(euler.z);
            transform.SetLocalRotationEuler(euler);
        }

        if (node["scale"]) {
            transform.SetLocalScale(node["scale"].as<Math::Vector3>());
        }
    }

    YAML::Node SceneSerializer::SerializeComponent(const Component* component) {
        String typeName = String(component->GetTypeName());

        // Clean up type name (remove namespace, etc.)
        size_t lastColon = typeName.find_last_of(':');
        if (lastColon != String::npos) {
            typeName = typeName.substr(lastColon + 1);
        }

        auto it = m_serializers.find(typeName);
        if (it == m_serializers.end()) {
            AGK_WARN("SceneSerializer: No serializer for component type '{}'", typeName);
            return YAML::Node();
        }

        YAML::Node node;
        node["type"] = typeName;
        node["enabled"] = component->IsEnabled();
        node["data"] = it->second(component);

        return node;
    }

    Component* SceneSerializer::DeserializeComponent(Entity* entity, const YAML::Node& node) {
        if (!node["type"]) {
            SetError("Component missing type field");
            return nullptr;
        }

        String typeName = node["type"].as<String>();
        auto it = m_deserializers.find(typeName);
        if (it == m_deserializers.end()) {
            AGK_WARN("SceneSerializer: No deserializer for component type '{}'", typeName);
            return nullptr;
        }

        Component* component = it->second(entity, node["data"]);

        if (component && node["enabled"]) {
            component->SetEnabled(node["enabled"].as<bool>());
        }

        return component;
    }

    YAML::Node SceneSerializer::SerializeMeshRenderer(const Component* component) {
        const MeshRenderer* renderer = static_cast<const MeshRenderer*>(component);

        YAML::Node node;
        node["mesh"] = renderer->GetMeshResourceId();
        node["castShadows"] = renderer->GetCastShadows();
        node["receiveShadows"] = renderer->GetReceiveShadows();
        node["renderLayer"] = renderer->GetRenderLayer();

        return node;
    }

    Component* SceneSerializer::DeserializeMeshRenderer(Entity* entity, const YAML::Node& node) {
        MeshRenderer* renderer = entity->AddComponent<MeshRenderer>();
        if (!renderer) {
            return nullptr;
        }

        if (node["mesh"]) {
            renderer->SetMesh(node["mesh"].as<String>());
        }

        if (node["castShadows"]) {
            renderer->SetCastShadows(node["castShadows"].as<bool>());
        }

        if (node["receiveShadows"]) {
            renderer->SetReceiveShadows(node["receiveShadows"].as<bool>());
        }

        if (node["renderLayer"]) {
            renderer->SetRenderLayer(node["renderLayer"].as<U32>());
        }

        return renderer;
    }

    YAML::Node SceneSerializer::SerializeLight(const Component* component) {
        const Light* light = static_cast<const Light*>(component);

        YAML::Node node;

        // Type
        const char* typeStr = "Point";
        switch (light->GetType()) {
        case Light::Type::Directional: typeStr = "Directional"; break;
        case Light::Type::Point: typeStr = "Point"; break;
        case Light::Type::Spot: typeStr = "Spot"; break;
        }
        node["type"] = typeStr;

        // Color and intensity
        const auto& color = light->GetColor();
        node["color"] = std::vector<F32>{ color.R, color.G, color.B};
        node["intensity"] = light->GetIntensity();

        // Type-specific properties
        if (light->GetType() != Light::Type::Directional) {
            node["range"] = light->GetRange();

            // Attenuation
            F32 constant, linear, quadratic;
            light->GetAttenuation(constant, linear, quadratic);
            node["attenuation"] = std::vector<F32>{ constant, linear, quadratic };
        }

        if (light->GetType() == Light::Type::Spot) {
            node["innerConeAngle"] = light->GetInnerConeAngle();
            node["outerConeAngle"] = light->GetOuterConeAngle();
        }

        // Shadows
        if (light->GetCastShadows()) {
            YAML::Node shadows;
            shadows["enabled"] = true;

            const char* qualityStr = "Medium";
            switch (light->GetShadowQuality()) {
            case Light::ShadowQuality::None: qualityStr = "None"; break;
            case Light::ShadowQuality::Low: qualityStr = "Low"; break;
            case Light::ShadowQuality::Medium: qualityStr = "Medium"; break;
            case Light::ShadowQuality::High: qualityStr = "High"; break;
            case Light::ShadowQuality::VeryHigh: qualityStr = "VeryHigh"; break;
            }
            shadows["quality"] = qualityStr;
            shadows["strength"] = light->GetShadowStrength();
            shadows["bias"] = light->GetShadowBias();

            if (light->GetType() != Light::Type::Point) {
                shadows["nearPlane"] = light->GetShadowNearPlane();
                shadows["farPlane"] = light->GetShadowFarPlane();
            }

            node["shadows"] = shadows;
        }

        // Culling mask (only save if not default)
        if (light->GetCullingMask() != 0xFFFFFFFF) {
            node["cullingMask"] = light->GetCullingMask();
        }

        return node;
    }

    Component* SceneSerializer::DeserializeLight(Entity* entity, const YAML::Node& node) {
        Light* light = entity->AddComponent<Light>();
        if (!light) {
            return nullptr;
        }

        // Type
        if (node["type"]) {
            String typeStr = node["type"].as<String>();
            if (typeStr == "Directional") {
                light->SetType(Light::Type::Directional);
            }
            else if (typeStr == "Point") {
                light->SetType(Light::Type::Point);
            }
            else if (typeStr == "Spot") {
                light->SetType(Light::Type::Spot);
            }
        }

        // Color
        if (node["color"]) {
            auto colorVec = node["color"].as<std::vector<F32>>();
            if (colorVec.size() >= 3) {
                light->SetColor(Math::Color(colorVec[0], colorVec[1], colorVec[2], 1.0f));
            }
        }

        // Intensity
        if (node["intensity"]) {
            light->SetIntensity(node["intensity"].as<F32>());
        }

        // Range
        if (node["range"]) {
            light->SetRange(node["range"].as<F32>());
        }

        // Attenuation
        if (node["attenuation"]) {
            auto attVec = node["attenuation"].as<std::vector<F32>>();
            if (attVec.size() >= 3) {
                light->SetAttenuation(attVec[0], attVec[1], attVec[2]);
            }
        }

        // Spot light properties
        if (node["innerConeAngle"]) {
            light->SetInnerConeAngle(node["innerConeAngle"].as<F32>());
        }
        if (node["outerConeAngle"]) {
            light->SetOuterConeAngle(node["outerConeAngle"].as<F32>());
        }

        // Shadows
        if (node["shadows"]) {
            const auto& shadows = node["shadows"];

            if (shadows["enabled"] && shadows["enabled"].as<bool>()) {
                light->SetCastShadows(true);

                if (shadows["quality"]) {
                    String qualityStr = shadows["quality"].as<String>();
                    if (qualityStr == "Low") {
                        light->SetShadowQuality(Light::ShadowQuality::Low);
                    }
                    else if (qualityStr == "Medium") {
                        light->SetShadowQuality(Light::ShadowQuality::Medium);
                    }
                    else if (qualityStr == "High") {
                        light->SetShadowQuality(Light::ShadowQuality::High);
                    }
                    else if (qualityStr == "VeryHigh") {
                        light->SetShadowQuality(Light::ShadowQuality::VeryHigh);
                    }
                }

                if (shadows["strength"]) {
                    light->SetShadowStrength(shadows["strength"].as<F32>());
                }

                if (shadows["bias"]) {
                    light->SetShadowBias(shadows["bias"].as<F32>());
                }

                if (shadows["nearPlane"] && shadows["farPlane"]) {
                    light->SetShadowNearFar(
                        shadows["nearPlane"].as<F32>(),
                        shadows["farPlane"].as<F32>()
                    );
                }
            }
        }

        // Culling mask
        if (node["cullingMask"]) {
            light->SetCullingMask(node["cullingMask"].as<U32>());
        }

        return light;
    }

    void SceneSerializer::SetError(const String& error) const {
        m_lastError = error;
        AGK_ERROR("SceneSerializer: {}", error);
    }

    bool SceneSerializer::ValidateVersion(const YAML::Node& root) {
        if (!root["version"]) {
            SetError("Missing version field");
            return false;
        }

        U32 version = root["version"].as<U32>();
        if (version > SCENE_VERSION) {
            SetError("Scene version " + std::to_string(version) +
                " is newer than supported version " + std::to_string(SCENE_VERSION));
            return false;
        }

        return true;
    }

    void SceneSerializer::BuildEntityHierarchy(const YAML::Node& entities, Scene* scene, EntityIDMap& idMap) {
        // First pass: Create all entities without hierarchy
        for (const auto& entityNode : entities) {
            Entity* entity = DeserializeEntity(scene, entityNode, nullptr);
            if (entity && entityNode["id"]) {
                idMap[entityNode["id"].as<String>()] = entity;
            }
        }

        // Second pass would be needed if we stored parent references by ID
        // For now, hierarchy is built during deserialization
    }

} // namespace Angaraka::Scene