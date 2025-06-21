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
    };

    // TextureManager to handle creation and management of GPU textures
    export class TextureManager
    {
    public:
        TextureManager();
        ~TextureManager();

        // Initialize with D3D12 device and command list for upload
        bool Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
        void Shutdown();

        // Creates a GPU texture from CPU ImageData
        // This is a temporary setup, later we'll use an upload heap/queue
        // For now, it will return a shared_ptr to allow multiple systems to refer to it.
        std::shared_ptr<Texture> LoadTexture(const std::string& filePath);
        std::shared_ptr<Texture> CreateTextureFromImageData(const Angaraka::Core::ImageData& imageData);

        // Placeholder for getting already loaded textures (caching)
        // std::shared_ptr<Texture> GetTexture(const std::wstring& name);

        // Call this after GPU has finished processing initialization commands
        void ClearUploadHeaps();

        // Get the SRV heap and descriptor size
        inline ID3D12DescriptorHeap* GetSrvHeap() const { return m_SrvHeap.Get(); }
        inline UINT GetSrvDescriptorSize() const { return m_SrvDescriptorSize; }

    private:
        ID3D12Device* m_Device = nullptr;
        ID3D12GraphicsCommandList* m_CommandList = nullptr; // Used for texture uploads

        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_UploadHeapsToRelease;

        // For now, a very simple descriptor heap for SRVs
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SrvHeap;
        UINT m_SrvDescriptorSize = 0;
        UINT m_NextSrvDescriptorIndex = 0; // Simple counter for descriptor allocation

        // A map to store loaded textures (simple cache for now)
        std::map<std::string, std::shared_ptr<Texture>> m_LoadedTextures;

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
        explicit TextureResource(const std::string& id);
        ~TextureResource();

        AGK_RESOURCE_TYPE_ID(TextureResource); // Define static TypeId

        // Implement pure virtual methods from base Resource class
        bool Load(const std::string& filePath, void* context = nullptr) override;
        void Unload() override;

        // Getters for the underlying D3D12 texture data (e.g., for binding)
        ID3D12Resource* GetD3D12Resource() const;
        CD3DX12_CPU_DESCRIPTOR_HANDLE GetSrvDescriptorHandle() const;
        const INT GetSrvIndex() const;

    private:
        // Hold the actual texture data managed by TextureManager
        // You might define a simple struct in Texture.hpp that contains these,
        // or get direct access. Let's assume TextureManager gives us a simple ID or struct.
        std::shared_ptr<Texture> m_textureData; // Assuming TextureManager::LoadTexture returns this

        // Important: You need access to the D3D12Device and CommandList for TextureManager::LoadTexture.
        // These would typically be provided to ResourceManager, and then to specific resource loaders.
        // For simplicity here, we'll assume a way to get them.
    };
}