module;

#include "Angaraka/GraphicsBase.hpp"
#include <DirectXTex.h> // This is for some helper functions in DirectXTex, not loading
#include <DirectXHelpers.h> // From DirectXTK12, if we decide to use it, for CreateTexture
#include <stdexcept>
#include <filesystem> // For file existence checks

module Angaraka.Graphics.DirectX12.Texture;

import Angaraka.Core.Resources;

namespace Angaraka::Graphics::DirectX12 {

    namespace {
    }

    TextureResource::TextureResource(const std::string& id)
        : Resource(id)
    {
        AGK_INFO("TextureResource: Created with ID '{0}'.", id);
    }

    TextureResource::~TextureResource() {
        AGK_INFO("TextureResource: Destructor called for ID '{0}'.", GetId());
        Unload(); // Ensure cleanup on destruction
    }

    bool TextureResource::Load(const std::string& filePath, void* context) {
        AGK_INFO("TextureResource: Loading texture from '{0}'...", filePath);

        TextureManager* textureManager = static_cast<TextureManager*>(context);
        if (!textureManager) {
            AGK_ERROR("TextureResource: TextureManager context is null. Cannot load texture.");
            return false;
        }
        
        m_textureData = textureManager->LoadTexture(filePath);

        if (m_textureData) {
            AGK_INFO("TextureResource: Successfully loaded D3D12 texture data for '{0}'.", filePath);
            return true;
        }
        else {
            AGK_ERROR("TextureResource: Failed to load D3D12 texture data for '{0}'.", filePath);
            return false;
        }
    }

    void TextureResource::Unload() {
        if (m_textureData) {
            AGK_INFO("TextureResource: Unloading D3D12 texture data for '{0}'.", GetId());
            // This relies on shared_ptr to release its reference.
            // If TextureManager needs explicit `DestroyTexture` calls, do it here.
            // Assuming the `Texture` struct/class's destructor handles its ComPtrs.
            m_textureData.reset(); // Release the shared_ptr, which will decrement ref count
        }
    }

    ID3D12Resource* TextureResource::GetD3D12Resource() const {
        return m_textureData ? m_textureData->Resource.Get() : nullptr;
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE TextureResource::GetSrvDescriptorHandle() const {
        return m_textureData ? m_textureData->SrvDescriptor : CD3DX12_CPU_DESCRIPTOR_HANDLE{};
    }

    const INT TextureResource::GetSrvIndex() const {
        return m_textureData ? m_textureData->SrvIndex : -1; // Return -1 if no texture is loaded
    }




    TextureManager::TextureManager() {}
    TextureManager::~TextureManager() { Shutdown(); }

    bool TextureManager::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
    {
        if (!device || !commandList)
        {
            AGK_ERROR("TextureManager::Initialize: Invalid D3D12 device or command list.");
            return false;
        }

        m_Device = device;
        m_CommandList = commandList;

        // Allocate a small descriptor heap for SRVs (Shader Resource Views)
        // This is a very basic, fixed-size heap for now. A real engine would have a dynamic allocator.
        if (!CreateSrvDescriptorHeap(256)) // Example: allow up to 256 textures
        {
            AGK_ERROR("Failed to create SRV descriptor heap for TextureManager.");
            return false;
        }

        AGK_INFO("TextureManager initialized.");
        initialized = true;
        return true;
    }

    void TextureManager::Shutdown()
    {
        if (!initialized) {
            return;
        }
        m_LoadedTextures.clear(); // Release shared_ptrs to textures
        m_SrvHeap.Reset();
        m_Device = nullptr;
        m_CommandList = nullptr;
        initialized = false;
        AGK_INFO("TextureManager shut down.");
    }

    bool TextureManager::CreateSrvDescriptorHeap(UINT numDescriptors)
    {
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = numDescriptors;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // Shaders can access this heap
        srvHeapDesc.NodeMask = 0;

        HRESULT hr = m_Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SrvHeap));
        if (FAILED(hr))
        {
            AGK_ERROR("Failed to create SRV descriptor heap. HRESULT: {:#x}", hr);
            return false;
        }

        m_SrvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        return true;
    }

    std::shared_ptr<Texture> TextureManager::LoadTexture(const std::string& filePath) {

        if (!std::filesystem::exists(filePath))
        {
            AGK_ERROR("Texture file not found: {}", std::string(filePath.begin(), filePath.end()));
            return nullptr;
        }

        [[maybe_unused]] DirectX::TexMetadata metadata;
        DirectX::ScratchImage scratchImage;

        DirectX::WIC_FLAGS wicFlags{ DirectX::WIC_FLAGS_NONE };
        DirectX::TGA_FLAGS tgaFlags{ DirectX::TGA_FLAGS_NONE };

        const std::wstring wfile{ std::wstring(filePath.begin(), filePath.end()) };
        const wchar_t* const file{ wfile.c_str() };

        wicFlags |= DirectX::WIC_FLAGS_FORCE_RGB;

        HRESULT hr{ DirectX::LoadFromWICFile(file, wicFlags, nullptr, scratchImage) };
        if (FAILED(hr))
        { // It wasn't WIC format. Try TGA.
            hr = DirectX::LoadFromTGAFile(file, tgaFlags, nullptr, scratchImage);
        }
        if (FAILED(hr))
        { // It wasn't TGA. Try HDR.
            hr = DirectX::LoadFromHDRFile(file, nullptr, scratchImage);
        }
        if (FAILED(hr))
        { // It wasn't HDR. Try DDS.
            hr = DirectX::LoadFromDDSFile(file, DirectX::DDS_FLAGS_FORCE_RGB, nullptr, scratchImage);
        }
        if (FAILED(hr))
        {
            AGK_ERROR("Failed to load texture '{}'. HRESULT: {:#x}", std::string(filePath.begin(), filePath.end()), hr);
            return nullptr;
        }

        // At this point, scratchImage contains the raw pixel data and metadata.
        // We need to copy it into our ImageData struct.
        // If the image has mipmaps or array slices, we'd iterate through them.
        // For simplicity, we'll just take the first image (base mip, first slice).
        const DirectX::Image* image = scratchImage.GetImage(0, 0, 0);
        if (!image)
        {
            AGK_ERROR("Failed to get image data from loaded texture '{}'.", std::string(filePath.begin(), filePath.end()));
            return nullptr;
        }

        std::unique_ptr<Angaraka::Core::ImageData> imageData = std::make_unique<Angaraka::Core::ImageData>();
        imageData->Width = image->width;
        imageData->Height = image->height;
        imageData->RowPitch = image->rowPitch;
        imageData->SlicePitch = image->slicePitch;
        imageData->Format = static_cast<int>(image->format); // Store DXGI_FORMAT as int

        // Allocate memory for pixels and copy data
        imageData->Pixels = std::make_unique<uint8_t[]>(image->slicePitch);
        memcpy(imageData->Pixels.get(), image->pixels, image->slicePitch);

        AGK_INFO("Successfully loaded texture '{}'. Dimensions: {}x{}, Format: {}",
            std::string(filePath.begin(), filePath.end()),
            imageData->Width, imageData->Height, imageData->Format);

        return CreateTextureFromImageData(*imageData);
    }

    std::shared_ptr<Texture> TextureManager::CreateTextureFromImageData(const Angaraka::Core::ImageData& imageData)
    {
        if (!m_Device || !m_CommandList)
        {
            AGK_ERROR("TextureManager not initialized when attempting to create texture from ImageData.");
            return nullptr;
        }
        if (!imageData.Pixels)
        {
            AGK_ERROR("ImageData provided has no pixel data.");
            return nullptr;
        }

        // For now, let's create a temporary unique name based on a counter
        // Later, this would be based on the file path or a proper asset ID
        std::string tempName = "TempTexture_" + std::to_string(m_LoadedTextures.size());
        if (m_LoadedTextures.count(tempName))
        {
            AGK_WARN("Texture with temporary name '{}' already exists in cache. This indicates a potential issue with unique naming.", tempName);
            // Return existing for now, but in a real system, you'd ensure uniqueness or better caching.
            return m_LoadedTextures.at(tempName);
        }

        std::shared_ptr<Texture> texture = std::make_shared<Texture>();
        texture->Width = static_cast<UINT>(imageData.Width);
        texture->Height = static_cast<UINT>(imageData.Height);
        texture->Format = static_cast<DXGI_FORMAT>(imageData.Format);

        // 1. Describe the texture resource
        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.MipLevels = 1; // For now, no mipmaps
        textureDesc.Format = texture->Format;
        textureDesc.Width = texture->Width;
        textureDesc.Height = texture->Height;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        // 2. Create a default heap resource for the GPU texture
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        HRESULT hr = m_Device->CreateCommittedResource(
            &heapProps, // Default heap for GPU-accessible resource
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, // Start in copy_dest state to upload data
            nullptr, // No optimized clear value for textures
            IID_PPV_ARGS(&texture->Resource));
        if (FAILED(hr))
        {
            AGK_ERROR("Failed to create committed resource for texture. HRESULT: {:#x}", hr);
            return nullptr;
        }
        NAME_D3D12_OBJECT(texture->Resource, "Angaraka Texture Resource");

        // This is a temporary upload buffer. A real engine uses a single, persistent upload heap.
        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture->Resource.Get(), 0, 1);

        // 3. Create an upload heap for copying CPU data to GPU
        CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC uploadResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(GetRequiredIntermediateSize(texture->Resource.Get(), 0, 1));

        Microsoft::WRL::ComPtr<ID3D12Resource> tempUploadHeap; // Temporary ComPtr for creation
        DXCall(m_Device->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &uploadResourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, // Upload heaps are always in this state
            nullptr,
            IID_PPV_ARGS(&tempUploadHeap)));

        NAME_D3D12_OBJECT(tempUploadHeap, "Angaraka Texture Upload Heap"); // Useful for debugging

        // IMPORTANT: Add this temporary upload heap to the list to keep it alive
        // The push_back operation copies the ComPtr, incrementing its reference count.
        m_UploadHeapsToRelease.push_back(tempUploadHeap);

        // Get the raw pointer to the resource that was just added to the vector
        ID3D12Resource* currentUploadHeap = m_UploadHeapsToRelease.back().Get();

        // Copy data from CPU ImageData to the upload heap, then to the default heap
        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = imageData.Pixels.get();
        subresourceData.RowPitch = imageData.RowPitch;
        subresourceData.SlicePitch = imageData.SlicePitch;

        // Use the currentUploadHeap obtained from the vector
        UpdateSubresources(m_CommandList, texture->Resource.Get(), currentUploadHeap, 0, 0, 1, &subresourceData);


        // 5. Transition the texture resource to a shader resource state
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture->Resource.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_CommandList->ResourceBarrier(1, &barrier);

        // 6. Create Shader Resource View (SRV)
        if (m_NextSrvDescriptorIndex >= m_SrvHeap->GetDesc().NumDescriptors)
        {
            AGK_ERROR("SRV descriptor heap is full! Cannot create more texture SRVs.");
            return nullptr;
        }

        texture->SrvDescriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(
            m_SrvHeap->GetCPUDescriptorHandleForHeapStart(),
            m_NextSrvDescriptorIndex,
            m_SrvDescriptorSize);
        texture->SrvIndex = m_NextSrvDescriptorIndex;

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = texture->Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1; // Only one mip level for now

        m_Device->CreateShaderResourceView(texture->Resource.Get(), &srvDesc, texture->SrvDescriptor);
        m_NextSrvDescriptorIndex++; // Increment for next texture

        m_LoadedTextures[tempName] = texture; // Add to cache

        AGK_INFO("GPU Texture created for '{}'.", tempName);
        return texture;
    }


    void TextureManager::ClearUploadHeaps()
    {
        // Calling clear() on a vector of ComPtrs will release all the underlying D3D12 resources
        m_UploadHeapsToRelease.clear();
        AGK_INFO("TextureManager: Cleared temporary upload heaps after GPU completion.");
    }
}