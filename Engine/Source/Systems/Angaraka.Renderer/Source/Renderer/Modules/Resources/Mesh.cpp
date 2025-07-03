// Engine/Source/Systems/Angaraka.Renderer/Source/Renderer/Modules/Mesh.cpp
module;

#include "Angaraka/GraphicsBase.hpp"
#include "Angaraka/MeshBase.hpp"
#include <algorithm>
#include <mutex>


module Angaraka.Graphics.DirectX12.Mesh;

import Angaraka.Math;
import Angaraka.Core.Resources;
import Angaraka.Graphics.DirectX12;

namespace Angaraka::Graphics::DirectX12 {

    MeshResource::MeshResource(const String& id, const String& vertexLayout)
        : Resource(id)
        , m_keepCPUData(false)
        , m_vertexLayoutString(vertexLayout)
    {
        AGK_INFO("MeshResource: Created with ID '{}' and vertex layout '{}'", id, vertexLayout);
    }

    MeshResource::~MeshResource() {
        AGK_INFO("MeshResource: Destructor called for ID '{}'", GetId());
        Unload(); // Ensure cleanup on destruction
    }

    bool MeshResource::Load(const String& filePath, void* context) {
        AGK_INFO("MeshResource: Loading mesh from '{}'...", filePath);
        m_isLoaded = false; // Reset loaded state

        DirectX12GraphicsSystem* graphicsSystem = static_cast<DirectX12GraphicsSystem*>(context);
        MeshManager* meshManager = graphicsSystem ? graphicsSystem->GetMeshManager() : nullptr;
        if (!meshManager) {
            AGK_ERROR("MeshResource: No MeshManager available for loading '{}'", filePath);
            return m_isLoaded;
        }

        // Check if file exists
        if (!std::filesystem::exists(filePath)) {
            AGK_ERROR("MeshResource: File not found: '{}'", filePath);
            return m_isLoaded;
        }

        // Load CPU-side mesh data
        Scope<MeshData> meshData = LoadMeshData(filePath);
        if (!meshData) {
            AGK_ERROR("MeshResource: Failed to load mesh data from '{}'", filePath);
            return m_isLoaded;
        }

        // Validate mesh data
        if (!meshData->IsValid()) {
            AGK_ERROR("MeshResource: Invalid mesh data loaded from '{}'", filePath);
            return m_isLoaded;
        }

        AGK_INFO("MeshResource: CPU mesh data loaded - {} vertices, {} indices, layout: {}",
            meshData->vertexCount, meshData->indexCount, m_vertexLayoutString);

        // Create GPU mesh using MeshManager
        m_gpuMesh = meshManager->CreateGPUMesh(*meshData);
        if (!m_gpuMesh || !m_gpuMesh->IsValid()) {
            AGK_ERROR("MeshResource: Failed to create GPU mesh for '{}'", filePath);
            return m_isLoaded;
        }

        // Keep CPU data if requested (useful for collision detection, etc.)
        if (m_keepCPUData) {
            m_cpuMeshData = std::move(meshData);
            AGK_INFO("MeshResource: CPU mesh data cached for '{}'", filePath);
        }

        AGK_INFO("MeshResource: Successfully loaded GPU mesh for '{}' - {} vertices, {} indices",
            filePath, m_gpuMesh->vertexCount, m_gpuMesh->indexCount);

        m_isLoaded = true;
        return m_isLoaded;
    }

    void MeshResource::Unload() {
        if (m_gpuMesh) {
            AGK_INFO("MeshResource: Unloading GPU mesh data for '{}'", GetId());

            // Reset GPU mesh
            m_gpuMesh.reset();
        }

        // Clear CPU data
        if (m_cpuMeshData) {
            AGK_INFO("MeshResource: Unloading CPU mesh data for '{}'", GetId());
            m_cpuMeshData.reset();
        }
    }

    size_t MeshResource::GetSizeInBytes(void) const
    {
        return m_gpuMesh ? m_gpuMesh->GetTotalGPUMemorySizeBytes() : 0;
    }








    MeshManager::MeshManager() {
        AGK_INFO("MeshManager: Constructor called");
        m_stats = {};
    }

    MeshManager::~MeshManager() {
        AGK_INFO("MeshManager: Destructor called");
        Shutdown();
    }

    bool MeshManager::Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue) {
        if (!device || !commandQueue) {
            AGK_ERROR("MeshManager::Initialize: Invalid D3D12 device or command list");
            return false;
        }

        if (m_initialized) {
            AGK_WARN("MeshManager: Already initialized");
            return true;
        }

        m_device = device;
        m_CommandQueue = commandQueue;
        m_initialized = true;

        // Create dedicated command allocator for texture loading
        DXCall(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&m_LoadingCommandAllocator)));

        // Create dedicated command list for texture loading
        DXCall(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_LoadingCommandAllocator.Get(), nullptr,
            IID_PPV_ARGS(&m_LoadingCommandList)));

        // Close it initially
        DXCall(m_LoadingCommandList->Close());

        // Initialize statistics
        m_stats = {};
        m_statsDirty = true;

        AGK_INFO("MeshManager: Initialized successfully");
        return true;
    }

    void MeshManager::Shutdown() {
        if (!m_initialized) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_meshesMutex);

        // Clear all upload heaps
        m_uploadHeapsToRelease.clear();

        // Clear active meshes tracking
        m_activeMeshes.clear();

        // Reset state
        m_device = nullptr;
        m_initialized = false;

        AGK_INFO("MeshManager: Shutdown complete");
    }

    ID3D12GraphicsCommandList* MeshManager::GetOrCreateCommandList() {
        // Reset allocator and command list for new recording
        DXCall(m_LoadingCommandAllocator->Reset());
        DXCall(m_LoadingCommandList->Reset(m_LoadingCommandAllocator.Get(), nullptr));

        return m_LoadingCommandList.Get();
    }

    void MeshManager::ExecuteAndWaitForGPU() {
        // Close command list
        DXCall(m_LoadingCommandList->Close());

        // Execute on command queue
        ID3D12CommandList* commandLists[] = { m_LoadingCommandList.Get() };
        m_CommandQueue->ExecuteCommandLists(1, commandLists);

        // Wait for completion (simple synchronization)
        // In production, use proper fence synchronization
        Microsoft::WRL::ComPtr<ID3D12Fence> fence;
        DXCall(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

        DXCall(m_CommandQueue->Signal(fence.Get(), 1));

        if (fence->GetCompletedValue() < 1) {
            HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            DXCall(fence->SetEventOnCompletion(1, fenceEvent));
            WaitForSingleObject(fenceEvent, INFINITE);
            CloseHandle(fenceEvent);
        }
    }

    Scope<GPUMesh> MeshManager::CreateGPUMesh(const MeshData& meshData) {
        if (!m_initialized) {
            AGK_ERROR("MeshManager: Not initialized when attempting to create GPU mesh");
            return nullptr;
        }

        if (!meshData.IsValid()) {
            AGK_ERROR("MeshManager: Invalid mesh data provided");
            return nullptr;
        }

        AGK_INFO("MeshManager: Creating GPU mesh - {} vertices, {} indices, layout: {}",
            meshData.vertexCount, meshData.indexCount, meshData.layout.name);

        auto gpuMesh = std::make_unique<GPUMesh>();

        // Copy mesh properties
        gpuMesh->vertexCount = meshData.vertexCount;
        gpuMesh->indexCount = meshData.indexCount;
        gpuMesh->vertexStride = meshData.layout.vertexSize;
        gpuMesh->layout = meshData.layout;

        // Copy bounding information
        gpuMesh->boundingBoxMin = meshData.boundingBoxMin;
        gpuMesh->boundingBoxMax = meshData.boundingBoxMax;
        gpuMesh->boundingSphereCenter = meshData.boundingSphereCenter;
        gpuMesh->boundingSphereRadius = meshData.boundingSphereRadius;

        // Copy materials
        gpuMesh->materials = meshData.materials;

        auto commandList = GetOrCreateCommandList();

        // Create vertex buffer
        if (!CreateVertexBuffer(commandList, meshData, *gpuMesh)) {
            AGK_ERROR("MeshManager: Failed to create vertex buffer");
            return nullptr;
        }

        // Create index buffer
        if (!CreateIndexBuffer(commandList, meshData, *gpuMesh)) {
            AGK_ERROR("MeshManager: Failed to create index buffer");
            return nullptr;
        }

        ExecuteAndWaitForGPU();

        // Track the mesh for statistics
        {
            std::lock_guard<std::mutex> lock(m_meshesMutex);
            m_activeMeshes.push_back(gpuMesh.get());
            m_statsDirty = true;
        }

        AGK_INFO("MeshManager: Successfully created GPU mesh - VB: {} bytes, IB: {} bytes",
            gpuMesh->GetVertexBufferSize(), gpuMesh->GetIndexBufferSize());

        return gpuMesh;
    }

    void MeshManager::DestroyGPUMesh(GPUMesh* gpuMesh) {
        if (!gpuMesh) return;

        std::lock_guard<std::mutex> lock(m_meshesMutex);

        // Remove from active meshes tracking
        auto it = std::find(m_activeMeshes.begin(), m_activeMeshes.end(), gpuMesh);
        if (it != m_activeMeshes.end()) {
            m_activeMeshes.erase(it);
            m_statsDirty = true;
            AGK_INFO("MeshManager: GPU mesh removed from tracking");
        }

        // Note: The actual GPU resources will be released when the GPUMesh's
        // ComPtr members go out of scope in the unique_ptr destructor
    }

    bool MeshManager::CreateVertexBuffer(ID3D12GraphicsCommandList* commandList, const MeshData& meshData, GPUMesh& gpuMesh) {
        const size_t vertexBufferSize = meshData.GetVertexBufferSize();

        // Create vertex buffer resource descriptor
        CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

        // Create default heap resource for the GPU vertex buffer
        D3D12_HEAP_PROPERTIES defaultHeapProps = GetDefaultHeapProperties();
        HRESULT hr = m_device->CreateCommittedResource(
            &defaultHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &vertexBufferDesc,
            D3D12_RESOURCE_STATE_COMMON, // Start in copy destination state
            nullptr,
            IID_PPV_ARGS(&gpuMesh.vertexBuffer)
        );

        if (FAILED(hr)) {
            AGK_ERROR("MeshManager: Failed to create vertex buffer resource. HRESULT: {:#x}", hr);
            return false;
        }

        NAME_D3D12_OBJECT(gpuMesh.vertexBuffer, "Mesh Vertex Buffer");

        // Create upload heap for vertex data transfer
        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(gpuMesh.vertexBuffer.Get(), 0, 1);
        CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

        Microsoft::WRL::ComPtr<ID3D12Resource> vertexUploadHeap;
        D3D12_HEAP_PROPERTIES uploadHeapProps = GetUploadHeapProperties();

        hr = m_device->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &uploadBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, // Upload heaps must be in this state
            nullptr,
            IID_PPV_ARGS(&vertexUploadHeap)
        );

        if (FAILED(hr)) {
            AGK_ERROR("MeshManager: Failed to create vertex upload heap. HRESULT: {:#x}", hr);
            return false;
        }

        NAME_D3D12_OBJECT(vertexUploadHeap, "Mesh Vertex Upload Heap");

        // Add to upload heaps list for later cleanup
        m_uploadHeapsToRelease.push_back(vertexUploadHeap);

        // Prepare subresource data
        D3D12_SUBRESOURCE_DATA vertexData = {};
        vertexData.pData = meshData.vertexData.data();
        vertexData.RowPitch = vertexBufferSize;
        vertexData.SlicePitch = vertexBufferSize;


        // Transition vertex buffer from COMMON to COPY_DEST for upload
        CD3DX12_RESOURCE_BARRIER vertexBufferBarrierToCopy = CD3DX12_RESOURCE_BARRIER::Transition(
            gpuMesh.vertexBuffer.Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_COPY_DEST
        );
        commandList->ResourceBarrier(1, &vertexBufferBarrierToCopy);

        // Copy data from CPU to GPU via upload heap
        UpdateSubresources(commandList, gpuMesh.vertexBuffer.Get(), vertexUploadHeap.Get(), 0, 0, 1, &vertexData);

        // Transition vertex buffer to vertex buffer state
        CD3DX12_RESOURCE_BARRIER vertexBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            gpuMesh.vertexBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );
        commandList->ResourceBarrier(1, &vertexBufferBarrier);

        // Initialize vertex buffer view
        gpuMesh.vertexBufferView.BufferLocation = gpuMesh.vertexBuffer->GetGPUVirtualAddress();
        gpuMesh.vertexBufferView.StrideInBytes = meshData.layout.vertexSize;
        gpuMesh.vertexBufferView.SizeInBytes = static_cast<UINT>(vertexBufferSize);

        return true;
    }

    bool MeshManager::CreateIndexBuffer(ID3D12GraphicsCommandList* commandList, const MeshData& meshData, GPUMesh& gpuMesh) {
        const size_t indexBufferSize = meshData.GetIndexBufferSize();

        // Create index buffer resource descriptor
        CD3DX12_RESOURCE_DESC indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

        // Create default heap resource for the GPU index buffer
        D3D12_HEAP_PROPERTIES defaultHeapProps = GetDefaultHeapProperties();
        HRESULT hr = m_device->CreateCommittedResource(
            &defaultHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &indexBufferDesc,
            D3D12_RESOURCE_STATE_COMMON, // Start in copy destination state
            nullptr,
            IID_PPV_ARGS(&gpuMesh.indexBuffer)
        );

        if (FAILED(hr)) {
            AGK_ERROR("MeshManager: Failed to create index buffer resource. HRESULT: {:#x}", hr);
            return false;
        }

        NAME_D3D12_OBJECT(gpuMesh.indexBuffer, "Mesh Index Buffer");

        // Create upload heap for index data transfer
        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(gpuMesh.indexBuffer.Get(), 0, 1);
        CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

        Microsoft::WRL::ComPtr<ID3D12Resource> indexUploadHeap;
        D3D12_HEAP_PROPERTIES uploadHeapProps = GetUploadHeapProperties();

        hr = m_device->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &uploadBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, // Upload heaps must be in this state
            nullptr,
            IID_PPV_ARGS(&indexUploadHeap)
        );

        if (FAILED(hr)) {
            AGK_ERROR("MeshManager: Failed to create index upload heap. HRESULT: {:#x}", hr);
            return false;
        }

        NAME_D3D12_OBJECT(indexUploadHeap, "Mesh Index Upload Heap");

        // Add to upload heaps list for later cleanup
        m_uploadHeapsToRelease.push_back(indexUploadHeap);

        // Prepare subresource data
        D3D12_SUBRESOURCE_DATA indexData = {};
        indexData.pData = meshData.indices.data();
        indexData.RowPitch = indexBufferSize;
        indexData.SlicePitch = indexBufferSize;

        // Transition index buffer from COMMON to COPY_DEST for upload
        CD3DX12_RESOURCE_BARRIER indexBufferBarrierToCopy = CD3DX12_RESOURCE_BARRIER::Transition(
            gpuMesh.indexBuffer.Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_COPY_DEST
        );
        commandList->ResourceBarrier(1, &indexBufferBarrierToCopy);

        // Copy data from CPU to GPU via upload heap
        UpdateSubresources(commandList, gpuMesh.indexBuffer.Get(), indexUploadHeap.Get(), 0, 0, 1, &indexData);

        // Transition index buffer to index buffer state
        CD3DX12_RESOURCE_BARRIER indexBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            gpuMesh.indexBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_INDEX_BUFFER
        );
        commandList->ResourceBarrier(1, &indexBufferBarrier);

        // Initialize index buffer view
        gpuMesh.indexBufferView.BufferLocation = gpuMesh.indexBuffer->GetGPUVirtualAddress();
        gpuMesh.indexBufferView.Format = DXGI_FORMAT_R32_UINT; // 32-bit indices
        gpuMesh.indexBufferView.SizeInBytes = static_cast<UINT>(indexBufferSize);

        return true;
    }

    void MeshManager::ClearUploadHeaps() {
        if (!m_uploadHeapsToRelease.empty()) {
            AGK_INFO("MeshManager: Clearing {} upload heaps after GPU completion", m_uploadHeapsToRelease.size());
            m_uploadHeapsToRelease.clear();
        }
    }

    MeshManager::Statistics MeshManager::GetStatistics() const {
        if (m_statsDirty) {
            UpdateStatistics();
        }
        return m_stats;
    }

    void MeshManager::UpdateStatistics() const {
        std::lock_guard<std::mutex> lock(m_statsMutex);

        m_stats = {}; // Reset stats

        for (const GPUMesh* mesh : m_activeMeshes) {
            if (mesh && mesh->IsValid()) {
                m_stats.totalMeshes++;
                m_stats.totalVertices += mesh->vertexCount;
                m_stats.totalIndices += mesh->indexCount;
                m_stats.totalVertexMemory += mesh->GetVertexBufferSize();
                m_stats.totalIndexMemory += mesh->GetIndexBufferSize();
            }
        }

        // Calculate upload memory (temporary)
        for (const auto& uploadHeap : m_uploadHeapsToRelease) {
            if (uploadHeap) {
                D3D12_RESOURCE_DESC desc = uploadHeap->GetDesc();
                m_stats.totalUploadMemory += desc.Width;
            }
        }

        m_statsDirty = false;
    }

    void MeshManager::CompactMemory() {
        // Future optimization: implement memory defragmentation
        AGK_INFO("MeshManager: Memory compaction not yet implemented");
    }

    bool MeshManager::ValidateAllMeshes() const {
        std::lock_guard<std::mutex> lock(m_meshesMutex);

        bool allValid = true;
        for (const GPUMesh* mesh : m_activeMeshes) {
            if (!mesh || !mesh->IsValid()) {
                AGK_ERROR("MeshManager: Invalid mesh found during validation");
                allValid = false;
            }
        }

        AGK_INFO("MeshManager: Validation complete - {} meshes, all valid: {}",
            m_activeMeshes.size(), allValid ? "YES" : "NO");

        return allValid;
    }

    void MeshManager::LogMemoryUsage() const {
        Statistics stats = GetStatistics();

        AGK_INFO("MeshManager Memory Usage:");
        AGK_INFO("  - Total Meshes: {}", stats.totalMeshes);
        AGK_INFO("  - Total Vertices: {}", stats.totalVertices);
        AGK_INFO("  - Total Indices: {}", stats.totalIndices);
        AGK_INFO("  - Vertex Memory: {:.2f} MB", static_cast<F64>(stats.totalVertexMemory) * Math::Constants::BytesToMB);
        AGK_INFO("  - Index Memory: {:.2f} MB", static_cast<F64>(stats.totalIndexMemory) * Math::Constants::BytesToMB);
        AGK_INFO("  - Upload Memory: {:.2f} MB", static_cast<F64>(stats.totalUploadMemory) * Math::Constants::BytesToMB);
        AGK_INFO("  - Total GPU Memory: {:.2f} MB", static_cast<F64>(stats.totalVertexMemory + stats.totalIndexMemory) * Math::Constants::BytesToMB);
    }

} // namespace Angaraka::Graphics::DirectX12