# UE5-Inspired UI Rendering Architecture

## Overview

This is a completely new UI rendering backend inspired by Unreal Engine 5's Slate rendering architecture. It provides a modular, retained-mode renderer with complete independence from the scene rendering pipeline.

## Architecture

### Rendering Pipeline

```
UI Build (Widget Tree)
↓
Geometry Cache (Retained Mode)
↓
Batch Generator (Draw Call Optimization)
↓
Texture Atlas Manager (Texture Packing)
↓
UI Command Buffer (Command Recording)
↓
Dedicated UI Renderer (Independent Pipeline)
↓
UI Compositor (Final Overlay)
↓
Present
```

## Core Components

### 1. UIRenderer2 (Main Renderer)

**File:** `UIRenderer2.hpp/cpp`

The main renderer class that orchestrates all UI rendering. It owns all Vulkan resources and is completely independent from the scene renderer.

**Key Features:**
- Owns graphics pipeline, pipeline layout, descriptor sets
- Manages per-frame geometry buffers
- Handles Vulkan state save/restore
- Provides statistics and debugging info

**Initialization:**
```cpp
UIRenderer2 renderer;
renderer.Init(
    physicalDevice,
    logicalDevice,
    graphicsQueue,
    graphicsQueueFamilyIndex,
    swapchainFormat,
    maxFramesInFlight
);
```

**Frame Rendering:**
```cpp
renderer.BeginFrame(frameIndex, width, height);
renderer.RenderWidget(rootWidget);
renderer.EndFrame(cmd, swapchainImageView);
```

### 2. UIGeometryCache (Retained-Mode Caching)

**File:** `UIGeometryCache.hpp/cpp`

Implements retained-mode rendering with geometry caching to reduce CPU usage.

**Key Features:**
- Caches vertex/index data per widget
- LRU eviction policy
- Dirty region tracking
- Cache statistics (hit/miss rates)

**Usage:**
```cpp
// Check cache
if (cache.GetGeometry(key, vertices, indices)) {
    // Cache hit - use cached geometry
} else {
    // Cache miss - generate new geometry
    cache.CacheGeometry(key, vertices, indices, frameIndex);
}
```

### 3. UIBatchGenerator (Draw Call Optimization)

**File:** `UIBatchGenerator.hpp/cpp`

Groups draw commands into efficient batches to minimize state changes.

**Key Features:**
- Merges compatible batches (same texture, scissor, stencil)
- Reduces draw calls
- Sorts by texture for better cache locality

**Batch Merging Criteria:**
- Same texture descriptor set
- Same scissor rectangle
- Same stencil reference

### 4. UITextureAtlasManager (Texture Packing)

**File:** `UITextureAtlasManager.hpp/cpp`

Manages a texture atlas for efficient texture sampling.

**Key Features:**
- Dynamic atlas expansion
- First-fit slot allocation
- UV coordinate conversion
- Atlas utilization tracking

**Usage:**
```cpp
uint32_t textureId = atlasManager.RegisterTexture(imageView, sampler, width, height);
float u0, v0, u1, v1;
atlasManager.GetTextureUV(textureId, u0, v0, u1, v1);
```

### 5. UICommandBuffer (Command Recording)

**File:** `UICommandBuffer.hpp/cpp`

Manages command buffer recording and geometry buffer updates.

**Key Features:**
- Per-frame geometry buffers
- Staging buffers for data upload
- Automatic buffer resizing
- Draw call tracking

### 6. UICompositor (Final Overlay)

**File:** `UICompositor.hpp/cpp`

Performs the final UI overlay as the last graphics pass before present.

**Key Features:**
- Uses dynamic rendering (no traditional render pass)
- Loads existing scene content
- Composites UI on top
- Manages viewport and scissor state

### 7. UIStateManager (Vulkan State Isolation)

**File:** `UIStateManager.hpp/cpp`

Saves and restores Vulkan state to ensure the UI renderer leaves no state behind.

**Key Features:**
- Saves pipeline, descriptor sets, viewport, scissor
- Restores all dynamic state
- Validates state before restoration

**Usage:**
```cpp
SavedVulkanState state;
stateManager.SaveState(cmd, state);
// ... UI rendering ...
stateManager.RestoreState(cmd, state);
```

### 8. UIWidgetAdapter (Widget System Integration)

**File:** `UIWidgetAdapter.hpp/cpp`

Bridges the existing widget system with the new renderer.

**Key Features:**
- Converts PaintContext/DrawCommand to UIRenderer2 format
- Handles all draw command types (rect, text, texture, icon, line, shadow, gradient, rounded outline)
- Maintains compatibility with existing widget API

**Usage:**
```cpp
UIWidgetAdapter adapter;
adapter.Initialize(&renderer);
adapter.ProcessWidget(rootWidget, width, height);
const auto& vertices = adapter.GetVertices();
const auto& indices = adapter.GetIndices();
const auto& batches = adapter.GetBatches();
```

## Shaders

### UI2.hlsl

**File:** `Engine/Shaders/Rendering/UI2.hlsl`

New shaders completely independent from scene rendering.

**Features:**
- Vertex shader with screen-space transformation
- Fragment shader with SDF-based rounded rectangles
- Textured quad support (text, icons, images)
- Anti-aliased rendering with smoothstep
- No HDR, tone mapping, or post-processing

**Draw Types:**
- Type 0: Textured quad (samples texture)
- Type 1: SDF rounded rectangle (solid UI chrome)
- Type 2: SDF rounded outline (borders)

## Key Design Principles

### 1. Complete Independence

The UI renderer owns all its resources:
- Graphics pipeline
- Pipeline layout
- Descriptor layouts
- Descriptor pool
- Texture atlas
- Font atlas
- Vertex/index buffers
- Command recording

No shared state with the scene renderer.

### 2. State Isolation

The UI renderer:
- Saves all Vulkan state before rendering
- Restores all state after rendering
- Leaves no state behind
- Uses dynamic rendering for clean pass boundaries

### 3. Retained-Mode Rendering

- Geometry is cached per widget
- Only dirty regions are updated
- Reduces CPU overhead for static UI
- LRU eviction manages memory

### 4. Modular Architecture

Each component is:
- Self-contained
- Replaceable
- Testable
- Documented

### 5. Performance Optimizations

- Batch merging reduces draw calls
- Texture atlas reduces descriptor set changes
- Geometry caching reduces CPU work
- Dirty regions minimize updates

## Render Order

The correct render order is maintained:

1. **Scene Renderer** (via RenderGraph)
2. **Post Processing** (via RenderGraph)
3. **Tone Mapping** (via RenderGraph)
4. **UI Renderer** (independent, via UIRenderer2)
5. **Present**

The UI renderer is called after all scene rendering is complete, ensuring it composites on top of the final scene output.

## Integration with Existing Widget System

The `UIWidgetAdapter` provides seamless integration:

1. Existing widgets continue to use `PaintContext`
2. Adapter converts paint commands to renderer format
3. No changes required to widget code
4. Preserves all existing functionality

## Supported UI Elements

- Windows
- Docking panels
- Menus
- Toolbars
- Viewports
- Icons
- Fonts (text rendering)
- Rounded rectangles
- Gradients
- Shadows
- Clipping (via scissor)
- Image widgets
- Lines

## Statistics

The renderer provides detailed frame statistics:

```cpp
const UIFrameStats& stats = renderer.GetFrameStats();
// stats.drawCalls
// stats.vertices
// stats.indices
// stats.batches
// stats.cacheHits
// stats.cacheMisses
// stats.dirtyRegions
// stats.cpuTimeMs
```

## Future Enhancements

Potential improvements:

1. **Proper Memory Type Selection** - Currently uses placeholder memory type index
2. **Shader Library Integration** - Currently has placeholder shader loading
3. **Guillotine Texture Packing** - Currently uses simple first-fit
4. **Secondary Command Buffers** - For better state isolation
5. **Compute Shader Culling** - For visibility testing
6. **Multi-threaded Geometry Generation** - For better CPU utilization

## Migration Guide

To migrate from the old UIRenderer to UIRenderer2:

1. Replace `UIRenderer` with `UIRenderer2` in Editor.cpp
2. Initialize with Vulkan device parameters instead of Renderer
3. Use `UIWidgetAdapter` to convert widget paint commands
4. Update the UI overlay callback to use UIRenderer2 API
5. Test all UI elements

## Files Created

**Headers:**
- `UIRenderer2.hpp` - Main renderer
- `UIGeometryCache.hpp` - Geometry caching
- `UIBatchGenerator.hpp` - Batch generation
- `UITextureAtlasManager.hpp` - Texture atlas
- `UICommandBuffer.hpp` - Command buffer
- `UICompositor.hpp` - Final compositing
- `UIStateManager.hpp` - State management
- `UIWidgetAdapter.hpp` - Widget integration

**Implementation:**
- `UIRenderer2.cpp`
- `UIGeometryCache.cpp`
- `UIBatchGenerator.cpp`
- `UITextureAtlasManager.cpp`
- `UICommandBuffer.cpp`
- `UICompositor.cpp`
- `UIStateManager.cpp`
- `UIWidgetAdapter.cpp`

**Shaders:**
- `UI2.hlsl` - New UI shaders

## Conclusion

This new architecture provides a clean, modular, and performant UI rendering backend inspired by UE5's Slate. It maintains complete independence from the scene renderer while preserving compatibility with the existing widget system.
