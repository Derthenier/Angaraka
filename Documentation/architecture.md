# Angaraka Engine Architecture Overview

## 1. Introduction

The Angaraka Engine is a modern C++ game engine designed for 3D graphics, leveraging the latest C++23 features and DirectX12. Our core philosophy emphasizes modularity, performance, clear separation of concerns, and maintainable, production-ready code.

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

### 4.2. Input System (`Angaraka.Core.Input`)

* **`InputManager`**: Handles raw input from keyboard and mouse.
    * Translates platform-specific input events into engine-agnostic events published on the `EventManager`.

### 4.3. Camera System (`Angaraka.Camera`)

* **`Camera`**: Manages the view and projection matrices.
    * Handles camera movement (position, pitch, yaw) and projection properties (FOV, aspect ratio, near/far planes).
    * Provides methods to retrieve `XMMATRIX` for view and projection transformations.

## 5. Graphics Subsystem (`Angaraka.Graphics.DirectX12`)

The graphics subsystem is built around DirectX12 and is highly modular, with specialized managers for different aspects of the rendering pipeline.

* **`DirectX12GraphicsSystem` (Renderer)**:
    * The primary interface for rendering operations.
    * Orchestrates the DirectX12 device, command management, swap chain, and scene rendering.
    * Manages the lifetime and initialization of core DirectX12 managers.
    * **Dependency Injection for Internal Managers**: Creates and provides necessary components (like `TextureManager`) to the `ResourceManager` through the `GraphicsContext`.
    * Handles window resize events by coordinating with relevant managers.

* **Sub-Managers (Injected Dependencies)**:
    * **`DeviceManager`**: Manages the `ID3D12Device` and associated debug layers.
    * **`CommandManager`**: Manages `ID3D12CommandAllocator`, `ID3D12GraphicsCommandList`, and `ID3D12CommandQueue` for issuing GPU commands. Includes synchronization mechanisms (fences).
    * **`SwapChainManager`**: Manages the `IDXGISwapChain3` and its associated render target views (RTVs). Handles presentation and resizing.
    * **`PSOManager`**: Manages `ID3D12PipelineState` objects and `ID3D12RootSignature` for rendering pipelines. Compiles shaders and defines input layouts.
    * **`BufferManager`**: Handles creation and management of GPU buffers:
        * **Vertex Buffers**: Stores per-vertex data (position, color, UV).
        * **Index Buffers**: Stores indices for indexed drawing.
        * **Constant Buffers**: Stores uniform data (e.g., MVP matrix) for shaders, mapped to CPU memory for efficient updates.
    * **`TextureManager`**: Manages GPU-side texture resources.
        * Responsible for creating `ID3D12Resource` objects for textures and their corresponding Shader Resource Views (SRVs).
        * Utilizes external libraries (e.g., DirectXTex) to process raw image data.
        * Crucially, it **does not** handle file I/O directly; it receives `ImageData` from the `TextureResource` (which loads from disk).

## 6. Resource Management System (`Angaraka.Core.Resources`)

This system provides a centralized, type-safe, and cache-driven approach to managing all engine assets. It is designed to be highly extensible and avoid problematic global state.

* **Core Principle**: Resources are loaded once, cached, and their lifetime is managed via `std::shared_ptr`. Dependencies required for loading (e.g., DirectX12 context) are injected.

* **`Resource` Base Class (`Angaraka/Core/Resources/Resource.hpp`)**:
    * An abstract interface (`virtual bool Load(...)`, `virtual void Unload()`) for all managed assets (textures, meshes, materials, etc.).
    * Each resource has a unique `id` (typically its file path).
    * Utilizes `AGK_RESOURCE_TYPE_ID` macro for runtime type identification, enabling generic resource handling while maintaining type safety.

* **`ResourceManager` (`Angaraka.Core.Resources.ResourceManager`)**:
    * The central cache and coordinator for `Resource` objects.
    * **Constructor Dependency Injection**: Receives a `Reference<Angaraka::Graphics::GraphicsContext>` during its construction. This `GraphicsContext` contains all necessary graphics-related dependencies (`ID3D12Device`, `ID3D12GraphicsCommandList`, `TextureManager`, etc.).
    * **`template<typename T> Reference<T> GetResource(const String& id)`**: The primary method for retrieving resources.
        * Checks its internal cache (`m_loadedResources`). If found and type matches, returns the cached instance.
        * If not found, creates a new `CreateReference<T>(id)` instance.
        * **Delegated Loading**: Calls `newResource->Load(id, *m_graphicsContext)` to trigger the actual loading process, passing down the necessary `GraphicsContext`.
        * Caches the newly loaded resource.
    * **Lifetime Management**: Uses `std::shared_ptr` within its `m_loadedResources` map. Resources are kept alive as long as the `ResourceManager` or any other part of the engine holds a `std::shared_ptr` to them. Explicit `UnloadResource` and `UnloadAllResources` methods are also available.

* **`GraphicsContext` (`Angaraka/Graphics/GraphicsContext.hpp`)**:
    * A lightweight `struct` that acts as a container for pointers to core graphics system components (`ID3D12Device*`, `ID3D12GraphicsCommandList*`, `TextureManager*`, etc.).
    * This struct is created and populated by `DirectX12GraphicsSystem` during its initialization.
    * It is then passed to the `ResourceManager` constructor and subsequently to the `Load()` methods of individual `Resource` objects, eliminating the need for global accessors.

* **Concrete `Resource` Implementations**:
    * **`TextureResource` (`Angaraka.Graphics.DirectX12.TextureResource`)**:
        * Derives from `Angaraka::Core::Resources::Resource`.
        * Its `Load(const String& filePath, const Angaraka::Graphics::GraphicsContext& graphicsContext)` method utilizes `graphicsContext.pTextureManager` to delegate the actual GPU-side texture creation to the `TextureManager`, effectively removing any global dependency.
        * Stores the `Reference<Texture>` returned by `TextureManager` internally.
    * **Future Resources**: This pattern will extend to `MeshResource`, `MaterialResource`, `ShaderResource`, etc., each taking the `GraphicsContext` to interact with their respective managers.

## 7. C++23 Modules Strategy

The engine leverages C++23 Modules for improved build times and better encapsulation:

* **`.hpp` Headers**: Used for public interfaces, declarations, and data structures that need to be shared across module boundaries or with traditional compilation units. These typically contain `export` directives for module interfaces, or are included in global module fragments.
* **`.cpp` Files (Module Implementation Units)**: Used for the actual implementation of classes and functions belonging to a module. These files typically `import` other modules or `#include` local `.hpp` files, but do not necessarily `export` anything themselves, unless they are the primary unit for a named module. This allows for faster, independent compilation of implementation details.

This hybrid approach balances the benefits of modules (encapsulation, faster compilation) with the practicality of existing C++ tooling and the need for public header-only definitions.

## 8. Best Practices & Utilities

* **Error Handling & Logging**: Extensive use of `AGK_INFO`, `AGK_WARN`, `AGK_ERROR`, `AGK_CRITICAL` macros for structured logging.
* **DirectX Error Handling**: `DXCall` macro is used to check `HRESULT` return values and report errors.
* **D3D12 Object Naming**: `NAME_D3D12_OBJECT` macro for debugging purposes, allowing easy identification of DirectX12 resources in tools like PIX.
* **Smart Pointers**: Consistent use of `std::unique_ptr` for exclusive ownership and `std::shared_ptr` for shared ownership, preventing memory leaks and simplifying resource management.

---
