module;

#include "Angaraka/GraphicsBase.hpp"
#include <DirectXTex.h> // This is for some helper functions in DirectXTex, not loading
#include <DirectXHelpers.h> // From DirectXTK12, if we decide to use it, for CreateTexture
#include <stdexcept>
#include <filesystem> // For file existence checks

module Angaraka.Graphics.DirectX12.Texture;

import Angaraka.Core.Resources;
import Angaraka.Graphics.DirectX12;

namespace Angaraka::Graphics::DirectX12 {

    namespace {
        String GetDXGIFormatAsString(DXGI_FORMAT format)
        {
            switch (format)
            {
                case DXGI_FORMAT_UNKNOWN: return "DXGI_FORMAT_UNKNOWN";
                case DXGI_FORMAT_R32G32B32A32_TYPELESS: return "DXGI_FORMAT_R32G32B32A32_TYPELESS";
                case DXGI_FORMAT_R32G32B32A32_FLOAT: return "DXGI_FORMAT_R32G32B32A32_FLOAT";
                case DXGI_FORMAT_R32G32B32A32_UINT: return "DXGI_FORMAT_R32G32B32A32_UINT";
                case DXGI_FORMAT_R32G32B32A32_SINT: return "DXGI_FORMAT_R32G32B32A32_SINT";
                case DXGI_FORMAT_R32G32B32_TYPELESS: return "DXGI_FORMAT_R32G32B32_TYPELESS";
                case DXGI_FORMAT_R32G32B32_FLOAT: return "DXGI_FORMAT_R32G32B32_FLOAT";
                case DXGI_FORMAT_R32G32B32_UINT: return "DXGI_FORMAT_R32G32B32_UINT";
                case DXGI_FORMAT_R32G32B32_SINT: return "DXGI_FORMAT_R32G32B32_SINT";
                case DXGI_FORMAT_R16G16B16A16_TYPELESS: return "DXGI_FORMAT_R16G16B16A16_TYPELESS";
                case DXGI_FORMAT_R16G16B16A16_FLOAT: return "DXGI_FORMAT_R16G16B16A16_FLOAT ";
                case DXGI_FORMAT_R16G16B16A16_UNORM: return "DXGI_FORMAT_R16G16B16A16_UNORM ";
                case DXGI_FORMAT_R16G16B16A16_UINT: return "DXGI_FORMAT_R16G16B16A16_UINT ";
                case DXGI_FORMAT_R16G16B16A16_SNORM: return "DXGI_FORMAT_R16G16B16A16_SNORM ";
                case DXGI_FORMAT_R16G16B16A16_SINT: return "DXGI_FORMAT_R16G16B16A16_SINT ";
                case DXGI_FORMAT_R32G32_TYPELESS: return "DXGI_FORMAT_R32G32_TYPELESS ";
                case DXGI_FORMAT_R32G32_FLOAT: return "DXGI_FORMAT_R32G32_FLOAT ";
                case DXGI_FORMAT_R32G32_UINT: return "DXGI_FORMAT_R32G32_UINT ";
                case DXGI_FORMAT_R32G32_SINT: return "DXGI_FORMAT_R32G32_SINT ";
                case DXGI_FORMAT_R32G8X24_TYPELESS: return "DXGI_FORMAT_R32G8X24_TYPELESS ";
                case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return "DXGI_FORMAT_D32_FLOAT_S8X24_UINT ";
                case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS: return "DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS ";
                case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT: return "DXGI_FORMAT_X32_TYPELESS_G8X24_UINT ";
                case DXGI_FORMAT_R10G10B10A2_TYPELESS: return "DXGI_FORMAT_R10G10B10A2_TYPELESS ";
                case DXGI_FORMAT_R10G10B10A2_UNORM: return "DXGI_FORMAT_R10G10B10A2_UNORM ";
                case DXGI_FORMAT_R10G10B10A2_UINT: return "DXGI_FORMAT_R10G10B10A2_UINT ";
                case DXGI_FORMAT_R11G11B10_FLOAT: return "DXGI_FORMAT_R11G11B10_FLOAT ";
                case DXGI_FORMAT_R8G8B8A8_TYPELESS: return "DXGI_FORMAT_R8G8B8A8_TYPELESS ";
                case DXGI_FORMAT_R8G8B8A8_UNORM: return "DXGI_FORMAT_R8G8B8A8_UNORM ";
                case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return "DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ";
                case DXGI_FORMAT_R8G8B8A8_UINT: return "DXGI_FORMAT_R8G8B8A8_UINT ";
                case DXGI_FORMAT_R8G8B8A8_SNORM: return "DXGI_FORMAT_R8G8B8A8_SNORM ";
                case DXGI_FORMAT_R8G8B8A8_SINT: return "DXGI_FORMAT_R8G8B8A8_SINT ";
                case DXGI_FORMAT_R16G16_TYPELESS: return "DXGI_FORMAT_R16G16_TYPELESS ";
                case DXGI_FORMAT_R16G16_FLOAT: return "DXGI_FORMAT_R16G16_FLOAT ";
                case DXGI_FORMAT_R16G16_UNORM: return "DXGI_FORMAT_R16G16_UNORM ";
                case DXGI_FORMAT_R16G16_UINT: return "DXGI_FORMAT_R16G16_UINT ";
                case DXGI_FORMAT_R16G16_SNORM: return "DXGI_FORMAT_R16G16_SNORM ";
                case DXGI_FORMAT_R16G16_SINT: return "DXGI_FORMAT_R16G16_SINT ";
                case DXGI_FORMAT_R32_TYPELESS: return "DXGI_FORMAT_R32_TYPELESS ";
                case DXGI_FORMAT_D32_FLOAT: return "DXGI_FORMAT_D32_FLOAT ";
                case DXGI_FORMAT_R32_FLOAT: return "DXGI_FORMAT_R32_FLOAT ";
                case DXGI_FORMAT_R32_UINT: return "DXGI_FORMAT_R32_UINT ";
                case DXGI_FORMAT_R32_SINT: return "DXGI_FORMAT_R32_SINT ";
                case DXGI_FORMAT_R24G8_TYPELESS: return "DXGI_FORMAT_R24G8_TYPELESS ";
                case DXGI_FORMAT_D24_UNORM_S8_UINT: return "DXGI_FORMAT_D24_UNORM_S8_UINT ";
                case DXGI_FORMAT_R24_UNORM_X8_TYPELESS: return "DXGI_FORMAT_R24_UNORM_X8_TYPELESS ";
                case DXGI_FORMAT_X24_TYPELESS_G8_UINT: return "DXGI_FORMAT_X24_TYPELESS_G8_UINT ";
                case DXGI_FORMAT_R8G8_TYPELESS: return "DXGI_FORMAT_R8G8_TYPELESS ";
                case DXGI_FORMAT_R8G8_UNORM: return "DXGI_FORMAT_R8G8_UNORM ";
                case DXGI_FORMAT_R8G8_UINT: return "DXGI_FORMAT_R8G8_UINT ";
                case DXGI_FORMAT_R8G8_SNORM: return "DXGI_FORMAT_R8G8_SNORM ";
                case DXGI_FORMAT_R8G8_SINT: return "DXGI_FORMAT_R8G8_SINT ";
                case DXGI_FORMAT_R16_TYPELESS: return "DXGI_FORMAT_R16_TYPELESS ";
                case DXGI_FORMAT_R16_FLOAT: return "DXGI_FORMAT_R16_FLOAT ";
                case DXGI_FORMAT_D16_UNORM: return "DXGI_FORMAT_D16_UNORM ";
                case DXGI_FORMAT_R16_UNORM: return "DXGI_FORMAT_R16_UNORM ";
                case DXGI_FORMAT_R16_UINT: return "DXGI_FORMAT_R16_UINT ";
                case DXGI_FORMAT_R16_SNORM: return "DXGI_FORMAT_R16_SNORM ";
                case DXGI_FORMAT_R16_SINT: return "DXGI_FORMAT_R16_SINT ";
                case DXGI_FORMAT_R8_TYPELESS: return "DXGI_FORMAT_R8_TYPELESS ";
                case DXGI_FORMAT_R8_UNORM: return "DXGI_FORMAT_R8_UNORM ";
                case DXGI_FORMAT_R8_UINT: return "DXGI_FORMAT_R8_UINT ";
                case DXGI_FORMAT_R8_SNORM: return "DXGI_FORMAT_R8_SNORM ";
                case DXGI_FORMAT_R8_SINT: return "DXGI_FORMAT_R8_SINT ";
                case DXGI_FORMAT_A8_UNORM: return "DXGI_FORMAT_A8_UNORM ";
                case DXGI_FORMAT_R1_UNORM: return "DXGI_FORMAT_R1_UNORM ";
                case DXGI_FORMAT_R9G9B9E5_SHAREDEXP: return "DXGI_FORMAT_R9G9B9E5_SHAREDEXP ";
                case DXGI_FORMAT_R8G8_B8G8_UNORM: return "DXGI_FORMAT_R8G8_B8G8_UNORM ";
                case DXGI_FORMAT_G8R8_G8B8_UNORM: return "DXGI_FORMAT_G8R8_G8B8_UNORM ";
                case DXGI_FORMAT_BC1_TYPELESS: return "DXGI_FORMAT_BC1_TYPELESS ";
                case DXGI_FORMAT_BC1_UNORM: return "DXGI_FORMAT_BC1_UNORM ";
                case DXGI_FORMAT_BC1_UNORM_SRGB: return "DXGI_FORMAT_BC1_UNORM_SRGB ";
                case DXGI_FORMAT_BC2_TYPELESS: return "DXGI_FORMAT_BC2_TYPELESS ";
                case DXGI_FORMAT_BC2_UNORM: return "DXGI_FORMAT_BC2_UNORM ";
                case DXGI_FORMAT_BC2_UNORM_SRGB: return "DXGI_FORMAT_BC2_UNORM_SRGB ";
                case DXGI_FORMAT_BC3_TYPELESS: return "DXGI_FORMAT_BC3_TYPELESS ";
                case DXGI_FORMAT_BC3_UNORM: return "DXGI_FORMAT_BC3_UNORM ";
                case DXGI_FORMAT_BC3_UNORM_SRGB: return "DXGI_FORMAT_BC3_UNORM_SRGB ";
                case DXGI_FORMAT_BC4_TYPELESS: return "DXGI_FORMAT_BC4_TYPELESS ";
                case DXGI_FORMAT_BC4_UNORM: return "DXGI_FORMAT_BC4_UNORM ";
                case DXGI_FORMAT_BC4_SNORM: return "DXGI_FORMAT_BC4_SNORM ";
                case DXGI_FORMAT_BC5_TYPELESS: return "DXGI_FORMAT_BC5_TYPELESS ";
                case DXGI_FORMAT_BC5_UNORM: return "DXGI_FORMAT_BC5_UNORM ";
                case DXGI_FORMAT_BC5_SNORM: return "DXGI_FORMAT_BC5_SNORM ";
                case DXGI_FORMAT_B5G6R5_UNORM: return "DXGI_FORMAT_B5G6R5_UNORM ";
                case DXGI_FORMAT_B5G5R5A1_UNORM: return "DXGI_FORMAT_B5G5R5A1_UNORM ";
                case DXGI_FORMAT_B8G8R8A8_UNORM: return "DXGI_FORMAT_B8G8R8A8_UNORM ";
                case DXGI_FORMAT_B8G8R8X8_UNORM: return "DXGI_FORMAT_B8G8R8X8_UNORM ";
                case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM: return "DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM ";
                case DXGI_FORMAT_B8G8R8A8_TYPELESS: return "DXGI_FORMAT_B8G8R8A8_TYPELESS ";
                case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return "DXGI_FORMAT_B8G8R8A8_UNORM_SRGB ";
                case DXGI_FORMAT_B8G8R8X8_TYPELESS: return "DXGI_FORMAT_B8G8R8X8_TYPELESS ";
                case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: return "DXGI_FORMAT_B8G8R8X8_UNORM_SRGB ";
                case DXGI_FORMAT_BC6H_TYPELESS: return "DXGI_FORMAT_BC6H_TYPELESS ";
                case DXGI_FORMAT_BC6H_UF16: return "DXGI_FORMAT_BC6H_UF16 ";
                case DXGI_FORMAT_BC6H_SF16: return "DXGI_FORMAT_BC6H_SF16 ";
                case DXGI_FORMAT_BC7_TYPELESS: return "DXGI_FORMAT_BC7_TYPELESS ";
                case DXGI_FORMAT_BC7_UNORM: return "DXGI_FORMAT_BC7_UNORM ";
                case DXGI_FORMAT_BC7_UNORM_SRGB: return "DXGI_FORMAT_BC7_UNORM_SRGB ";
                case DXGI_FORMAT_AYUV: return "DXGI_FORMAT_AYUV =";
                case DXGI_FORMAT_Y410: return "DXGI_FORMAT_Y410 =";
                case DXGI_FORMAT_Y416: return "DXGI_FORMAT_Y416 =";
                case DXGI_FORMAT_NV12: return "DXGI_FORMAT_NV12 =";
                case DXGI_FORMAT_P010: return "DXGI_FORMAT_P010 =";
                case DXGI_FORMAT_P016: return "DXGI_FORMAT_P016 =";
                case DXGI_FORMAT_420_OPAQUE: return "DXGI_FORMAT_420_OPAQUE =";
                case DXGI_FORMAT_YUY2: return "DXGI_FORMAT_YUY2 =";
                case DXGI_FORMAT_Y210: return "DXGI_FORMAT_Y210 =";
                case DXGI_FORMAT_Y216: return "DXGI_FORMAT_Y216 =";
                case DXGI_FORMAT_NV11: return "DXGI_FORMAT_NV11 =";
                case DXGI_FORMAT_AI44: return "DXGI_FORMAT_AI44 =";
                case DXGI_FORMAT_IA44: return "DXGI_FORMAT_IA44 =";
                case DXGI_FORMAT_P8: return "DXGI_FORMAT_P8 =";
                case DXGI_FORMAT_A8P8: return "DXGI_FORMAT_A8P8 =";
                case DXGI_FORMAT_B4G4R4A4_UNORM: return "DXGI_FORMAT_B4G4R4A4_UNORM =";
                case DXGI_FORMAT_P208: return "DXGI_FORMAT_P208 =";
                case DXGI_FORMAT_V208: return "DXGI_FORMAT_V208 =";
                case DXGI_FORMAT_V408: return "DXGI_FORMAT_V408 =";
                case DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE: return "DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE =";
                case DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE: return "DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE =";
                default: return "DXGI_FORMAT_UNKNOWN";
            }
        }

        size_t GetDXGIFormatBytesPerPixel(DXGI_FORMAT format)
        {
            switch (format)
            {
            case DXGI_FORMAT_R32G32B32A32_TYPELESS:
            case DXGI_FORMAT_R32G32B32A32_FLOAT:
            case DXGI_FORMAT_R32G32B32A32_UINT:
            case DXGI_FORMAT_R32G32B32A32_SINT:
                return 16; // 4 components * 4 bytes/component
            case DXGI_FORMAT_R32G32B32_TYPELESS:
            case DXGI_FORMAT_R32G32B32_FLOAT:
            case DXGI_FORMAT_R32G32B32_UINT:
            case DXGI_FORMAT_R32G32B32_SINT:
                return 12; // 3 components * 4 bytes/component
            case DXGI_FORMAT_R16G16B16A16_TYPELESS:
            case DXGI_FORMAT_R16G16B16A16_FLOAT:
            case DXGI_FORMAT_R16G16B16A16_UNORM:
            case DXGI_FORMAT_R16G16B16A16_UINT:
            case DXGI_FORMAT_R16G16B16A16_SNORM:
            case DXGI_FORMAT_R16G16B16A16_SINT:
                return 8; // 4 components * 2 bytes/component
            case DXGI_FORMAT_R32G32_TYPELESS:
            case DXGI_FORMAT_R32G32_FLOAT:
            case DXGI_FORMAT_R32G32_UINT:
            case DXGI_FORMAT_R32G32_SINT:
                return 8; // 2 components * 4 bytes/component
            case DXGI_FORMAT_R10G10B10A2_TYPELESS:
            case DXGI_FORMAT_R10G10B10A2_UNORM:
            case DXGI_FORMAT_R10G10B10A2_UINT:
            case DXGI_FORMAT_R11G11B10_FLOAT:
            case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            case DXGI_FORMAT_R8G8B8A8_UINT:
            case DXGI_FORMAT_R8G8B8A8_SNORM:
            case DXGI_FORMAT_R8G8B8A8_SINT:
                return 4; // 4 bytes/pixel
            case DXGI_FORMAT_R16G16_TYPELESS:
            case DXGI_FORMAT_R16G16_FLOAT:
            case DXGI_FORMAT_R16G16_UNORM:
            case DXGI_FORMAT_R16G16_UINT:
            case DXGI_FORMAT_R16G16_SNORM:
            case DXGI_FORMAT_R16G16_SINT:
                return 4; // 2 components * 2 bytes/component
            case DXGI_FORMAT_R32_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT:
            case DXGI_FORMAT_R32_FLOAT:
            case DXGI_FORMAT_R32_UINT:
            case DXGI_FORMAT_R32_SINT:
                return 4; // 1 component * 4 bytes/component
            case DXGI_FORMAT_R8G8_TYPELESS:
            case DXGI_FORMAT_R8G8_UNORM:
            case DXGI_FORMAT_R8G8_UINT:
            case DXGI_FORMAT_R8G8_SNORM:
            case DXGI_FORMAT_R8G8_SINT:
                return 2; // 2 components * 1 byte/component
            case DXGI_FORMAT_R16_TYPELESS:
            case DXGI_FORMAT_R16_FLOAT:
            case DXGI_FORMAT_D16_UNORM:
            case DXGI_FORMAT_R16_UNORM:
            case DXGI_FORMAT_R16_UINT:
            case DXGI_FORMAT_R16_SNORM:
            case DXGI_FORMAT_R16_SINT:
                return 2; // 1 component * 2 bytes/component
            case DXGI_FORMAT_R8_TYPELESS:
            case DXGI_FORMAT_R8_UNORM:
            case DXGI_FORMAT_R8_UINT:
            case DXGI_FORMAT_R8_SNORM:
            case DXGI_FORMAT_R8_SINT:
            case DXGI_FORMAT_A8_UNORM:
                return 1; // 1 component * 1 byte/component
            case DXGI_FORMAT_BC1_TYPELESS:
            case DXGI_FORMAT_BC1_UNORM:
            case DXGI_FORMAT_BC1_UNORM_SRGB:
                return 8; // 8 bytes per 4x4 block
            case DXGI_FORMAT_BC2_TYPELESS:
            case DXGI_FORMAT_BC2_UNORM:
            case DXGI_FORMAT_BC2_UNORM_SRGB:
            case DXGI_FORMAT_BC3_TYPELESS:
            case DXGI_FORMAT_BC3_UNORM:
            case DXGI_FORMAT_BC3_UNORM_SRGB:
                return 16; // 16 bytes per 4x4 block
            case DXGI_FORMAT_BC4_TYPELESS:
            case DXGI_FORMAT_BC4_UNORM:
            case DXGI_FORMAT_BC4_SNORM:
                return 8; // 8 bytes per 4x4 block
            case DXGI_FORMAT_BC5_TYPELESS:
            case DXGI_FORMAT_BC5_UNORM:
            case DXGI_FORMAT_BC5_SNORM:
                return 16; // 16 bytes per 4x4 block
            case DXGI_FORMAT_BC6H_TYPELESS:
            case DXGI_FORMAT_BC6H_UF16:
            case DXGI_FORMAT_BC6H_SF16:
                return 16; // 16 bytes per 4x4 block
            case DXGI_FORMAT_BC7_TYPELESS:
            case DXGI_FORMAT_BC7_UNORM:
            case DXGI_FORMAT_BC7_UNORM_SRGB:
                return 16; // 16 bytes per 4x4 block
                // Add other formats as needed. A comprehensive list can be found
                // in DirectXTex library's TexMetadata.cpp or similar sources.
            default:
                // Handle unknown formats, perhaps log an error or assert
                return 0;
            }
        }

        // This helper function checks if a given DXGI_FORMAT is a block-compressed format.
        inline bool IsDXGIFormatBlockCompressed(DXGI_FORMAT format)
        {
            switch (format)
            {
            case DXGI_FORMAT_BC1_TYPELESS:
            case DXGI_FORMAT_BC1_UNORM:
            case DXGI_FORMAT_BC1_UNORM_SRGB:
            case DXGI_FORMAT_BC2_TYPELESS:
            case DXGI_FORMAT_BC2_UNORM:
            case DXGI_FORMAT_BC2_UNORM_SRGB:
            case DXGI_FORMAT_BC3_TYPELESS:
            case DXGI_FORMAT_BC3_UNORM:
            case DXGI_FORMAT_BC3_UNORM_SRGB:
            case DXGI_FORMAT_BC4_TYPELESS:
            case DXGI_FORMAT_BC4_UNORM:
            case DXGI_FORMAT_BC4_SNORM:
            case DXGI_FORMAT_BC5_TYPELESS:
            case DXGI_FORMAT_BC5_UNORM:
            case DXGI_FORMAT_BC5_SNORM:
            case DXGI_FORMAT_BC6H_TYPELESS:
            case DXGI_FORMAT_BC6H_UF16:
            case DXGI_FORMAT_BC6H_SF16:
            case DXGI_FORMAT_BC7_TYPELESS:
            case DXGI_FORMAT_BC7_UNORM:
            case DXGI_FORMAT_BC7_UNORM_SRGB:
                return true;
            default:
                return false;
            }
        }

        // This function calculates the total memory size in bytes for a texture.
        // It considers both uncompressed and block-compressed formats.
        size_t CalculateTextureMemorySize(UINT width, UINT height, DXGI_FORMAT format)
        {
            if (width == 0 || height == 0)
            {
                return 0;
            }

            if (IsDXGIFormatBlockCompressed(format))
            {
                // For block-compressed formats, dimensions must be multiples of 4 (or padded).
                // Calculate the number of blocks in width and height.
                UINT numBlocksWide = (width + 3) / 4;
                UINT numBlocksHigh = (height + 3) / 4;

                // Get the size of one block for the given format.
                size_t bytesPerBlock = GetDXGIFormatBytesPerPixel(format);

                return static_cast<size_t>(numBlocksWide) * numBlocksHigh * bytesPerBlock;
            }
            else
            {
                // For uncompressed formats, it's a direct pixel count * bytes per pixel.
                size_t bytesPerPixel = GetDXGIFormatBytesPerPixel(format);
                return static_cast<size_t>(width) * height * bytesPerPixel;
            }
        }
    }

    TextureResource::TextureResource(const String& id)
        : Resource(id)
    {
        AGK_INFO("TextureResource: Created with ID '{0}'.", id);
    }

    TextureResource::~TextureResource() {
        AGK_INFO("TextureResource: Destructor called for ID '{0}'.", GetId());
        Unload(); // Ensure cleanup on destruction
    }

    bool TextureResource::Load(const String& filePath, void* context) {
        AGK_INFO("TextureResource: Loading texture from '{0}'...", filePath);
        m_isLoaded = false; // Reset loaded state

        DirectX12GraphicsSystem* graphicsSystem = static_cast<DirectX12GraphicsSystem*>(context);
        TextureManager* textureManager = graphicsSystem ? graphicsSystem->GetTextureManager() : nullptr;
        if (!textureManager) {
            AGK_ERROR("TextureResource: TextureManager context is null. Cannot load texture.");
            return m_isLoaded;
        }
        
        m_textureData = textureManager->LoadTexture(filePath);

        if (m_textureData) {
            AGK_INFO("TextureResource: Successfully loaded D3D12 texture data for '{0}'.", filePath);
            m_isLoaded = true;
        }
        else {
            AGK_ERROR("TextureResource: Failed to load D3D12 texture data for '{0}'.", filePath);
        }

        UINT srvIndex = GetSrvIndex();
        m_srvGpuHandle = textureManager->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart();
        m_srvGpuHandle.Offset(srvIndex, textureManager->GetSrvDescriptorSize());

        return m_isLoaded;
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

    CD3DX12_GPU_DESCRIPTOR_HANDLE TextureResource::GetGpuSrvDescriptorHandle() const {
        return m_srvGpuHandle; // This is set during Load
    }

    const INT TextureResource::GetSrvIndex() const {
        return m_textureData ? m_textureData->SrvIndex : -1; // Return -1 if no texture is loaded
    }




    TextureManager::TextureManager() { m_LoadedTextures.clear(); }
    TextureManager::~TextureManager() { Shutdown(); }

    bool TextureManager::Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue)
    {
        if (!device || !commandQueue)
        {
            AGK_ERROR("TextureManager::Initialize: Invalid D3D12 device or command queue.");
            return false;
        }

        m_Device = device;
        m_CommandQueue = commandQueue;

        // Create dedicated command allocator for texture loading
        DXCall(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&m_LoadingCommandAllocator)));

        // Create dedicated command list for texture loading
        DXCall(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_LoadingCommandAllocator.Get(), nullptr,
            IID_PPV_ARGS(&m_LoadingCommandList)));

        // Close it initially
        DXCall(m_LoadingCommandList->Close());

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

        initialized = false;
        AGK_INFO("TextureManager shut down.");
    }

    ID3D12GraphicsCommandList* TextureManager::GetOrCreateCommandList() {
        // Reset allocator and command list for new recording
        DXCall(m_LoadingCommandAllocator->Reset());
        DXCall(m_LoadingCommandList->Reset(m_LoadingCommandAllocator.Get(), nullptr));

        return m_LoadingCommandList.Get();
    }

    void TextureManager::ExecuteAndWaitForGPU() {
        // Close command list
        DXCall(m_LoadingCommandList->Close());

        // Execute on command queue
        ID3D12CommandList* commandLists[] = { m_LoadingCommandList.Get() };
        m_CommandQueue->ExecuteCommandLists(1, commandLists);

        // Wait for completion (simple synchronization)
        // In production, use proper fence synchronization
        Microsoft::WRL::ComPtr<ID3D12Fence> fence;
        DXCall(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

        DXCall(m_CommandQueue->Signal(fence.Get(), 1));

        if (fence->GetCompletedValue() < 1) {
            HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            DXCall(fence->SetEventOnCompletion(1, fenceEvent));
            WaitForSingleObject(fenceEvent, INFINITE);
            CloseHandle(fenceEvent);
        }
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

    Reference<Texture> TextureManager::LoadTexture(const String& filePath) {

        if (!std::filesystem::exists(filePath))
        {
            AGK_ERROR("Texture file not found: {}", String(filePath.begin(), filePath.end()));
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
            AGK_ERROR("Failed to load texture '{}'. HRESULT: {:#x}", String(filePath.begin(), filePath.end()), hr);
            return nullptr;
        }

        // At this point, scratchImage contains the raw pixel data and metadata.
        // We need to copy it into our ImageData struct.
        // If the image has mipmaps or array slices, we'd iterate through them.
        // For simplicity, we'll just take the first image (base mip, first slice).
        const DirectX::Image* image = scratchImage.GetImage(0, 0, 0);
        if (!image)
        {
            AGK_ERROR("Failed to get image data from loaded texture '{}'.", String(filePath.begin(), filePath.end()));
            return nullptr;
        }

        Scope<Angaraka::Core::ImageData> imageData = CreateScope<Angaraka::Core::ImageData>();
        imageData->Width = image->width;
        imageData->Height = image->height;
        imageData->RowPitch = image->rowPitch;
        imageData->SlicePitch = image->slicePitch;
        imageData->Format = static_cast<int>(image->format); // Store DXGI_FORMAT as int

        // Allocate memory for pixels and copy data
        imageData->Pixels = CreateScope<U8[]>(image->slicePitch);
        memcpy(imageData->Pixels.get(), image->pixels, image->slicePitch);

        AGK_INFO("Successfully loaded texture '{}'. Dimensions: {}x{}, Format: {}",
            String(filePath.begin(), filePath.end()),
            imageData->Width, imageData->Height, GetDXGIFormatAsString(image->format));

        return CreateTextureFromImageData(*imageData);
    }

    Reference<Texture> TextureManager::CreateTextureFromImageData(const Angaraka::Core::ImageData& imageData)
    {
        if (!m_Device)
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
        String tempName = "TempTexture_" + std::to_string(m_LoadedTextures.size());
        if (m_LoadedTextures.count(tempName))
        {
            AGK_WARN("Texture with temporary name '{}' already exists in cache. This indicates a potential issue with unique naming.", tempName);
            // Return existing for now, but in a real system, you'd ensure uniqueness or better caching.
            return m_LoadedTextures.at(tempName);
        }

        Reference<Texture> texture = CreateReference<Texture>();
        texture->Width = static_cast<UINT>(imageData.Width);
        texture->Height = static_cast<UINT>(imageData.Height);
        texture->Format = static_cast<DXGI_FORMAT>(imageData.Format);
        texture->MemorySizeBytes = CalculateTextureMemorySize(texture->Width, texture->Height, texture->Format);

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
        auto commandList = GetOrCreateCommandList();
        UpdateSubresources(commandList, texture->Resource.Get(), currentUploadHeap, 0, 0, 1, &subresourceData);


        // 5. Transition the texture resource to a shader resource state
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture->Resource.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(1, &barrier);

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

        ExecuteAndWaitForGPU();

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