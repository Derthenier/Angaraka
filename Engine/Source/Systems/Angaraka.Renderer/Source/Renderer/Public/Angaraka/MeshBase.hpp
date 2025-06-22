// Engine/Source/Systems/Angaraka.Renderer/Source/Renderer/Public/Angaraka/MeshBase.hpp
#pragma once

#ifndef ANGARAKA_RENDERER_MESH_BASE_HPP
#define ANGARAKA_RENDERER_MESH_BASE_HPP

#include "GraphicsBase.hpp"
#include <DirectXMath.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <cassert>

namespace Angaraka::Graphics::DirectX12 {

    // Vertex attribute types
    enum class VertexAttributeType : U8 {
        Float1,     // 1 F32 (e.g., bone weight)
        Float2,     // 2 floats (e.g., UV coordinates)
        Float3,     // 3 floats (e.g., position, normal)
        Float4,     // 4 floats (e.g., color, tangent)
        UInt4,      // 4 unsigned ints (e.g., bone indices)
        UByte4_Norm // 4 unsigned bytes normalized to [0,1] (e.g., compressed color)
    };

    // Semantic names for vertex attributes
    enum class VertexSemantic : U8 {
        Position,
        Normal,
        Tangent,
        Bitangent,
        TexCoord0,
        TexCoord1,
        Color,
        BoneIndices,
        BoneWeights
    };

    // Single vertex attribute descriptor
    struct VertexAttribute {
        VertexSemantic semantic;
        VertexAttributeType type;
        U32 offset;        // Byte offset within vertex
        U8 semanticIndex;  // For multiple UVs: TEXCOORD0, TEXCOORD1

        VertexAttribute(VertexSemantic sem, VertexAttributeType typ, U32 off, U8 index = 0)
            : semantic(sem), type(typ), offset(off), semanticIndex(index) {
        }

        // Get size in bytes for this attribute type
        U32 GetSize() const {
            switch (type) {
            case VertexAttributeType::Float1: return 4;
            case VertexAttributeType::Float2: return 8;
            case VertexAttributeType::Float3: return 12;
            case VertexAttributeType::Float4: return 16;
            case VertexAttributeType::UInt4: return 16;
            case VertexAttributeType::UByte4_Norm: return 4;
            default: return 0;
            }
        }

        // Get DXGI format for D3D12 input layout
        DXGI_FORMAT GetDXGIFormat() const {
            switch (type) {
            case VertexAttributeType::Float1: return DXGI_FORMAT_R32_FLOAT;
            case VertexAttributeType::Float2: return DXGI_FORMAT_R32G32_FLOAT;
            case VertexAttributeType::Float3: return DXGI_FORMAT_R32G32B32_FLOAT;
            case VertexAttributeType::Float4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case VertexAttributeType::UInt4: return DXGI_FORMAT_R32G32B32A32_UINT;
            case VertexAttributeType::UByte4_Norm: return DXGI_FORMAT_R8G8B8A8_UNORM;
            default: return DXGI_FORMAT_UNKNOWN;
            }
        }

        // Get semantic name for D3D12 input layout
        const char* GetSemanticName() const {
            switch (semantic) {
            case VertexSemantic::Position: return "POSITION";
            case VertexSemantic::Normal: return "NORMAL";
            case VertexSemantic::Tangent: return "TANGENT";
            case VertexSemantic::Bitangent: return "BITANGENT";
            case VertexSemantic::TexCoord0:
            case VertexSemantic::TexCoord1: return "TEXCOORD";
            case VertexSemantic::Color: return "COLOR";
            case VertexSemantic::BoneIndices: return "BLENDINDICES";
            case VertexSemantic::BoneWeights: return "BLENDWEIGHT";
            default: return "UNKNOWN";
            }
        }
    };

    // Predefined vertex layouts for common use cases
    enum class VertexLayout : U8 {
        P,          // Position only (simple geometry, debug)
        PN,         // Position + Normal (lighting without textures)
        PNT,        // Position + Normal + TexCoord (standard static mesh)
        PNTC,       // Position + Normal + TexCoord + Color (vertex-colored mesh)
        PNTT,       // Position + Normal + 2 TexCoords (detail textures)
        PNTCB,      // Position + Normal + TexCoord + Color + Bones (animated mesh)
        PNTTB       // Position + Normal + Tangent + TexCoord + Bones (animated with normal maps)
    };

    // Vertex layout descriptor - contains all attributes for a layout
    struct VertexLayoutDescriptor {
        std::vector<VertexAttribute> attributes;
        U32 vertexSize;        // Total size of one vertex in bytes
        String name;           // Human-readable name

        VertexLayoutDescriptor() : vertexSize(0) {}

        // Add an attribute and update vertex size
        void AddAttribute(VertexSemantic semantic, VertexAttributeType type, U8 semanticIndex = 0) {
            attributes.emplace_back(semantic, type, vertexSize, semanticIndex);
            vertexSize += attributes.back().GetSize();
        }

        // Get D3D12 input element descriptors for PSO creation
        std::vector<D3D12_INPUT_ELEMENT_DESC> GetD3D12InputElements() const {
            std::vector<D3D12_INPUT_ELEMENT_DESC> elements;
            elements.reserve(attributes.size());

            for (const auto& attr : attributes) {
                D3D12_INPUT_ELEMENT_DESC desc = {};
                desc.SemanticName = attr.GetSemanticName();
                desc.SemanticIndex = attr.semanticIndex;
                desc.Format = attr.GetDXGIFormat();
                desc.InputSlot = 0;
                desc.AlignedByteOffset = attr.offset;
                desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                desc.InstanceDataStepRate = 0;
                elements.push_back(desc);
            }

            return elements;
        }

        // Find attribute by semantic
        const VertexAttribute* FindAttribute(VertexSemantic semantic, U8 semanticIndex = 0) const {
            for (const auto& attr : attributes) {
                if (attr.semantic == semantic && attr.semanticIndex == semanticIndex) {
                    return &attr;
                }
            }
            return nullptr;
        }

        // Check if layout has specific attribute
        bool HasAttribute(VertexSemantic semantic, U8 semanticIndex = 0) const {
            return FindAttribute(semantic, semanticIndex) != nullptr;
        }
    };

    // Factory class to create predefined vertex layouts
    class VertexLayoutFactory {
    public:
        static VertexLayoutDescriptor CreateLayout(VertexLayout layout) {
            VertexLayoutDescriptor desc;

            switch (layout) {
            case VertexLayout::P:
                desc.name = "Position";
                desc.AddAttribute(VertexSemantic::Position, VertexAttributeType::Float3);
                break;

            case VertexLayout::PN:
                desc.name = "Position + Normal";
                desc.AddAttribute(VertexSemantic::Position, VertexAttributeType::Float3);
                desc.AddAttribute(VertexSemantic::Normal, VertexAttributeType::Float3);
                break;

            case VertexLayout::PNT:
                desc.name = "Position + Normal + TexCoord";
                desc.AddAttribute(VertexSemantic::Position, VertexAttributeType::Float3);
                desc.AddAttribute(VertexSemantic::Normal, VertexAttributeType::Float3);
                desc.AddAttribute(VertexSemantic::TexCoord0, VertexAttributeType::Float2);
                break;

            case VertexLayout::PNTC:
                desc.name = "Position + Normal + TexCoord + Color";
                desc.AddAttribute(VertexSemantic::Position, VertexAttributeType::Float3);
                desc.AddAttribute(VertexSemantic::Normal, VertexAttributeType::Float3);
                desc.AddAttribute(VertexSemantic::TexCoord0, VertexAttributeType::Float2);
                desc.AddAttribute(VertexSemantic::Color, VertexAttributeType::Float4);
                break;

            case VertexLayout::PNTT:
                desc.name = "Position + Normal + 2 TexCoords";
                desc.AddAttribute(VertexSemantic::Position, VertexAttributeType::Float3);
                desc.AddAttribute(VertexSemantic::Normal, VertexAttributeType::Float3);
                desc.AddAttribute(VertexSemantic::TexCoord0, VertexAttributeType::Float2, 0);
                desc.AddAttribute(VertexSemantic::TexCoord1, VertexAttributeType::Float2, 1);
                break;

            case VertexLayout::PNTCB:
                desc.name = "Position + Normal + TexCoord + Color + Bones";
                desc.AddAttribute(VertexSemantic::Position, VertexAttributeType::Float3);
                desc.AddAttribute(VertexSemantic::Normal, VertexAttributeType::Float3);
                desc.AddAttribute(VertexSemantic::TexCoord0, VertexAttributeType::Float2);
                desc.AddAttribute(VertexSemantic::Color, VertexAttributeType::Float4);
                desc.AddAttribute(VertexSemantic::BoneIndices, VertexAttributeType::UInt4);
                desc.AddAttribute(VertexSemantic::BoneWeights, VertexAttributeType::Float4);
                break;

            case VertexLayout::PNTTB:
                desc.name = "Position + Normal + Tangent + TexCoord + Bones";
                desc.AddAttribute(VertexSemantic::Position, VertexAttributeType::Float3);
                desc.AddAttribute(VertexSemantic::Normal, VertexAttributeType::Float3);
                desc.AddAttribute(VertexSemantic::Tangent, VertexAttributeType::Float4);
                desc.AddAttribute(VertexSemantic::TexCoord0, VertexAttributeType::Float2);
                desc.AddAttribute(VertexSemantic::BoneIndices, VertexAttributeType::UInt4);
                desc.AddAttribute(VertexSemantic::BoneWeights, VertexAttributeType::Float4);
                break;

            default:
                assert(false && "Unknown vertex layout");
                break;
            }

            return desc;
        }

        // Convert string to VertexLayout enum (for bundle parsing)
        static VertexLayout StringToLayout(const String& layoutStr) {
            if (layoutStr == "P") return VertexLayout::P;
            if (layoutStr == "PN") return VertexLayout::PN;
            if (layoutStr == "PNT") return VertexLayout::PNT;
            if (layoutStr == "PNTC") return VertexLayout::PNTC;
            if (layoutStr == "PNTT") return VertexLayout::PNTT;
            if (layoutStr == "PNTCB") return VertexLayout::PNTCB;
            if (layoutStr == "PNTTB") return VertexLayout::PNTTB;

            assert(false && "Unknown vertex layout string");
            return VertexLayout::PNT; // Default fallback
        }

        // Convert VertexLayout enum to string
        static String LayoutToString(VertexLayout layout) {
            switch (layout) {
            case VertexLayout::P: return "P";
            case VertexLayout::PN: return "PN";
            case VertexLayout::PNT: return "PNT";
            case VertexLayout::PNTC: return "PNTC";
            case VertexLayout::PNTT: return "PNTT";
            case VertexLayout::PNTCB: return "PNTCB";
            case VertexLayout::PNTTB: return "PNTTB";
            default: return "UNKNOWN";
            }
        }
    };

    // CPU-side mesh data container
    struct MeshData {
        // Core mesh data
        std::vector<U8> vertexData;    // Raw interleaved vertex buffer
        std::vector<U32> indices;      // Index buffer (32-bit indices)
        VertexLayoutDescriptor layout;      // Vertex format description

        // Mesh properties
        U32 vertexCount;               // Number of vertices
        U32 indexCount;                // Number of indices
        String name;                   // Mesh name (from file or assigned)

        // Bounding information
        DirectX::XMFLOAT3 boundingBoxMin;   // AABB minimum
        DirectX::XMFLOAT3 boundingBoxMax;   // AABB maximum
        DirectX::XMFLOAT3 boundingSphereCenter;
        F32 boundingSphereRadius;

        // Future expansion for FBX/ufbx
        struct MaterialInfo {
            String name;
            String diffuseTexture;
            String normalTexture;
            String specularTexture;
            DirectX::XMFLOAT3 diffuseColor;
            F32 specularPower;
        };
        std::vector<MaterialInfo> materials;     // Material definitions
        std::vector<U32> materialIndices;  // Per-triangle material assignment

        // Constructor
        MeshData()
            : vertexCount(0)
            , indexCount(0)
            , boundingBoxMin(0.0f, 0.0f, 0.0f)
            , boundingBoxMax(0.0f, 0.0f, 0.0f)
            , boundingSphereCenter(0.0f, 0.0f, 0.0f)
            , boundingSphereRadius(0.0f)
        {
        }

        // Helper: Get size of one vertex in bytes
        U32 GetVertexSize() const {
            return layout.vertexSize;
        }

        // Helper: Get total vertex buffer size in bytes
        size_t GetVertexBufferSize() const {
            return static_cast<size_t>(vertexCount) * layout.vertexSize;
        }

        // Helper: Get total index buffer size in bytes
        size_t GetIndexBufferSize() const {
            return static_cast<size_t>(indexCount) * sizeof(U32);
        }

        // Helper: Validate mesh data consistency
        bool IsValid() const {
            if (vertexData.size() != GetVertexBufferSize()) return false;
            if (indices.size() != indexCount) return false;
            if (indexCount % 3 != 0) return false; // Must be triangles
            if (layout.attributes.empty()) return false;

            // Verify all indices are within vertex range
            for (U32 index : indices) {
                if (index >= vertexCount) return false;
            }

            return true;
        }

        // Helper: Calculate bounding box from position data
        void CalculateBounds() {
            if (vertexCount == 0) return;

            const VertexAttribute* posAttr = layout.FindAttribute(VertexSemantic::Position);
            if (!posAttr || posAttr->type != VertexAttributeType::Float3) return;

            // Initialize bounds to first vertex position
            const F32* firstPos = reinterpret_cast<const F32*>(vertexData.data() + posAttr->offset);
            boundingBoxMin = boundingBoxMax = DirectX::XMFLOAT3(firstPos[0], firstPos[1], firstPos[2]);

            // Iterate through all vertices
            for (U32 i = 0; i < vertexCount; ++i) {
                const F32* pos = reinterpret_cast<const F32*>(
                    vertexData.data() + i * layout.vertexSize + posAttr->offset
                    );

                // Update bounding box
                boundingBoxMin.x = std::min(boundingBoxMin.x, pos[0]);
                boundingBoxMin.y = std::min(boundingBoxMin.y, pos[1]);
                boundingBoxMin.z = std::min(boundingBoxMin.z, pos[2]);

                boundingBoxMax.x = std::max(boundingBoxMax.x, pos[0]);
                boundingBoxMax.y = std::max(boundingBoxMax.y, pos[1]);
                boundingBoxMax.z = std::max(boundingBoxMax.z, pos[2]);
            }

            // Calculate bounding sphere
            boundingSphereCenter.x = (boundingBoxMin.x + boundingBoxMax.x) * 0.5f;
            boundingSphereCenter.y = (boundingBoxMin.y + boundingBoxMax.y) * 0.5f;
            boundingSphereCenter.z = (boundingBoxMin.z + boundingBoxMax.z) * 0.5f;

            // Find maximum distance from center to any vertex
            boundingSphereRadius = 0.0f;
            DirectX::XMVECTOR center = DirectX::XMLoadFloat3(&boundingSphereCenter);

            for (U32 i = 0; i < vertexCount; ++i) {
                const F32* pos = reinterpret_cast<const F32*>(
                    vertexData.data() + i * layout.vertexSize + posAttr->offset
                    );

                DirectX::XMVECTOR vertexPos = DirectX::XMVectorSet(pos[0], pos[1], pos[2], 0.0f);
                DirectX::XMVECTOR distance = DirectX::XMVector3Length(DirectX::XMVectorSubtract(vertexPos, center));

                F32 dist = DirectX::XMVectorGetX(distance);
                boundingSphereRadius = std::max(boundingSphereRadius, dist);
            }
        }

        // Helper: Reserve memory for known vertex/index counts (optimization)
        void Reserve(U32 vertexReserve, U32 indexReserve) {
            if (layout.vertexSize > 0) {
                vertexData.reserve(static_cast<size_t>(vertexReserve) * layout.vertexSize);
            }
            indices.reserve(indexReserve);
        }

        // Helper: Clear all data
        void Clear() {
            vertexData.clear();
            indices.clear();
            vertexCount = 0;
            indexCount = 0;
            name.clear();
            materials.clear();
            materialIndices.clear();
            boundingBoxMin = boundingBoxMax = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
            boundingSphereCenter = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
            boundingSphereRadius = 0.0f;
        }
    };

} // namespace Angaraka::Graphics::DirectX12

#endif // ANGARAKA_RENDERER_MESH_BASE_HPP