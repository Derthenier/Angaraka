module;

#include "Angaraka/Base.hpp"

#undef near
#undef far

export module Angaraka.Scene.Components.Light;

import Angaraka.Math;
import Angaraka.Math.Color;
import Angaraka.Math.Vector4;
import Angaraka.Math.BoundingBox;
import Angaraka.Scene.Component;

namespace Angaraka::SceneSystem {

    /**
     * @brief Light component for scene lighting
     *
     * Supports multiple light types (directional, point, spot) with
     * shadows, color, and intensity controls.
     */
    export class Light : public ComponentBase<Light> {
    public:
        /**
         * @brief Type of light source
         */
        enum class Type {
            Directional,  // Sun-like, parallel rays
            Point,        // Omni-directional from a point
            Spot          // Cone of light
        };

        /**
         * @brief Shadow mapping quality
         */
        enum class ShadowQuality {
            None,         // No shadows
            Low,          // 512x512
            Medium,       // 1024x1024
            High,         // 2048x2048
            VeryHigh      // 4096x4096
        };

        Light() = default;
        ~Light() override = default;

        // ================== Light Properties ==================

        /**
         * @brief Set light type
         */
        void SetType(Type type) {
            if (m_type != type) {
                m_type = type;
                m_dirty = true;
            }
        }

        Type GetType() const { return m_type; }

        /**
         * @brief Set light color
         */
        void SetColor(const Math::Color& color) {
            m_color = color;
            m_dirty = true;
        }

        const Math::Color& GetColor() const { return m_color; }

        /**
         * @brief Set light intensity (brightness multiplier)
         */
        void SetIntensity(F32 intensity) {
            m_intensity = Math::Util::Max(0.0f, intensity);
            m_dirty = true;
        }

        F32 GetIntensity() const { return m_intensity; }

        /**
         * @brief Get final light color (color * intensity)
         */
        Math::Color GetFinalColor() const {
            return m_color * m_intensity;
        }

        // ================== Range (Point/Spot) ==================

        /**
         * @brief Set light range (point and spot lights)
         */
        void SetRange(F32 range) {
            m_range = Math::Util::Max(0.0f, range);
            m_dirty = true;
        }

        F32 GetRange() const { return m_range; }

        /**
         * @brief Set attenuation factors for falloff
         * @param constant Constant attenuation
         * @param linear Linear attenuation
         * @param quadratic Quadratic attenuation
         */
        void SetAttenuation(F32 constant, F32 linear, F32 quadratic) {
            m_attenuationConstant = constant;
            m_attenuationLinear = linear;
            m_attenuationQuadratic = quadratic;
            m_dirty = true;
        }

        void GetAttenuation(F32& constant, F32& linear, F32& quadratic) const {
            constant = m_attenuationConstant;
            linear = m_attenuationLinear;
            quadratic = m_attenuationQuadratic;
        }

        // ================== Spot Light Properties ==================

        /**
         * @brief Set inner cone angle (full intensity within this angle)
         * @param angle Angle in degrees
         */
        void SetInnerConeAngle(F32 angle) {
            m_innerConeAngle = Math::Util::Clamp(angle, 0.0f, m_outerConeAngle);
            m_innerConeCosine = std::cos(Math::Util::DegreesToRadians(m_innerConeAngle));
            m_dirty = true;
        }

        F32 GetInnerConeAngle() const { return m_innerConeAngle; }
        F32 GetInnerConeCosine() const { return m_innerConeCosine; }

        /**
         * @brief Set outer cone angle (zero intensity beyond this angle)
         * @param angle Angle in degrees
         */
        void SetOuterConeAngle(F32 angle) {
            m_outerConeAngle = Math::Util::Clamp(angle, m_innerConeAngle, 90.0f);
            m_outerConeCosine = std::cos(Math::Util::DegreesToRadians(m_outerConeAngle));
            m_dirty = true;
        }

        F32 GetOuterConeAngle() const { return m_outerConeAngle; }
        F32 GetOuterConeCosine() const { return m_outerConeCosine; }

        // ================== Shadows ==================

        /**
         * @brief Enable/disable shadow casting
         */
        void SetCastShadows(bool cast) {
            if (m_castShadows != cast) {
                m_castShadows = cast;
                m_shadowMapDirty = true;
            }
        }

        bool GetCastShadows() const { return m_castShadows; }

        /**
         * @brief Set shadow quality
         */
        void SetShadowQuality(ShadowQuality quality) {
            if (m_shadowQuality != quality) {
                m_shadowQuality = quality;
                m_shadowMapDirty = true;
            }
        }

        ShadowQuality GetShadowQuality() const { return m_shadowQuality; }

        /**
         * @brief Set shadow strength (0 = no shadows, 1 = full shadows)
         */
        void SetShadowStrength(F32 strength) {
            m_shadowStrength = Math::Util::Clamp01(strength);
        }

        F32 GetShadowStrength() const { return m_shadowStrength; }

        /**
         * @brief Set shadow bias to prevent shadow acne
         */
        void SetShadowBias(F32 bias) {
            m_shadowBias = bias;
        }

        F32 GetShadowBias() const { return m_shadowBias; }

        /**
         * @brief Set shadow map near/far planes (directional/spot lights)
         */
        void SetShadowNearFar(F32 near, F32 far) {
            m_shadowNearPlane = near;
            m_shadowFarPlane = far;
            m_shadowMapDirty = true;
        }

        F32 GetShadowNearPlane() const { return m_shadowNearPlane; }
        F32 GetShadowFarPlane() const { return m_shadowFarPlane; }

        // ================== Directional Light Properties ==================

        /**
         * @brief Get light direction (for directional/spot lights)
         * @return Forward direction in world space
         */
        Math::Vector3 GetDirection() const;

        /**
         * @brief Get light position (for point/spot lights)
         * @return World position
         */
        Math::Vector3 GetPosition() const;

        // ================== Culling & Optimization ==================

        /**
         * @brief Set light culling mask (which layers this light affects)
         */
        void SetCullingMask(U32 mask) {
            m_cullingMask = mask;
        }

        U32 GetCullingMask() const { return m_cullingMask; }

        /**
         * @brief Check if light affects a specific layer
         */
        bool AffectsLayer(U32 layer) const {
            return (m_cullingMask & (1 << layer)) != 0;
        }

        /**
         * @brief Set render mode (for debugging or special effects)
         */
        enum class RenderMode {
            Normal,      // Standard rendering
            Debug,       // Show light volume
            Disabled     // Light is off
        };

        void SetRenderMode(RenderMode mode) {
            m_renderMode = mode;
        }

        RenderMode GetRenderMode() const { return m_renderMode; }

        // ================== Light Bounds ==================

        /**
         * @brief Get world-space bounding sphere for light influence
         */
        void GetBoundingSphere(Math::Vector3& center, F32& radius) const;

        /**
         * @brief Get world-space bounding box for light influence
         */
        Math::BoundingBox GetBoundingBox() const;

        /**
         * @brief Check if light affects a bounding box
         */
        bool AffectsBounds(const Math::BoundingBox& bounds) const;

        // ================== GPU Data ==================

        /**
         * @brief GPU-friendly light data structure
         */
        struct GPULightData {
            Math::Vector4 positionAndType;      // xyz = position, w = type
            Math::Vector4 directionAndIntensity; // xyz = direction, w = intensity
            Math::Vector4 colorAndRange;         // xyz = color, w = range
            Math::Vector4 attenuationAndSpot;    // x,y,z = attenuation, w = inner cone cos
            Math::Vector4 shadowData;            // x = outer cone cos, y = bias, z = strength, w = flags
        };

        /**
         * @brief Get light data for GPU upload
         */
        GPULightData GetGPUData() const;

        // ================== State Management ==================

        /**
         * @brief Check if light parameters have changed
         */
        bool IsDirty() const { return m_dirty; }
        void ClearDirty() { m_dirty = false; }

        /**
         * @brief Check if shadow map needs update
         */
        bool IsShadowMapDirty() const { return m_shadowMapDirty; }
        void ClearShadowMapDirty() { m_shadowMapDirty = false; }

        // ================== Component Lifecycle ==================

        void OnEnable() override;
        void OnDisable() override;
        void OnTransformChanged() override;

    private:
        // Light properties
        Type m_type = Type::Point;
        Math::Color m_color = Math::Color(1.0f, 1.0f, 1.0f, 1.0f);
        F32 m_intensity = 1.0f;

        // Range and attenuation (point/spot)
        F32 m_range = 10.0f;
        F32 m_attenuationConstant = 1.0f;
        F32 m_attenuationLinear = 0.09f;
        F32 m_attenuationQuadratic = 0.032f;

        // Spot light properties
        F32 m_innerConeAngle = 30.0f;
        F32 m_outerConeAngle = 45.0f;
        F32 m_innerConeCosine = 0.866f; // cos(30)
        F32 m_outerConeCosine = 0.707f; // cos(45)

        // Shadow properties
        bool m_castShadows = false;
        ShadowQuality m_shadowQuality = ShadowQuality::Medium;
        F32 m_shadowStrength = 1.0f;
        F32 m_shadowBias = 0.005f;
        F32 m_shadowNearPlane = 0.1f;
        F32 m_shadowFarPlane = 100.0f;

        // Optimization
        U32 m_cullingMask = 0xFFFFFFFF; // Affects all layers by default
        RenderMode m_renderMode = RenderMode::Normal;

        // State tracking
        mutable bool m_dirty = true;
        mutable bool m_shadowMapDirty = true;

        // Cached data
        mutable Math::BoundingBox m_boundingBox;
        mutable bool m_boundsDirty = true;
    };

} // namespace Angaraka::Scene