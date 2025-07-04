// Engine/Source/Systems/Angaraka.Renderer/Source/Renderer/Modules/Texture.ixx
module;

#include "Angaraka/GraphicsBase.hpp"

export module Angaraka.Graphics.DirectX12.Texture;

import Angaraka.Core.Resources;

namespace Angaraka::Graphics::DirectX12 {

    // Simple struct to represent a GPU texture
    export struct Texture
    {
        Microsoft::WRL::ComPtr<ID3D12Resource>       Resource;
        CD3DX12_CPU_DESCRIPTOR_HANDLE                SrvDescriptor; // CPU handle to the Shader Resource View descriptor
        INT                                          SrvIndex = -1; // Index in the descriptor heap, for easy tracking
        // You might add SamplerDescriptor, UAVDescriptor if needed
        UINT                                         Width = 0;
        UINT                                         Height = 0;
        DXGI_FORMAT                                  Format = DXGI_FORMAT_UNKNOWN;
        size_t                                       MemorySizeBytes = 0; // Size in bytes for this texture
    };

    // TextureManager to handle creation and management of GPU textures
    export class TextureManager
    {
    public:
        TextureManager();
        ~TextureManager();

        // Initialize with D3D12 device and command list for upload
        bool Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue);
        void Shutdown();

        // Creates a GPU texture from CPU ImageData
        // This is a temporary setup, later we'll use an upload heap/queue
        // For now, it will return a shared_ptr to allow multiple systems to refer to it.
        Reference<Texture> LoadTexture(const String& filePath);
        Reference<Texture> CreateTextureFromImageData(const Angaraka::Core::ImageData& imageData);

        // Placeholder for getting already loaded textures (caching)
        // Reference<Texture> GetTexture(const std::wstring& name);

        // Call this after GPU has finished processing initialization commands
        void ClearUploadHeaps();

        // Add method to get fresh command list for loading
        ID3D12GraphicsCommandList* GetOrCreateCommandList();
        void ExecuteAndWaitForGPU();

        // Get the SRV heap and descriptor size
        inline ID3D12DescriptorHeap* GetSrvHeap() const { return m_SrvHeap.Get(); }
        inline UINT GetSrvDescriptorSize() const { return m_SrvDescriptorSize; }

    private:
        ID3D12Device* m_Device = nullptr;

        // Add command allocator for texture loading
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_LoadingCommandAllocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_LoadingCommandList;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue; // Store command queue reference

        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_UploadHeapsToRelease;

        // For now, a very simple descriptor heap for SRVs
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SrvHeap;
        UINT m_SrvDescriptorSize = 0;
        UINT m_NextSrvDescriptorIndex = 0; // Simple counter for descriptor allocation

        // A map to store loaded textures (simple cache for now)
        std::map<String, Reference<Texture>> m_LoadedTextures;

        // Internal helper to create a descriptor heap
        bool CreateSrvDescriptorHeap(UINT numDescriptors);

        bool initialized{ false };
    };

    /**
     * @brief Concrete resource type for textures.
     *
     * Manages the lifecycle of a loaded texture, interacting with the low-level
     * TextureManager for GPU resource creation.
     */
    export class TextureResource : public Angaraka::Core::Resource {
    public:
        // Constructor that takes the resource ID (file path)
        explicit TextureResource(const String& id);
        ~TextureResource();

        AGK_RESOURCE_TYPE_ID(TextureResource); // Define static TypeId

        // Implement pure virtual methods from base Resource class
        bool Load(const String& filePath, void* context = nullptr) override;
        void Unload() override;

        inline size_t GetSizeInBytes() const override {
            return m_textureData ? m_textureData->MemorySizeBytes : 0;
        }

        // Getters for the underlying D3D12 texture data (e.g., for binding)
        ID3D12Resource* GetD3D12Resource() const;
        CD3DX12_CPU_DESCRIPTOR_HANDLE GetSrvDescriptorHandle() const;
        CD3DX12_GPU_DESCRIPTOR_HANDLE GetGpuSrvDescriptorHandle() const;
        const INT GetSrvIndex() const;

    private:
        // Hold the actual texture data managed by TextureManager
        // You might define a simple struct in Texture.hpp that contains these,
        // or get direct access. Let's assume TextureManager gives us a simple ID or struct.
        Reference<Texture> m_textureData; // Assuming TextureManager::LoadTexture returns this

        CD3DX12_GPU_DESCRIPTOR_HANDLE m_srvGpuHandle; // GPU handle for the SRV
    };
}