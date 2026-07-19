# Landscape Mode & Terrain Editor Architecture

> Landscape Mode is the production advanced editor mode on the Viewport Editing Framework.
> Terrain exists as a normal world actor and asset — never as special viewport state.
> **Creating a landscape never requires a heightmap file.** Flat is the default.

---

## Layers

```
Tools drawer (Landscape mode customContent)
    │  LandscapeWorkspacePanel — Create / Sculpt / Paint / Manage
    ▼
ILandscapeEditor (wizard, brush, layers, manage commands)
    ▼
ViewportEdit (Landscape viewport mode + tools)
    ▼
Terrain Runtime (ITerrainRuntime)
```

---

## Landscape Workspace Panel

KindUI workspace in the Tools drawer (Place Actors pattern via `customContent`):

| Tab | Role |
|-----|------|
| **Create** | New Landscape form + sticky Create / Create Additional CTA |
| **Sculpt** | Grouped brush tools + brush settings |
| **Paint** | Layer list + paint brush settings |
| **Manage** | Info snapshot + resize / rebuild / import-export / delete |

Panel binds only to `ILandscapeEditor*` (injected). No direct Terrain Runtime calls from UI.
`RegisterExtension(tab, factory)` reserves future slots (Water, Foliage, Splines, …).

---

## New Landscape Dialog

Wizard / dialog state drives Create tab:

- Default generator: **Flat**
- Heightmap import appears only when creation method is HeightmapImport (optional)
- Finish is valid without any heightmap path

---

## Editing Loop

1. User selects Landscape mode → workspace opens; Create if none, else Sculpt.
2. Brush BeginStroke → CaptureElevationSamples → Apply → EndStroke → Undo transaction.
3. Dirty chunks rebuild mesh/collision; streaming/LOD tick with camera.

---

## Integration Map

| System | Integration |
|--------|-------------|
| World / Scene | Landscape actor via `ITerrainComponent` |
| Undo | Automatic brush-stroke transactions |
| Reflection / Property Editor | Registered create info, brush ops, generators |
| Serialization / Asset Runtime | `ITerrainAsset` elevation samples |
| Content Browser | `.weterrain` / `.terrain` / `.landscape` |
| ViewportEdit | Landscape mode + sculpt/paint tools |
| ToolsPanel | Landscape `customContent` workspace |

---

## Host Wiring

Editor constructs `CreateTerrainRuntime`, installs via `SetDefaultTerrainRuntime`, then binds Landscape editor scene/undo/viewport and `InstallViewportMode`.

---

## Diagnostics / Tests / Benchmarks

- Runtime: `TerrainDiagnostics`, `RunTerrainRuntimeTests()`, `RunTerrainRuntimeBenchmarks()`
- Editor: `TerrainEditorDiagnostics`, `RunTerrainEditorTests()`, `RunTerrainEditorBenchmarks()`

---

## Non-goals (intentionally deferred)

- GPU sculpt compute
- World partition page streaming (chunk streaming foundation shipped)
- Virtual texture physical cache (API hooks shipped)
- OS file dialogs for heightmap paths (path fields used)
