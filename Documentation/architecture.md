# Angaraka Engine Architecture Overview

## 1. Introduction

The Angaraka Engine is a modern C++ game engine designed for 3D graphics, leveraging the latest C++23 features and DirectX12. Our core philosophy emphasizes modularity, performance, clear separation of concerns, and maintainable, production-ready code. The engine has evolved to include a complete mesh loading system with bundle management.

## 2. Core Principles

* **Modern C++ (C++23):** Embracing new language features for cleaner, safer, and more expressive code.
* **Modularity:** Strong division of responsibilities among components and subsystems, facilitated by C++23 Modules for implementation units and traditional .hpp headers for public interfaces.
* **Dependency Injection (DI):** Minimizing global state and tight coupling by passing dependencies (like manager pointers or contexts) through constructors or explicit methods, enhancing testability and flexibility.
* **Performance:** Designing with efficiency in mind, especially for graphics and resource management.
* **Clarity & Maintainability:** Prioritizing readability, consistent naming conventions, and comprehensive logging.
* **Resource Ownership:** Strict use of smart pointers (`std::unique_ptr`, `std::shared_ptr`) for clear ownership semantics and automatic memory management.

## 3. Top-Level Structure

The engine is primarily composed of an `Application` layer that orchestrates various managers and subsystems.

* **`Angaraka::Application`**:
    * The main entry point and orchestrator of the engine lifecycle (Initialization, Update loop, Render loop, Shutdown).
    * Responsible for creating and managing the lifetime of top-level managers like `EventManager`, `InputManager`, `DirectX12GraphicsSystem`, and `ResourceManager`.

## 4. Core Subsystems

### 4.1. Event System (`Angaraka.Core.Events`)

* **`EventManager`**: A central pub-sub (publisher-subscriber) system.
    * Decouples components by allowing them to communicate asynchronously through events.
    * Used for system-wide notifications (e.g., window resize, input events, resource loaded events).
    * Thread-safe with atomic subscription ID generation and mutex-protected listener maps.

### 4.2. Input System (`Angaraka.Input.Windows`)

* **`Win64InputSystem`**: Handles raw input from keyboard and mouse.
    * Translates platform-specific input events into engine-agnostic events published on the `EventManager`.
    * Event-driven architecture with state tracking for press/release detection.
    * Supports mouse capture and visibility control for game-style input.

### 4.3. Camera System (`Angaraka.Camera`)

* **`Camera`**: Manages the view and projection matrices.
    * Handles camera movement (position, pitch, yaw) with FPS-style controls.
    * Provides projection properties (FOV, aspect ratio, near/far planes).
    * Integrates with input system via events for smooth camera movement.
    * Returns `XMMATRIX` for view and projection transformations.

## 5. Graphics Subsystem (`Angaraka.Graphics.DirectX12`)

The graphics subsystem is built around DirectX12 and is highly modular, with specialized managers for different aspects of the rendering pipeline.

### 5.1. Core Graphics Manager
* **`DirectX12GraphicsSystem` (Renderer)**:
    * The primary interface for rendering operations.
    * Orchestrates the DirectX12 device, command management, swap chain, and scene rendering.
    * Manages the lifetime and initialization of core DirectX12 managers.
    * **Dependency Injection for Internal Managers**: Creates and provides necessary components to the `ResourceManager`.
    * Handles window resize events by coordinating with relevant managers.
    * Supports multiple object rendering with independent transformations.

### 5.2. Device and Command Management
* **`DeviceManager`**: Manages the `ID3D12Device` and associated debug layers.
    * Handles adapter enumeration and device creation.
    * Configures debug layers for development builds.
    * Provides device access to other managers.

* **`CommandQueueAndListManager`**: Manages `ID3D12CommandAllocator`, `ID3D12GraphicsCommandList`, and `ID3D12CommandQueue`.
    * Issues GPU commands with proper synchronization.
    * Includes fence-based GPU/CPU synchronization mechanisms.
    * Handles command list reset/execute cycles.

### 5.3. Rendering Pipeline Management
* **`SwapChainManager`**: Manages the `IDXGISwapChain3` and associated render target views (RTVs).
    * Handles presentation and window resizing.
    * Manages double/triple buffering with proper resource state transitions.

* **`PSOManager`**: Manages `ID3D12PipelineState` objects and `ID3D12RootSignature`.
    * Compiles HLSL shaders and defines input layouts.
    * Creates root signatures for texture and constant buffer binding.
    * Supports multiple vertex layout configurations.

### 5.4. Resource Management
* **`BufferManager`**: Handles creation and management of GPU buffers.
    * **Vertex Buffers**: Stores per-vertex data with flexible layouts.
    * **Index Buffers**: Stores indices for indexed drawing.
    * **Constant Buffers**: Stores uniform data (e.g., MVP matrix) mapped to CPU memory for efficient updates.

* **`TextureManager`**: Manages GPU-side texture resources.
    * Creates `ID3D12Resource` objects for textures and their corresponding Shader Resource Views (SRVs).
    * Utilizes DirectXTex for processing various image formats (WIC, TGA, HDR, DDS).
    * Handles upload heap management for efficient GPU texture creation.

* **`MeshManager`**: **NEW** - Manages GPU-side mesh resources and buffers.
    * Creates vertex and index buffers from CPU mesh data.
    * Handles upload heap management with proper D3D12 state transitions.
    * Provides statistics tracking and memory usage monitoring.
    * Thread-safe mesh tracking with automatic cleanup.

### 5.5. Shader Management
* **`ShaderManager`**: Manages shader compilation from HLSL source files.
    * Compiles vertex and pixel shaders with debug support.
    * Handles shader caching and recompilation during development.

## 6. Resource Management System (`Angaraka.Core.Resources`)

This system provides a centralized, type-safe, and cache-driven approach to managing all engine assets, now including comprehensive mesh support.

### 6.1. Core Architecture
* **Core Principle**: Resources are loaded once, cached, and their lifetime is managed via `std::shared_ptr`. Dependencies required for loading are injected through dependency injection.

* **`Resource` Base Class**:
    * Abstract interface (`virtual bool Load(...)`, `virtual void Unload()`) for all managed assets.
    * Each resource has a unique `id` (typically its file path).
    * Utilizes `AGK_RESOURCE_TYPE_ID` macro for runtime type identification.

* **`ResourceManager`**:
    * Central cache and coordinator for `Resource` objects.
    * **Constructor Dependency Injection**: Receives graphics dependencies during construction.
    * **`template<typename T> std::shared_ptr<T> GetResource(const std::string& id)`**: Primary resource retrieval method.
    * **Lifetime Management**: Uses `std::shared_ptr` with explicit unload methods available.

### 6.2. Concrete Resource Implementations

#### 6.2.1. Texture Resources
* **`TextureResource`**: Manages loaded textures.
    * Integrates with `TextureManager` for GPU resource creation.
    * Supports multiple image formats through DirectXTex integration.
    * Handles CPU-to-GPU data transfer with upload heap management.

#### 6.2.2. Mesh Resources (**NEW**)
* **`MeshResource`**: **NEW** - Manages loaded 3D meshes.
    * Integrates with `MeshManager` for GPU buffer creation.
    * Supports OBJ file format with extensible loader architecture.
    * Flexible vertex layout system (P, PN, PNT, PNTC, PNTCB, PNTTB).
    * Optional CPU data caching for collision detection and physics.
    * Future-ready for FBX support via ufbx library.

### 6.3. Mesh Loading Pipeline (**NEW**)

#### 6.3.1. Vertex Layout System
* **Flexible Vertex Attributes**: Support for Position, Normal, TexCoord, Color, Bone data.
* **Predefined Layouts**: Common combinations (PNT, PNTC, etc.) with extensibility.
* **Type-Safe Attributes**: DXGI format mapping and D3D12 input layout generation.
* **Memory Optimization**: Interleaved vertex data for optimal GPU cache performance.

#### 6.3.2. OBJ Loader
* **Complete OBJ Support**: Vertices, normals, texture coordinates, faces, materials.
* **Robust Parsing**: Handles triangles, quads (auto-triangulated), and n-gons.
* **MTL Material Support**: Diffuse, normal, and specular texture loading.
* **Vertex Deduplication**: Eliminates duplicate vertices for optimal GPU usage.
* **Error Handling**: Detailed error messages with line number reporting.

#### 6.3.3. CPU Mesh Data (`MeshData`)
* **Flexible Storage**: Raw interleaved vertex buffer with attribute descriptors.
* **Validation**: Comprehensive mesh data validation methods.
* **Bounding Information**: Automatic AABB and bounding sphere calculation.
* **Material Support**: Embedded material definitions for future expansion.

#### 6.3.4. GPU Mesh Management (`GPUMesh`)
* **D3D12 Integration**: Proper buffer creation with state transitions.
* **Resource Views**: Pre-configured vertex and index buffer views.
* **Memory Tracking**: Statistics and validation for debugging.
* **Upload Heap Management**: Temporary heap cleanup after GPU completion.

## 7. Asset Bundle System

### 7.1. Bundle Definition (YAML)
The engine uses YAML-based bundle definitions for asset management:

```yaml
bundle:
  name: "Core_Assets"
  priority: 0
  auto_load: true
  unload_strategy: "manual"

assets:
  - type: "texture"
    id: "ui/button_normal"
    path: "textures/ui/button_normal.png"
    priority: 0
    
  - type: "mesh"
    id: "props/barrel"
    path: "meshes/barrel.obj"
    vertex_layout: "PNT"
    priority: 5
```

### 7.2. Bundle Integration
* **Unified Loading**: Both textures and meshes use the same bundle system.
* **Priority Management**: Asset loading priority for memory management.
* **Automatic Loading**: Engine-level assets loaded automatically.
* **Extensible Format**: Easy to add new asset types (audio, materials, etc.).

## 8. C++23 Modules Strategy

The engine leverages C++23 Modules for improved build times and better encapsulation:

### 8.1. Module Usage Patterns
* **`.ixx` Module Interface Units**: Used for module declarations and exports.
* **`.cpp` Module Implementation Units**: Used for implementation details.
* **`.hpp` Headers**: Used for public interfaces, shared data structures, and interop.

### 8.2. Module Organization
* **Core Modules**: `Angaraka.Core.Events`, `Angaraka.Core.Resources`, `Angaraka.Core.Config`
* **Graphics Modules**: Individual managers as separate modules for better isolation.
* **System Modules**: Input, Camera, and other subsystems as focused modules.

## 9. Current System Capabilities

### 9.1. Rendering Features
* **Multi-Object Rendering**: Support for multiple meshes with independent transforms.
* **Flexible Vertex Layouts**: Runtime vertex format selection based on asset needs.
* **Texture Mapping**: Complete texture loading and binding pipeline.
* **Camera Control**: FPS-style camera with smooth movement and mouse look.

### 9.2. Asset Pipeline
* **OBJ Mesh Loading**: Complete OBJ file support with material parsing.
* **Image Format Support**: WIC, TGA, HDR, DDS texture formats.
* **Bundle Management**: YAML-based asset organization and loading.
* **Resource Caching**: Automatic deduplication and lifetime management.

### 9.3. Development Features
* **D3D12 Debugging**: Comprehensive object naming and validation.
* **Memory Tracking**: Resource usage statistics and leak detection.
* **Error Handling**: Detailed logging with proper error recovery.
* **Hot Reloading Ready**: Architecture supports runtime asset reloading.

## 10. Future Expansion Points

### 10.1. Advanced Graphics (Next Priority)
* **Scene Management**: Multiple mesh instances, transform hierarchy, frustum culling.
* **Material System**: PBR materials, shader permutations, texture atlasing.
* **Lighting**: Point, directional, and spot lights with shadow mapping.
* **Post-Processing**: Tone mapping, bloom, anti-aliasing.

### 10.2. Animation and Physics
* **Skeletal Animation**: Bone hierarchies, animation clips, blending.
* **Physics Integration**: Collision detection, rigid body dynamics.
* **Particle Systems**: GPU-based particle simulation and rendering.

### 10.3. Audio and AI
* **3D Audio**: Spatial audio with HRTF and environmental effects.
* **AI Framework**: Behavior trees, pathfinding, decision making.
* **Scripting**: Lua or C# integration for gameplay logic.

### 10.4. Tools and Editor
* **In-Engine Editor**: Scene editing, asset browser, property inspector.
* **Asset Pipeline**: Automated asset processing and optimization.
* **Profiling Tools**: Performance analysis and bottleneck identification.

## 11. Best Practices & Utilities

### 11.1. Error Handling & Logging
* **Structured Logging**: `AGK_INFO`, `AGK_WARN`, `AGK_ERROR`, `AGK_CRITICAL` macros.
* **DirectX Error Handling**: `DXCall` macro for `HRESULT` validation.
* **D3D12 Object Naming**: `NAME_D3D12_OBJECT` macro for debugging support.

### 11.2. Memory Management
* **Smart Pointers**: Consistent use of `std::unique_ptr` and `std::shared_ptr`.
* **RAII Principles**: Automatic resource cleanup through destructors.
* **Upload Heap Management**: Temporary resource cleanup after GPU completion.

### 11.3. Threading and Synchronization
* **Thread-Safe Design**: Mutex protection for shared data structures.
* **GPU Synchronization**: Fence-based CPU/GPU coordination.
* **Event System**: Asynchronous communication between subsystems.

---

**Current Status**: The engine has a complete mesh loading and rendering pipeline with bundle management, flexible vertex layouts, and production-ready resource management. The foundation is solid for expanding into advanced graphics features, scene management, and tool development.