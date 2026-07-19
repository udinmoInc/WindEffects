# Viewport Editing Framework Architecture

> ViewportEdit is the editor interaction layer for 3D viewport picking, selection, transform tools, snap/grid, and camera framing. It does **not** own gameplay data, renderer pixels, or the undo stack.

---

## System Overview

```
ToolsPanel / ViewportWidget / Editor
              │
              ▼
IViewportEditor  →  IViewportContext
       │                 ├─ IViewportSelection
       │                 ├─ IViewportHitTester
       │                 ├─ IViewportCameraController  → EditorCamera
       │                 ├─ IViewportManipulator
       │                 ├─ IViewportSnapProvider / IViewportGridProvider
       │                 └─ Undo / Scene / PropertyEditor (DI pointers)
       ▼
IViewportInteraction → active IViewportTool / IViewportMode
```

| Layer | Owns | Does not own |
|-------|------|--------------|
| **ViewportEdit** | Tools, modes, selection set, hit-test policy, snap, gizmo drag state, overlays hooks | Entity/actor data, GPU resources, undo history |
| **Scene / World** | Objects & transforms | Input routing |
| **Undo** | Transaction history via `RecordCustom` / property hooks | Viewport UI |
| **Viewport (widget)** | Display surface + camera navigation | Editing semantics |
| **PropertyEditor / Outliner** | Details / hierarchy UI | Picking |

---

## Ownership & DI

1. Editor constructs `CreateViewportEditRuntime(ViewportEditDependencies)`.
2. `ViewportEditSession::Install(editor)` binds ToolsPanel / ViewportWidget consumers.
3. Selection sync: ViewportEdit → `Scene::SetSelectedEntityId` → Environment `TickEditor` refreshes Details.
4. Outliner → Scene selection → `SyncSelectionFromScene()` on tick (host).
5. Transforms record Undo with before/after transform snapshots; Reflection/Serialization remain owners of typed property diffs.

---

## Tools & Modes

| Tool | Shortcut (ToolsPanel) | Behavior |
|------|-----------------------|----------|
| Select | Q | Click pick, Ctrl/Shift toggle, Alt marquee |
| Move | W | Drag / `ApplyTranslation` → Undo |
| Rotate | E | Drag / `ApplyRotationDegrees` → Undo |
| Scale | R | Drag / `ApplyScale` → Undo |

Modes (`Default`, Landscape, Foliage, …) gate specialized tools; Default ships with the runtime.

---

## Hit Testing

Near-term: ray vs entity sphere proxies (radius by type/scale) using `EditorCamera` view/projection. Marquee uses screen-space projection of entity pivots. Future: mesh/BVH / World actor handles without changing public interfaces.

---

## Extension Points

| Extension | Mechanism |
|-----------|-----------|
| Tools | `IViewportEditor::RegisterTool` |
| Modes | `RegisterMode` |
| Overlays | `RegisterOverlay` |
| Drag-drop | `IViewportDragDrop` |
| Renderer hooks | `IViewportRenderer` (outlines/gizmo draw) |

---

## Constraints

- ViewportEdit never stores authoritative transforms — Scene/World do.
- Every committed transform goes through Undo (`ITransactionManager::RecordCustom` or property path).
- Public headers stay interface-first; glm only in Private TUs.
