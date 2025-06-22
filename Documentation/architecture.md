# Angaraka Engine Architecture Overview

## 1. Introduction

The Angaraka Engine is a modern C++ game engine designed for 3D graphics, leveraging the latest C++23 features and DirectX12. Our core philosophy emphasizes modularity, performance, clear separation of concerns, and maintainable, production-ready code.

**Target Game**: First-person RPG inspired by ancient hindu stories and mythology - set in 6400 AD.
**Architecture Strategy**: Immediate pivot capability for third-person perspective when team scales
**Development Approach**: Build engine MVP alongside RPG game systems for rapid iteration

## 2. Core Principles

* **Modern C++ (C++23):** Embracing new language features for cleaner, safer, and more expressive code.
* **Modularity:** Strong division of responsibilities among components and subsystems, facilitated by C++23 Modules for implementation units and traditional .hpp headers for public interfaces.
* **Dependency Injection (DI):** Minimizing global state and tight coupling by passing dependencies (like manager pointers or contexts) through constructors or explicit methods, enhancing testability and flexibility.
* **Performance:** Designing with efficiency in mind, especially for graphics and resource management.
* **Clarity & Maintainability:** Prioritizing readability, consistent naming conventions, and comprehensive logging.
* **Resource Ownership:** Strict use of smart pointers (`std::unique_ptr`, `std::shared_ptr`) for clear ownership semantics and automatic memory management.
* **Game-Engine Co-development:** Building engine features driven by immediate game requirements for rapid iteration and practical validation.
* **Scalability Architecture:** Designed to support team growth and pivot from first-person to third-person perspective without major rewrites.

## 3. Top-Level Structure & Game Integration

The engine is primarily composed of an `Application` layer that orchestrates various managers and subsystems, designed to support the target RPG game requirements.

* **`Angaraka::Application`**:
    * The main entry point and orchestrator of the engine lifecycle (Initialization, Update loop, Render loop, Shutdown).
    * Responsible for creating and managing the lifetime of top-level managers like `EventManager`, `InputManager`, `DirectX12GraphicsSystem`, and `ResourceManager`.
    * Integrates with RPG-specific systems for immediate game development capability.

## 4. Target Game Requirements

### Game Vision: "Bioshock Infinite meets Skyrim" (6400 AD Setting)
* **Core Perspective**: First-person with third-person pivot capability
* **World Structure**: Multiple connected interior zones (Bioshock-style interiors)
* **Combat System**: Hybrid ranged/magic combat with simple projectile mechanics
* **Progression**: Skyrim-style skill trees, world scaling to player level
* **Narrative**: Live NPCs with dialogue trees, linear story progression
* **Interaction**: Device activation puzzles, NPC conversations, visual inventory with item icons
* **Asset Strategy**: Marketplace assets (no custom art team)
* **Audio**: Text-only dialogue with environmental audio for immersion

### Immediate Pivot Strategy (Team Scaling)
When team grows, engine supports transition to:
* **Third-person perspective** (Skyrim/Witcher style)
* **Character animation systems** for visible player character
* **Larger outdoor zones** with streaming capability
* **Advanced lighting and weather** for open-world feel

## 5. Core Subsystems

### 5.1. Event System (`Angaraka.Core.Events`)

* **`EventManager`**: A central pub-sub (publisher-subscriber) system.
    * Decouples components by allowing them to communicate asynchronously through events.
    * Used for system-wide notifications (e.g., window resize, input events, resource loaded events).

### 5.2. Input System (`Angaraka.Core.Input`)

* **`InputManager`**: Handles raw input from keyboard and mouse.
    * Translates platform-specific input events into engine-agnostic events published on the `EventManager`.

### 5.3. Camera System (`Angaraka.Camera`)

* **`Camera`**: Manages view and projection matrices with dual-mode support.
    * **First-Person Mode**: Direct view matrix for immersive RPG experience.
    * **Third-Person Mode**: Orbiting camera for character visibility (pivot capability).
    * **Runtime Switching**: Seamless transition between camera modes for development flexibility.
    * Handles camera movement (position, pitch, yaw) and projection properties (FOV, aspect ratio, near/far planes).
    * Provides methods to retrieve `XMMATRIX` for view and projection transformations.
    * **RPG Integration**: Supports interaction raycasting and UI overlay positioning.

## 6. Graphics Subsystem (`Angaraka.Graphics.DirectX12`)

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
    * **MeshManager**: Handles creation and management of GPU mesh resources.
        * Loads OBJ files with flexible vertex layouts (P, PN, PNT, PNTC, PNTCB, PNTTB).
        * Creates vertex and index buffers for GPU rendering.
        * Supports static mesh rendering for environments and characters.
        * **RPG Integration**: Optimized for interior scene rendering with moderate polygon counts.
    * **Scene Management**: Multi-object rendering system for game worlds.
        * **SceneManager**: Container for multiple renderable objects with individual transforms.
        * **Transform Hierarchy**: Parent-child relationships for complex scene organization.
        * **Culling System**: Frustum culling for performance in larger scenes.
        * **Zone Loading**: Supports multiple connected areas for RPG world structure.

## 7. Resource Management System (`Angaraka.Core.Resources`)

This system provides a centralized, type-safe, and cache-driven approach to managing all engine assets. It is designed to be highly extensible and avoid problematic global state.

* **Core Principle**: Resources are loaded once, cached, and their lifetime is managed via `std::shared_ptr`. Dependencies required for loading (e.g., DirectX12 context) are injected.

* **`Resource` Base Class (`Angaraka/Core/Resources/Resource.hpp`)**:
    * An abstract interface (`virtual bool Load(...)`, `virtual void Unload()`) for all managed assets (textures, meshes, materials, etc.).
    * Each resource has a unique `id` (typically its file path).
    * Utilizes `AGK_RESOURCE_TYPE_ID` macro for runtime type identification, enabling generic resource handling while maintaining type safety.

* **`ResourceManager` (`Angaraka.Core.Resources.ResourceManager`)**:
    * The central cache and coordinator for `Resource` objects.
    * **Constructor Dependency Injection**: Receives a `std::shared_ptr<Angaraka::Graphics::GraphicsContext>` during its construction. This `GraphicsContext` contains all necessary graphics-related dependencies (`ID3D12Device`, `ID3D12GraphicsCommandList`, `TextureManager`, etc.).
    * **`template<typename T> std::shared_ptr<T> GetResource(const std::string& id)`**: The primary method for retrieving resources.
        * Checks its internal cache (`m_loadedResources`). If found and type matches, returns the cached instance.
        * If not found, creates a new `std::make_shared<T>(id)` instance.
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
        * Its `Load(const std::string& filePath, const Angaraka::Graphics::GraphicsContext& graphicsContext)` method utilizes `graphicsContext.pTextureManager` to delegate the actual GPU-side texture creation to the `TextureManager`, effectively removing any global dependency.
        * Stores the `std::shared_ptr<Texture>` returned by `TextureManager` internally.
    * **`MeshResource`**: Complete mesh resource support for OBJ file loading and GPU buffer creation.
    * **Future Resources**: This pattern will extend to `MaterialResource`, `ShaderResource`, `AudioResource`, etc., each taking the `GraphicsContext` to interact with their respective managers.

* **RPG Asset Integration**:
    * **Bundle System**: YAML-defined asset collections for zone loading.
    * **Priority Loading**: Critical assets (player character, UI) vs. background assets.
    * **Marketplace Asset Pipeline**: Optimized for loading external assets without custom toolchain.

## 8. Game Systems Integration

### RPG-Specific Systems (Built on Engine Foundation)
* **Character System**:
    * **PlayerCharacter**: Mesh rendering, transform management, controller integration
    * **NPCs**: Static mesh placement with interaction triggers
    * **Dual Perspective Support**: Character visibility management for first/third-person modes

* **Interaction System**:
    * **Device Activation**: Raycast-based interaction with world objects
    * **NPC Dialogue**: Trigger volumes and UI integration
    * **Inventory Items**: Visual item representation and collection mechanics

* **Zone Management**:
    * **Scene Loading**: Multiple connected interior areas
    * **Transition System**: Seamless zone changes with asset streaming
    * **Environmental Audio**: Spatial audio for immersion

* **UI Framework** (Planned):
    * **Dialogue Trees**: Text-based conversation system
    * **Inventory Interface**: Visual item icons and equipment slots
    * **HUD Elements**: Health, skill progress, interaction prompts

### Scalability for Team Growth
* **Character Animation**: Skeletal animation system for visible characters
* **Advanced Lighting**: Dynamic lighting for open-world environments  
* **Audio Systems**: 3D spatial audio and music management
* **AI Framework**: Behavior trees and pathfinding for NPCs
* **Editor Integration**: Real-time scene editing and asset preview

## 9. C++23 Modules Strategy

The engine leverages C++23 Modules for improved build times and better encapsulation:

* **`.hpp` Headers**: Used for public interfaces, declarations, and data structures that need to be shared across module boundaries or with traditional compilation units. These typically contain `export` directives for module interfaces, or are included in global module fragments.
* **`.cpp` Files (Module Implementation Units)**: Used for the actual implementation of classes and functions belonging to a module. These files typically `import` other modules or `#include` local `.hpp` files, but do not necessarily `export` anything themselves, unless they are the primary unit for a named module. This allows for faster, independent compilation of implementation details.

This hybrid approach balances the benefits of modules (encapsulation, faster compilation) with the practicality of existing C++ tooling and the need for public header-only definitions.

## 10. Development Roadmap & Milestones

### Current Status: Engine MVP Complete ✅
- Core rendering pipeline with mesh and texture loading
- Resource management with caching
- Input system with event integration  
- Basic camera and window management

### Phase 1: Scene Management (Week 1) 🚧
- Multi-object rendering for populated game zones
- Transform management for object placement
- Basic interaction system for NPCs and devices

### Phase 2: Character Integration (Week 2) 📅
- Player character rendering (third-person pivot capability)
- Dual camera system (first-person + third-person)
- Character controller for both perspectives

### Phase 3: Game Systems (Week 3-4) 📅
- UI framework for dialogue and inventory
- Visual inventory with item icons
- Zone transition and loading system
- Basic audio integration

### Phase 4: RPG Foundation (Month 2) 📅
- Dialogue system with branching conversations
- Skill progression mechanics
- Equipment and upgrade systems
- Save/load functionality

### Future Expansion (Team Scaling) 📅
- Character animation pipeline
- Advanced lighting and weather
- AI systems and pathfinding
- Full editor integration

## 11. Best Practices & Utilities

* **Error Handling & Logging**: Extensive use of `AGK_INFO`, `AGK_WARN`, `AGK_ERROR`, `AGK_CRITICAL` macros for structured logging.
* **DirectX Error Handling**: `DXCall` macro is used to check `HRESULT` return values and report errors.
* **D3D12 Object Naming**: `NAME_D3D12_OBJECT` macro for debugging purposes, allowing easy identification of DirectX12 resources in tools like PIX.
* **Smart Pointers**: Consistent use of `std::unique_ptr` for exclusive ownership and `std::shared_ptr` for shared ownership, preventing memory leaks and simplifying resource management.

---