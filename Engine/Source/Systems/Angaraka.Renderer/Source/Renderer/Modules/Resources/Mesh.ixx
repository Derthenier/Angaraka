// Engine/Source/Systems/Angaraka.Renderer/Source/Renderer/Modules/Mesh.ixx
module;

#include "Angaraka/MeshBase.hpp"

export module Angaraka.Graphics.DirectX12.Mesh;

import <filesystem>;
import Angaraka.Core.Resources;
import Angaraka.Graphics.DirectX12.ObjLoader;

namespace Angaraka::Graphics::DirectX12 {

    // GPU-side mesh representation
    export struct GPUMesh {
        Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
        D3D12_INDEX_BUFFER_VIEW indexBufferView;

        // Mesh properties
        U32 vertexCount;
        U32 indexCount;
        U32 vertexStride;
        VertexLayoutDescriptor layout;

        // Bounding information
        DirectX::XMFLOAT3 boundingBoxMin;
        DirectX::XMFLOAT3 boundingBoxMax;
        DirectX::XMFLOAT3 boundingSphereCenter;
        F32 boundingSphereRadius;

        // Material information (for future use)
        std::vector<MeshData::MaterialInfo> materials;

        inline GPUMesh()
            : vertexCount(0)
            , indexCount(0)
            , vertexStride(0)
            , boundingBoxMin(0.0f, 0.0f, 0.0f)
            , boundingBoxMax(0.0f, 0.0f, 0.0f)
            , boundingSphereCenter(0.0f, 0.0f, 0.0f)
            , boundingSphereRadius(0.0f)
        {
        }

        // Helper: Check if mesh is valid for rendering
        inline bool IsValid() const {
            return vertexBuffer && indexBuffer && vertexCount > 0 && indexCount > 0;
        }

        // Helper: Get total vertex buffer size in bytes
        inline size_t GetVertexBufferSize() const {
            return static_cast<size_t>(vertexCount) * vertexStride;
        }

        // Helper: Get total index buffer size in bytes
        inline size_t GetIndexBufferSize() const {
            return static_cast<size_t>(indexCount) * sizeof(U32);
        }

        inline size_t GetTotalGPUMemorySizeBytes() const {
            return GetVertexBufferSize() + GetIndexBufferSize();
        }
    };

    /**
     * @brief Manages GPU-side mesh resources and buffers.
     *
     * Similar to TextureManager, this class handles the creation, management,
     * and cleanup of GPU mesh buffers. It works with the command list for
     * uploading mesh data and manages temporary upload heaps.
     */
    export class MeshManager {
    public:
        MeshManager();
        ~MeshManager();

        // Initialize with D3D12 device and command queue
        bool Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue);
        void Shutdown();

        // Create GPU mesh from CPU mesh data
        Scope<GPUMesh> CreateGPUMesh(const MeshData& meshData);

        // Destroy GPU mesh (called by MeshResource::Unload)
        void DestroyGPUMesh(GPUMesh* gpuMesh);

        // Call this after GPU has finished processing upload commands
        void ClearUploadHeaps();

        ID3D12GraphicsCommandList* GetOrCreateCommandList();
        void ExecuteAndWaitForGPU();

        // Get statistics
        struct Statistics {
            uint32_t totalMeshes;
            uint32_t totalVertices;
            uint32_t totalIndices;
            size_t totalVertexMemory;   // In bytes
            size_t totalIndexMemory;    // In bytes
            size_t totalUploadMemory;   // In bytes (temporary)
        };
        Statistics GetStatistics() const;

        // Memory management
        void CompactMemory();  // Defragment GPU memory (future optimization)

        // Validation and debugging
        bool ValidateAllMeshes() const;
        void LogMemoryUsage() const;

    private:
        ID3D12Device* m_device = nullptr;

        // Add command allocator for texture loading
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_LoadingCommandAllocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_LoadingCommandList;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue; // Store command queue reference

        // Track upload heaps that need to be released after GPU completes
        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_uploadHeapsToRelease;

        // Statistics tracking
        mutable Statistics m_stats;
        mutable std::mutex m_statsMutex;
        mutable bool m_statsDirty = true;

        // Internal mesh tracking (for statistics and validation)
        std::vector<GPUMesh*> m_activeMeshes;
        mutable std::mutex m_meshesMutex;

        bool m_initialized = false;

        // Helper methods
        bool CreateVertexBuffer(ID3D12GraphicsCommandList* commandList, const MeshData& meshData, GPUMesh& gpuMesh);
        bool CreateIndexBuffer(ID3D12GraphicsCommandList* commandList, const MeshData& meshData, GPUMesh& gpuMesh);
        void UpdateStatistics() const;

        // Memory utilities
        inline static D3D12_HEAP_PROPERTIES GetDefaultHeapProperties() {
            return CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        }

        inline static D3D12_HEAP_PROPERTIES GetUploadHeapProperties() {
            return CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        }
    };

    /**
     * @brief Concrete resource type for meshes.
     *
     * Manages the lifecycle of a loaded mesh, integrating with the MeshManager
     * for GPU resource creation and management.
     */
    export class MeshResource : public Angaraka::Core::Resource {
    public:
        // Constructor that takes the resource ID (file path) and optional parameters
        explicit MeshResource(const String& id, const String& vertexLayout = "PNT");
        ~MeshResource();

        AGK_RESOURCE_TYPE_ID(MeshResource); // Define static TypeId

        // Implement pure virtual methods from base Resource class
        bool Load(const String& filePath, void* context = nullptr) override;
        void Unload() override;

        size_t GetSizeInBytes(void) const override;

        // Getters for the underlying GPU mesh data
        const GPUMesh* GetGPUMesh() const { return m_gpuMesh.get(); }
        GPUMesh* GetGPUMesh() { return m_gpuMesh.get(); }

        // Convenience getters
        inline ID3D12Resource* GetVertexBuffer() const {
            return m_gpuMesh ? m_gpuMesh->vertexBuffer.Get() : nullptr;
        }

        inline ID3D12Resource* GetIndexBuffer() const {
            return m_gpuMesh ? m_gpuMesh->indexBuffer.Get() : nullptr;
        }

        inline const D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView() const {
            return m_gpuMesh ? &m_gpuMesh->vertexBufferView : nullptr;
        }

        inline const D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView() const {
            return m_gpuMesh ? &m_gpuMesh->indexBufferView : nullptr;
        }

        inline U32 GetVertexCount() const {
            return m_gpuMesh ? m_gpuMesh->vertexCount : 0;
        }

        inline U32 GetIndexCount() const {
            return m_gpuMesh ? m_gpuMesh->indexCount : 0;
        }

        inline const VertexLayoutDescriptor& GetVertexLayout() const {
            static VertexLayoutDescriptor emptyLayout;
            return m_gpuMesh ? m_gpuMesh->layout : emptyLayout;
        }

        // Bounding information
        inline DirectX::XMFLOAT3 GetBoundingBoxMin() const {
            return m_gpuMesh ? m_gpuMesh->boundingBoxMin : DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        }

        inline DirectX::XMFLOAT3 GetBoundingBoxMax() const {
            return m_gpuMesh ? m_gpuMesh->boundingBoxMax : DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        }

        inline DirectX::XMFLOAT3 GetBoundingSphereCenter() const {
            return m_gpuMesh ? m_gpuMesh->boundingSphereCenter : DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        }

        inline F32 GetBoundingSphereRadius() const {
            return m_gpuMesh ? m_gpuMesh->boundingSphereRadius : 0.0f;
        }

        // Material information
        inline const std::vector<MeshData::MaterialInfo>& GetMaterials() const {
            static std::vector<MeshData::MaterialInfo> emptyMaterials;
            return m_gpuMesh ? m_gpuMesh->materials : emptyMaterials;
        }

        // Check if mesh is loaded and valid
        inline bool IsLoaded() const {
            return m_gpuMesh && m_gpuMesh->IsValid();
        }

        // Get vertex layout as string (for bundle system)
        inline String GetVertexLayoutString() const {
            return m_vertexLayoutString;
        }

        // Set vertex layout (for bundle parsing)
        inline void SetVertexLayout(const std::string& layoutStr) {
            m_vertexLayoutString = layoutStr;
        }

        // Get CPU-side mesh data (if cached)
        inline const MeshData* GetCPUMeshData() const {
            return m_cpuMeshData.get();
        }

        // Enable/disable CPU data caching (useful for collision detection, etc.)
        inline void SetKeepCPUData(bool keep) {
            m_keepCPUData = keep;
        }

    private:
        // GPU-side mesh data
        Scope<GPUMesh> m_gpuMesh;

        // Optional CPU-side mesh data (for collision, etc.)
        Scope<MeshData> m_cpuMeshData;
        bool m_keepCPUData;

        // Vertex layout specification
        std::string m_vertexLayoutString;

        // OBJ loader instance
        mutable OBJ::Loader m_objLoader;

        // Helper: Determine file format from extension
        enum class MeshFormat {
            OBJ,
            FBX,    // Future
            GLTF,   // Future
            Unknown
        };

        MeshFormat GetMeshFormat(const String& filePath) const {
            std::filesystem::path path(filePath);
            String extension = path.extension().string();

            // Convert to lowercase
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

            if (extension == ".obj") return MeshFormat::OBJ;
            if (extension == ".fbx") return MeshFormat::FBX;
            if (extension == ".gltf" || extension == ".glb") return MeshFormat::GLTF;

            return MeshFormat::Unknown;
        }

        // Helper: Load mesh based on file format
        Scope<MeshData> LoadMeshData(const String& filePath) {
            MeshFormat format = GetMeshFormat(filePath);
            VertexLayout layout = VertexLayoutFactory::StringToLayout(m_vertexLayoutString);

            switch (format) {
            case MeshFormat::OBJ:
                return m_objLoader.Load(filePath, layout);

            case MeshFormat::FBX:
                AGK_ERROR("MeshResource: FBX format not yet supported for '{}'", filePath);
                return nullptr;

            case MeshFormat::GLTF:
                AGK_ERROR("MeshResource: glTF format not yet supported for '{}'", filePath);
                return nullptr;

            default:
                AGK_ERROR("MeshResource: Unknown mesh format for '{}'", filePath);
                return nullptr;
            }
        }
    };
}