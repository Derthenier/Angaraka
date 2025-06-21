# Enhanced Asset Management Architecture

## Asset Bundle System

### Bundle Structure
```yaml
bundles:
  level_castle:
    priority: "High"              # Critical, High, Medium, Low
    preload: true                 # Load immediately on level start
    persistent: false             # Can be unloaded under memory pressure
    memory_budget_mb: 128         # Maximum memory allocation
    streaming_distance: 1000.0   # Distance-based loading threshold
    assets: ["castle_walls", "stone_texture", "wood_material"]
    
  ui_global:
    priority: "Critical"
    preload: true
    persistent: true              # Never unload
    memory_budget_mb: 32
    assets: ["ui_button", "ui_background", "ui_font"]

  characters_common:
    priority: "High"
    preload: false
    streaming_distance: 500.0
    lod_strategy: "distance_based"
    assets: ["player_mesh", "npc_base", "weapon_sword"]
```

### Asset Definitions
```yaml
assets:
  castle_walls:
    path: "meshes/castle_walls.obj"
    type: "mesh"
    bundle: "level_castle"
    priority: "High"
    dependencies: ["stone_material", "wood_material"]
    lod_variants:
      - path: "meshes/castle_walls_lod0.obj"  # Highest quality
        distance: 0.0
      - path: "meshes/castle_walls_lod1.obj"  # Medium quality
        distance: 200.0
      - path: "meshes/castle_walls_lod2.obj"  # Lowest quality
        distance: 500.0
    
  stone_texture:
    path: "textures/stone_diffuse.png"
    type: "texture"
    bundle: "level_castle"
    priority: "Medium"
    compression: "BC7"
    mip_levels: 8
    variants:
      high: "textures/stone_diffuse_4k.png"
      medium: "textures/stone_diffuse_2k.png"
      low: "textures/stone_diffuse_1k.png"
```

## Priority System

### Asset Priority Levels
```cpp
enum class AssetPriority : U8 {
    Critical = 0,    // UI elements, player character - never evicted
    High = 1,        // Current level geometry, active NPCs - rarely evicted
    Medium = 2,      // Background objects, distant geometry - evicted under pressure
    Low = 3          // Streaming content, far LODs - first to evict
};
```

### Bundle Priority Properties
- **Critical**: Loaded immediately, never unloaded, reserved memory pool
- **High**: Preloaded, low eviction priority, performance-critical assets
- **Medium**: On-demand loading, standard eviction priority
- **Low**: Streaming/background loading, first eviction candidates

## Enhanced Staging System

### Staging Loader Architecture
```cpp
class StagingLoader {
public:
    // Bundle-based loading
    void QueueBundle(const std::string& bundleId, AssetPriority overridePriority = AssetPriority::Medium);
    void UnloadBundle(const std::string& bundleId);
    
    // Individual asset loading with priority
    void QueueAsset(const std::string& assetId, AssetPriority priority);
    
    // Streaming controls
    void SetStreamingCenter(const DirectX::XMVECTOR& position);
    void UpdateStreamingPriorities();
    
private:
    PriorityQueue<AssetLoadRequest> m_loadQueue;
    std::unordered_map<std::string, BundleState> m_bundleStates;
    std::thread_pool m_ioThreads;
};
```

### Staging Cache with Bundle Awareness
```cpp
struct StagingEntry {
    std::string assetId;
    std::string bundleId;
    AssetPriority priority;
    std::unique_ptr<RawAssetData> data;
    std::chrono::time_point<std::chrono::steady_clock> loadTime;
    std::vector<std::string> dependencies;
};

class StagingCache {
public:
    void Insert(const std::string& assetId, StagingEntry entry);
    std::optional<StagingEntry> Extract(const std::string& assetId);
    
    // Bundle operations
    void EvictBundle(const std::string& bundleId);
    size_t GetBundleMemoryUsage(const std::string& bundleId) const;
    
private:
    std::unordered_map<std::string, StagingEntry> m_entries;
    std::unordered_map<std::string, std::vector<std::string>> m_bundleAssets;
    mutable std::shared_mutex m_mutex;
};
```

## GPU Upload Manager Integration

### Enhanced Upload Strategy
```cpp
class GPUUploadManager {
public:
    // Priority-based upload processing
    void ProcessUploads(size_t maxUploadsPerFrame = 4);
    
    // Bundle upload coordination
    void UploadBundle(const std::string& bundleId);
    bool IsBundleReady(const std::string& bundleId) const;
    
    // Memory management
    void SetUploadBudget(size_t budgetBytes);
    void FlushPendingUploads();
    
private:
    struct UploadRequest {
        std::string assetId;
        AssetPriority priority;
        std::unique_ptr<StagingEntry> stagingData;
        std::function<void()> completionCallback;
    };
    
    PriorityQueue<UploadRequest> m_uploadQueue;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_uploadHeaps;
    size_t m_uploadBudget;
};
```

## Memory Management Strategy

### Bundle Memory Budgets
- **Per-bundle limits**: Prevent single bundle from consuming all memory
- **Priority-based eviction**: Critical bundles protected, Low priority evicted first
- **Global memory pressure**: System-wide limits with bundle-aware eviction

### LRU Cache Integration
```cpp
class EnhancedResourceManager {
public:
    // Bundle-aware resource retrieval
    template<typename T>
    std::shared_ptr<T> GetResource(const std::string& assetId, 
                                   void* context, 
                                   AssetPriority overridePriority = AssetPriority::Medium);
    
    // Bundle management
    void LoadBundle(const std::string& bundleId);
    void UnloadBundle(const std::string& bundleId, bool force = false);
    
private:
    // Priority-aware eviction policy
    void EvictByPriority(size_t bytesToFree);
    void UpdateAccessPriority(const std::string& assetId, AssetPriority priority);
};
```

## Streaming and LOD System

### Distance-Based Loading
```cpp
class StreamingManager {
public:
    void SetViewerPosition(const DirectX::XMVECTOR& position);
    void UpdateStreamingState();
    
private:
    struct StreamingZone {
        std::string bundleId;
        BoundingSphere bounds;
        float streamingDistance;
        AssetPriority priority;
    };
    
    std::vector<StreamingZone> m_streamingZones;
    DirectX::XMVECTOR m_viewerPosition;
};
```

### LOD Management
- **Distance-based selection**: Automatic LOD switching based on viewer distance
- **Memory-based selection**: Lower LODs during memory pressure
- **Bundle integration**: LOD variants grouped within bundles

## Configuration Integration

### Enhanced YAML Configuration
```yaml
renderer:
  resource_cache:
    max_memory_mb: 1024
    bundle_budgets:
      critical: 128     # Reserved for Critical priority bundles
      high: 512         # Maximum for High priority bundles
      medium: 256       # Standard allocation
      low: 128          # Streaming content
    
    eviction_policy:
      enable_bundle_protection: true
      critical_never_evict: true
      eviction_order: ["Low", "Medium", "High"]  # Eviction priority order
    
    streaming:
      enable_distance_streaming: true
      update_frequency_ms: 100
      prediction_distance: 1000.0
      
asset_manifest:
  path: "config/assets.yaml"
  enable_hot_reload: true
  bundle_validation: true
```

## Implementation Timeline

### Phase 1: Asset Manifest (Week 1)
- Bundle configuration parsing
- Asset dependency resolution
- Priority system implementation

### Phase 2: Staging System (Week 2)
- Priority-based I/O queue
- Bundle-aware staging cache
- Background loading threads

### Phase 3: GPU Integration (Week 3)
- Enhanced upload manager
- Bundle upload coordination
- Memory budget enforcement

### Phase 4: Streaming (Weeks 4-5)
- Distance-based loading
- LOD system integration
- Performance optimization