module;

#include "Angaraka/Base.hpp"
#include "Angaraka/MathCore.hpp"
#include <algorithm>

module Angaraka.Scene.Components.Light;

import Angaraka.Scene.Component;
import Angaraka.Scene.Transform;
import Angaraka.Scene.Entity;

namespace Angaraka::SceneSystem {

    Math::Vector3 Light::GetDirection() const {
        // For directional and spot lights, use the forward direction
        return GetTransform().GetForward();
    }

    Math::Vector3 Light::GetPosition() const {
        // For point and spot lights, use world position
        return GetTransform().GetWorldPosition();
    }

    void Light::GetBoundingSphere(Math::Vector3& center, F32& radius) const {
        switch (m_type) {
        case Type::Directional:
            // Directional lights have infinite range
            center = Math::Vector3(0, 0, 0);
            radius = std::numeric_limits<F32>::max();
            break;

        case Type::Point:
            center = GetPosition();
            radius = m_range;
            break;

        case Type::Spot:
            // Conservative sphere that contains the spot cone
            center = GetPosition();
            // Calculate radius based on cone angle and range
            F32 halfAngle = Math::DegreesToRadians(m_outerConeAngle);
            F32 coneRadius = m_range * std::tan(halfAngle);
            // Sphere must contain the cone base
            radius = std::sqrt(m_range * m_range + coneRadius * coneRadius);
            break;
        }
    }

    Math::BoundingBox Light::GetBoundingBox() const {
        if (m_boundsDirty) {
            switch (m_type) {
            case Type::Directional:
                // Infinite bounds
                m_boundingBox = Math::BoundingBox(
                    Math::Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX),
                    Math::Vector3(FLT_MAX, FLT_MAX, FLT_MAX)
                );
                break;

            case Type::Point: {
                Math::Vector3 pos = GetPosition();
                Math::Vector3 halfExtents(m_range, m_range, m_range);
                m_boundingBox = Math::BoundingBox(
                    pos - halfExtents,
                    pos + halfExtents
                );
                break;
            }

            case Type::Spot: {
                // Calculate bounding box that contains the spot cone
                Math::Vector3 pos = GetPosition();
                Math::Vector3 dir = GetDirection();

                // Cone base center
                Math::Vector3 baseCenter = pos + dir * m_range;

                // Cone base radius
                F32 halfAngle = Math::DegreesToRadians(m_outerConeAngle);
                F32 baseRadius = m_range * std::tan(halfAngle);

                // Start with position
                m_boundingBox = Math::BoundingBox(pos, pos);

                // Expand to include cone base circle
                // This is conservative but correct
                Math::Vector3 baseExtents(baseRadius, baseRadius, baseRadius);
                m_boundingBox.ExpandToInclude(baseCenter - baseExtents);
                m_boundingBox.ExpandToInclude(baseCenter + baseExtents);
                break;
            }
            }

            m_boundsDirty = false;
        }

        return m_boundingBox;
    }

    bool Light::AffectsBounds(const Math::BoundingBox& bounds) const {
        switch (m_type) {
        case Type::Directional:
            // Directional lights affect everything
            return true;

        case Type::Point:
            // Check if bounds intersects sphere
            return bounds.IntersectsSphere(GetPosition(), m_range);

        case Type::Spot: {
            // First check bounding box intersection
            if (!GetBoundingBox().Intersects(bounds)) {
                return false;
            }

            // More precise cone test could be added here
            // For now, the bounding box test is sufficient
            return true;
        }
        }

        return false;
    }

    Light::GPULightData Light::GetGPUData() const {
        GPULightData data = {};

        // Position and type
        Math::Vector3 pos = GetPosition();
        data.positionAndType = Math::Vector4(pos.x, pos.y, pos.z, static_cast<F32>(m_type));

        // Direction and intensity
        Math::Vector3 dir = GetDirection();
        data.directionAndIntensity = Math::Vector4(dir.x, dir.y, dir.z, m_intensity);

        // Color and range
        data.colorAndRange = Math::Vector4(m_color.ToVector3(), m_range);

        // Attenuation and spot inner cone
        data.attenuationAndSpot = Math::Vector4(
            m_attenuationConstant,
            m_attenuationLinear,
            m_attenuationQuadratic,
            m_innerConeCosine
        );

        // Shadow data and spot outer cone
        U32 flags = 0;
        if (m_castShadows) flags |= 1;
        if (m_renderMode == RenderMode::Disabled) flags |= 2;

        data.shadowData = Math::Vector4(
            m_outerConeCosine,
            m_shadowBias,
            m_shadowStrength,
            static_cast<F32>(flags)
        );

        return data;
    }

    void Light::OnEnable() {
        AGK_TRACE("Light: Enabled on entity '{}'", GetEntity()->GetName());
        m_dirty = true;
        m_shadowMapDirty = true;

        // Register with lighting system
        // TODO: Get lighting manager and register this light
    }

    void Light::OnDisable() {
        AGK_TRACE("Light: Disabled on entity '{}'", GetEntity()->GetName());

        // Unregister from lighting system
        // TODO: Get lighting manager and unregister this light
    }

    void Light::OnTransformChanged() {
        m_dirty = true;
        m_boundsDirty = true;

        // Shadow map needs update if light moved (except directional)
        if (m_castShadows && m_type != Type::Directional) {
            m_shadowMapDirty = true;
        }
    }

} // namespace Angaraka::Scene