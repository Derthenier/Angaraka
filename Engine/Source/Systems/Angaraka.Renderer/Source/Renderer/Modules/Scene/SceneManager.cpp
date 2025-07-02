module;

#include "Angaraka/GraphicsBase.hpp"
#include "Angaraka/MathCore.hpp"

module Angaraka.Graphics.DirectX12.SceneManager;

import Angaraka.Graphics.DirectX12;
import Angaraka.Graphics.DirectX12.Mesh;
import Angaraka.Core.ResourceCache;

namespace Angaraka::Graphics::DirectX12::Scene {

    Manager::Manager(
        Core::CachedResourceManager* resourceManager,
        DirectX12GraphicsSystem* meshManager)
        : m_resourceManager(resourceManager)
        , m_meshManager(meshManager)
    {
        AGK_INFO("Scene Manager initialized");
    }

    void Manager::AddObject(const Object& object)
    {
        // Validate mesh resource exists
        if (!IsValidMeshResource(object.meshResourceId))
        {
            AGK_WARN("Cannot add object '{}': mesh resource '{}' not found",
                object.name, object.meshResourceId);
            return;
        }

        // Check for duplicate names
        if (m_objectIndexMap.find(object.name) != m_objectIndexMap.end())
        {
            AGK_WARN("Object with name '{}' already exists, replacing it", object.name);
            RemoveObject(object.name);
        }

        // Add object
        m_objects.push_back(object);
        m_objectIndexMap[object.name] = m_objects.size() - 1;

        AGK_TRACE("Added scene object '{}' using mesh '{}'", object.name, object.meshResourceId);
    }

    void Manager::RemoveObject(const std::string& name)
    {
        auto it = m_objectIndexMap.find(name);
        if (it != m_objectIndexMap.end())
        {
            size_t index = it->second;

            // Remove from vector (swap with last element for efficiency)
            if (index < m_objects.size() - 1)
            {
                std::swap(m_objects[index], m_objects.back());
            }
            m_objects.pop_back();

            // Rebuild index map since indices changed
            RebuildIndexMap();

            AGK_INFO("Removed scene object '{}'", name);
        }
        else
        {
            AGK_WARN("Cannot remove object '{}': not found", name);
        }
    }

    Object* Manager::GetObject(const std::string& name)
    {
        auto it = m_objectIndexMap.find(name);
        if (it != m_objectIndexMap.end())
        {
            return &m_objects[it->second];
        }
        return nullptr;
    }

    void Manager::Update(F32 deltaTime)
    {
        // Update all objects (handles animation, etc.)
        for (auto& object : m_objects)
        {
            object.Update(deltaTime);
        }
    }

    void Manager::Clear()
    {
        m_objects.clear();
        m_objectIndexMap.clear();
        AGK_TRACE("Scene cleared - all objects removed");
    }

    void Manager::CreateTestScene()
    {
        AGK_TRACE("Creating test scene with multiple objects...");

        // Clear existing scene
        Clear();

        // Create multiple instances of your test mesh at different positions
        // Note: This assumes you have mesh resources loaded in your ResourceManager

        // Central rotating cube
        Object centerCube;
        centerCube.name = "CenterCube";
        centerCube.meshResourceId = "character/player_mesh"; // Use your existing mesh
        centerCube.position = { 0.0f, 0.0f, 0.0f };
        centerCube.scale = { 2.0f, 0.0f, 5.0f };
        centerCube.rotationSpeed = { 0.0f, 1.0f, 0.0f }; // Rotate around Y-axis
        AddObject(centerCube);

        // Surrounding objects in a circle
        const int numObjects = 8;
        const F32 radius = 5.0f;
        const F32 angleStep = Math::TwoPiF / numObjects;

        for (int i = 0; i < numObjects; ++i)
        {
            Object obj;
            obj.name = "CircleObject_" + std::to_string(i);
            obj.meshResourceId = "character/player_mesh"; // Use your existing mesh

            // Position in circle
            F32 angle = i * angleStep;
            obj.position.x = radius * cosf(angle);
            obj.position.z = radius * sinf(angle);
            obj.position.y = 0.0f;

            // Vary scale and rotation
            F32 scaleFactor = 0.5f + (i % 3) * 0.25f; // Scale between 0.5 and 1.0
            obj.scale = { scaleFactor, scaleFactor, scaleFactor };

            // Different rotation speeds
            obj.rotationSpeed.y = 0.5f + (i % 4) * 0.3f;

            AddObject(obj);
        }

        // Add some objects at different heights
        for (int i = 0; i < 4; ++i)
        {
            Object highObj;
            highObj.name = "HighObject_" + std::to_string(i);
            highObj.meshResourceId = "character/player_mesh";
            highObj.position = {
                (i - 1.5f) * 3.0f,  // X position
                3.0f + i * 1.0f,    // Y position (height)
                -8.0f               // Z position (behind circle)
            };
            highObj.scale = { 0.8f, 0.8f, 0.8f };
            highObj.rotationSpeed = { 0.3f, 0.8f, 0.2f }; // Tumbling motion

            AddObject(highObj);
        }

        AGK_TRACE("Test scene created with {} objects", GetObjectCount());
    }

    void Manager::LoadSceneFromBundle(const std::string& bundleName)
    {
        // TODO: Implement bundle-based scene loading
        // This would read a scene definition file (YAML) that lists objects and their transforms
        AGK_INFO("Scene loading from bundle '{}' - not yet implemented", bundleName);

        // For now, create test scene
        CreateTestScene();
    }

    void Manager::RebuildIndexMap()
    {
        m_objectIndexMap.clear();
        for (size_t i = 0; i < m_objects.size(); ++i)
        {
            m_objectIndexMap[m_objects[i].name] = i;
        }
    }

    bool Manager::IsValidMeshResource(const std::string& meshResourceId) const
    {
        if (!m_resourceManager)
        {
            AGK_ERROR("ResourceManager is null - cannot validate mesh resource");
            return false;
        }

        // Check if the mesh resource exists in the resource manager
        auto meshResource = m_resourceManager->GetResource<Angaraka::Core::Resource>(meshResourceId);
        return meshResource != nullptr;
    }
}