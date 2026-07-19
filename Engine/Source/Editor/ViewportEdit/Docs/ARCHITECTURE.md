# Viewport Editing Framework Architecture

> ViewportEdit is the permanent mode-driven editing workspace for the WindEffects Editor.
> It hosts Select, Landscape, Foliage, Mesh Paint, Modeling, Animation, Physics, Navigation,
> Water, and plugin-defined modes without changing core viewport code.

---

## System Overview

```
ToolsPanel / ViewportWidget / Editor
              │
              ▼
IViewportEditor ──► IViewportWorkspace
       │                 ├─ IViewportModeManager ──► IViewportModeRegistry / IViewportModeFactory
       │                 ├─ IViewportCommandRouter  (Undo-backed)
       │                 ├─ IViewportSelectionContext
       │                 ├─ IViewportOverlay[]
       │                 ├─ IViewportRenderExtension[]
       │                 └─ IViewportInteractionLayer[]
       │
       ▼
IViewportContext ── tools / modes / manipulators (never owns gameplay data)
       │
       ▼
IViewportInteraction ── active IViewportTool + prioritized layers
```

| Layer | Owns | Does not own |
|-------|------|--------------|
| **ViewportEdit** | Mode load/unload, tool routing, selection set, hit-test policy, snap, overlays, command routing | Entity/actor data, GPU resources, undo history |
| **Mode plugins** (TerrainEditor, …) | Mode-specific tools, overlays, wizards | Core viewport / mode manager |
| **Scene / World** | Actors & transforms | Input routing |
| **Undo** | Transaction history | Viewport UI |
| **Renderer** | Pixels / GPU brush hooks via extensions | Editing semantics |

---

## Interface Contract (mode framework)

| Interface | Role |
|-----------|------|
| `IViewportModeManager` | Load / unload / activate modes dynamically |
| `IViewportMode` | Enter/exit/tick + preferred tools |
| `IViewportTool` | Interaction tool (stateless services preferred) |
| `IViewportToolContext` | Narrow DI surface for tools |
| `IViewportWorkspace` | Permanent host for modes, overlays, extensions |
| `IViewportOverlay` | Screen / brush-preview overlays |
| `IViewportManipulator` | Transform gizmos |
| `IViewportInteractionLayer` | Prioritized input stack |
| `IViewportCommandRouter` | Named commands → Undo |
| `IViewportSelectionContext` | Mode-filtered selection + terrain primary |
| `IViewportRenderExtension` | GPU/CPU draw hooks (brush preview, etc.) |
| `IViewportModeFactory` | Creates mode instances |
| `IViewportModeRegistry` | Thread-safe factory registry (plugin extensible) |

Modes register factories with `GetViewportModeRegistry()`. Core never hardcodes Landscape/Foliage/Water implementations.

---

## Ownership & DI

1. Editor constructs `CreateViewportEditRuntime(ViewportEditDependencies)`.
2. `ViewportEditSession::Install(editor)` binds ToolsPanel / ViewportWidget consumers.
3. TerrainEditor (and future plugins) call `Modes().Registry().RegisterFactory(...)` then `LoadMode`.
4. Selection sync: ViewportEdit → Scene → Outliner / PropertyEditor.
5. Every committed edit goes through `IViewportCommandRouter` / `ITransactionManager`.

---

## Built-in Modes

| Mode | Module | Status |
|------|--------|--------|
| Select | ViewportEdit | Shipping |
| Landscape | TerrainEditor | Shipping (first advanced mode) |
| Foliage / MeshPaint / Modeling / Animation / Physics / Navigation / Water | Registry stubs | Ready for plugins |
| Plugin-defined | External DLLs | Register factory at startup |

---

## Extension Points

| Extension | Mechanism |
|-----------|-----------|
| Modes | `IViewportModeRegistry::RegisterFactory` |
| Tools | `IViewportEditor::RegisterTool` / workspace |
| Overlays | `IViewportWorkspace::RegisterOverlay` |
| Render | `IViewportWorkspace::RegisterRenderExtension` |
| Input | `PushInteractionLayer` |
| Commands | `IViewportCommandRouter::Register` |
| Drag-drop | `IViewportDragDrop` |

---

## Constraints

- ViewportEdit never stores authoritative transforms or heightfields — Scene/World/Terrain do.
- Landscape is a normal world actor + `TerrainAsset`, not viewport state.
- Modes load/unload without modifying `ViewportEditorImpl` interaction core.
- Public headers stay interface-first; glm only in Private TUs.
- Diagnostics, tests, and benchmarks live beside the runtime (`ViewportEditDiagnostics`, `RunViewportEditRuntimeTests`, `RunViewportEditBenchmarks`).

---

## Future Systems (no viewport redesign)

Foliage, water, roads, splines, erosion graphs, procedural generation, world partition, and large-world streaming plug in as additional modes / render extensions / interaction layers on this workspace.
