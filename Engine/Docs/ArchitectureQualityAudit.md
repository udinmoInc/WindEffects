# Architecture Quality Audit — Second Pass

Date: 2026-07-18  
Scope: Entire `Engine/Source` (Runtime, Editor, Programs, IgniteBT tooling) + ThirdParty wrappers.  
Companion: [ArchitectureAudit.md](ArchitectureAudit.md) (folder-layout pass).

This report focuses on **API boundaries, dependencies, ownership, threading, logging, and maintainability**. Safe fixes applied in this pass are marked **[Fixed]**. Remaining debt is prioritized P0–P3.

---

## Executive summary

| Area | Verdict |
|------|---------|
| Public → Private includes | Clean (0 violations) |
| Third-party in Public headers | Clean (0 glm/lunasvg/etc.) |
| Runtime → Editor deps | Clean (0) |
| Circular PublicDependencies | Clean (0) |
| Logging hygiene (Runtime/Editor) | Good (WE_LOG_*); CLI tools use cout |
| Export macro coverage | Uneven — ~47 types missing `*_API` |
| Singleton / global service surface | High — ~38 Public `Get()` APIs |
| Thread safety of shared caches | **P0** gaps (AssetRegistry, KindUI registries) |
| Over-broad PublicDependencies | **[Fixed]** several demotions this pass |
| Accidental Public internals | Partially **[Fixed]** (PlaceActors managers/chrome) |

---

## 1. Public API audit

### Healthy
- **346** Public headers, **0** Public `.cpp`
- **0** Public headers include Private paths
- **0** third-party includes in Public (`glm`, `lunasvg`, `nlohmann`, Vulkan, D3D, SDL, stb, FreeType)
- Math Public surface uses `Core/Math/Types.h` (`we::math::*`)

### Accidental / oversized Public surface

**Same-module-only internals still in Public (demote candidates):**

| Module | Headers |
|--------|---------|
| KindUI | `AtlasCache`, `AtlasManager`, `IconManager` (IconManager is transitively needed by Public IconRenderer — needs pimpl before demote) |
| ECS | `ArchetypeManager`, `EntityManager` (pulled by `World.h` — needs façade) |
| Terrain | `TerrainChunkManager`, `TerrainLODManager` |
| Text | `FontAssetManager`, `FontAtlasManager` |
| World | `EnvironmentManager` (included by exported `EnvironmentSystem`) |
| Core | `ModuleManager` (process-wide; keep but document) |
| UIFramework | `DockManager` / `IDockManager` (Editor-only consumers — consider Private + façade) |

**[Fixed]** Demoted to Private (PlaceActors only):
- `ActorsPanelChrome.h`
- `PlaceActorsFavoritesManager.h`
- `PlaceActorsRecentlyUsedManager.h`

**Cross-module Public that must stay** (or get slim façades): `PanelChrome`, `ControlChrome`, `ToolbarButtonChrome`, `OverlayManager`, `ThemeManager`, `PluginManager`.

### Export macros

Engine uses module-local macros (`CORE_API`, `RHI_API`, `KINDUI_API`, …), not a single `WE_API` family. That is fine if consistent per module.

| Finding | Detail |
|---------|--------|
| **[Fixed]** | `UIFRAMEWORK_API` now has non-Windows visibility fallback |
| Gap | ~47 Public classes lack class-level `*_API` (World Environment\* siblings, PlaceActors Catalog/Panel/…, ContentBrowser widgets, Viewport helpers, WorldOutliner toolbars) |
| Style mix | Some modules export free functions only (`ENVIRONMENT_API`, `WORLDOUTLINER_API`); others export classes |
| Programs | Stub `Export.h` with no macros (OK for executables) |

**Recommendation:** Standardize on class-level `MODULE_API` for every type with out-of-line methods in Public. Prefer one pattern: `class MODULE_API Foo`.

### Forward declarations / STL in Public

- **209 / 346** Public headers include `<string>`, `<vector>`, `<memory>`, and/or `<unordered_map>`
- ~36 orphan STL includes (easy cleanup)
- True IWYU wins are mostly custom types behind `unique_ptr`, not dropping `<string>` from by-value APIs

---

## 2. Private implementation audit

| Status | Item |
|--------|------|
| **[Fixed]** | ECS `World` pimpl: `Impl*` → `std::unique_ptr<Impl>` (ownership clearer; dtor still in `.cpp` for DLL-safe teardown) |
| Open | PlatformFactory still returns raw `new` backends |
| Open | ECS chunk allocator uses intentional raw `new`/`delete` (document as arena ownership) |
| Open | KindUI/Text/Terrain “Manager” types remain Public (see §1) |
| Open | ~38 Public singletons — prefer injectable services for Editor features |

---

## 3. Module boundary audit

### Dependency graph (target layers)

```mermaid
flowchart TB
  subgraph runtime [Runtime]
    Core --> Platform
    Platform --> Engine
    Platform --> RHI
    Engine --> ECS
    RHI --> Renderer
    ECS --> Renderer
    Renderer --> Scene
    Scene --> World
    Renderer --> Text
    Text --> KindUI
    RHI --> KindUI
  end
  subgraph editor [Editor]
    KindUI --> UIFramework
    UIFramework --> panels [EditorPanels]
  end
  subgraph programs [Programs]
    panels --> EditorExe[Editor]
    runtime --> EditorExe
    runtime --> WeLauncher
    RHI --> backends[VulkanRHI_DX12_Null]
    EditorExe -.Private.-> backends
    WeLauncher -.Private.-> backends
  end
```

### Hard rules — verified clean
- No Runtime → Editor
- No circular PublicDependencies
- RHI backends depend only on Core/Platform/RHI
- Renderer does **not** depend on KindUI (KindUI → Renderer is correct)

### **[Fixed]** Public → Private dependency demotions

| Module | Change |
|--------|--------|
| Scene | Renderer → Private |
| World | Renderer → Private |
| Terrain | Renderer → Private |
| UIFramework | Engine, Scene, World → Private |
| PropertyEditor | ContentBrowser → Private |
| PlaceActors | TerrainEditor → Private |
| ToolsPanel | Menus, ContentBrowser → Private |
| TerrainEditor | Removed unused ToolsPanel |
| WeLauncher | Engine, RHI, Renderer, ECS → Private |
| CrashReporter | Engine, Renderer, RHI, KindUI, UIFramework → Private |

### Remaining smells
| Issue | Notes |
|-------|-------|
| CrashReporter → UIFramework | Program still links Editor UI module (Private). Long-term: extract shared dialog chrome to Runtime/KindUI |
| WeLauncher lacks DirectX12RHI Private | Asymmetry vs Editor — intentional if launcher is Vulkan-only; document |
| Editor.exe PublicDependencies | Host app lists all panels as Public — consider Private composition-only |

---

## 4. Include hygiene

| Status | Finding |
|--------|---------|
| Clean | No Public→Private includes |
| Open | ~36 Public headers with unused STL includes |
| Open | Full IWYU pass not automated — do per-module when touching files |
| Open | Prefer forward decls in Public for opaque handles (`class Foo;` + `unique_ptr<Foo>`) |

**Include order standard (enforce on touch):**
1. Own header  
2. Module Public  
3. Engine Public  
4. Third-party  
5. STL / C  

---

## 5. API consistency (`*_API`)

Desired family mapping (documentation, not rename):

| Layer | Macro examples |
|-------|----------------|
| Platform | `PLATFORM_API` |
| Core | `CORE_API` |
| RHI | `RHI_API`, `DX12RHI_API`, `VULKANRHI_API` |
| Runtime | `ENGINE_API`, `RENDERER_API`, `ECS_API`, `KINDUI_API`, … |
| Editor | `UIFRAMEWORK_API`, `VIEWPORT_API`, … |

**Action:** Add missing class exports in World Environment siblings and PlaceActors Public types (P1).

---

## 6. Memory ownership

| Status | Finding |
|--------|---------|
| **[Fixed]** | `World::m_Impl` → `unique_ptr` |
| P1 | `PlatformFactory` → return `unique_ptr<IPlatform>` |
| P1 | ContentBrowserToolbar `shared_ptr(new T)` → `make_shared` |
| P2 | Document ECS chunk `new`/`delete` as owning allocator |
| P2 | Document non-owning raw pointers in RHI handle maps |

---

## 7. Const / constexpr

Not exhaustively rewritten. Priority targets for next pass:
- Query APIs returning views (`span` / const ref)
- Math helpers in `Core/Math/Types.h` (already mostly `constexpr`)
- Public getters that return copies of large `vector`/`string`

---

## 8. Global state / singletons

**~38 Public `Get()` / `Instance()` APIs.** Worst clusters:

| Cluster | Count | Examples |
|---------|------:|----------|
| PlaceActors | 7 | Config, Catalog, Placement, Favorites, RecentlyUsed, Icon/Thumbnail providers |
| UIFramework | 7 | ModeController, ToolsRegistry, Preferences, Workspace, NavSettings, PerfStats, ExtensionBootstrap |
| Core | 6 | AssetRegistry, ModuleManager, PluginManager, Localization, StartupValidator, ModuleInitializerRegistry |
| KindUI | 4 | ThemeManager, IconRegistry, StyleClassRegistry, EngineIconArt |

**Process globals (acceptable if documented main-thread):** `Platform::Get()`, `RHI::Get()`, `Renderer::Get()`.

**Recommendation:** Keep Core/RHI/Platform process services; inject Editor feature services via `EditorApplicationContext` / workspace instead of proliferating `Get()`.

---

## 9. Thread safety — P0

| Component | Issue | Action |
|-----------|-------|--------|
| `AssetRegistry` | Unsynchronized map + singleton | Mutex **or** assert main-thread-only |
| `EngineIconArt::m_Cache` | GPU cache, no lock | Same |
| `IconRegistry` / `StyleClassRegistry` | Maps, no sync | Same |
| `UIRepaintGate` public statics | Non-atomic flags | Make atomic **or** document UI-thread-only |
| `DPIContext` / `TextMetrics` file statics | Unsynced | Document UI-thread-only |
| Logger | Mutex + atomics | OK |
| AtlasCache / ThemeManager / ThumbnailManager | Mutex present | OK |

---

## 10. Error handling

| Status | Finding |
|--------|---------|
| **[Fixed]** | WeLauncher `LauncherSettingsStore::Save` and `JsonFile` Load/Save now log on exception |
| Open | Silent `catch` in DockManager layout load, SearchController bad regex, CrashReporter JSON, Platform/RHI `IsAvailable` |
| Open | Many parse helpers return fallback without log (config INI/JSON) — add `WE_LOG_WARN` at parse failure |
| Open | Prefer `RHIResult` / `Expected`-style for new Public APIs; migrate bool returns gradually |

---

## 11. Logging

| Area | Status |
|------|--------|
| Runtime / Editor | Predominantly `WE_LOG_*` |
| Logger internals | May use cout as sink — OK |
| CLI Programs (FontImport, IconImport, ECSTests, TextRenderTest) | `std::cout` — acceptable for tools |
| `OutputDebugStringW` | Windows platform services — OK if wrapped |
| printf in engine | Essentially none (IgniteBT `we_probe.c` only) |

---

## 12. Naming consistency

PascalCase dominates. Exceptions (documented):
- `Programs/Windows/Resources/resource.h`
- IgniteBT `we_probe.c`

---

## 13. Performance opportunities (sampling)

| Opportunity | Where |
|-------------|--------|
| Temporary `std::string` concatenations in logs | Prefer format buffers / string_view APIs |
| Pass-by-value large structs in UI layout | Prefer const ref / move |
| Virtual dispatch in hot KindUI paint | Profile before changing |
| Redundant map lookups in ContentBrowser / PlaceActors filters | Cache filtered views |
| STL in Public headers | Increases rebuild fan-out — shrink Public includes |

---

## 14. Documentation gaps

Public subsystems needing overview docs (responsibility, deps, thread model, ownership):

1. RHI + backends  
2. Renderer  
3. ECS World  
4. KindUI application host  
5. UIFramework docking/workspace  
6. AssetRegistry  
7. Module/Plugin managers  
8. WeLauncher services  

Suggested location: `Engine/Docs/Subsystems/<Name>.md`.

---

## 15. Dead code / duplicates

| Item | Notes |
|------|-------|
| FirstRunAgreementPaint vs Markdown | Some duplicate scroll helpers observed in earlier split — consolidate if both paths are live |
| TerrainEditor → ToolsPanel | **[Fixed]** removed unused dep |
| Win32WindowChrome/Icon | Already deleted in layout pass |

---

## 16. Fixes applied this pass

1. Demoted Scene/World/Terrain Renderer PublicDependencies → Private  
2. Demoted UIFramework Engine/Scene/World → Private  
3. Demoted PropertyEditor ContentBrowser → Private  
4. Demoted PlaceActors TerrainEditor → Private  
5. Demoted ToolsPanel Menus/ContentBrowser → Private  
6. Removed TerrainEditor → ToolsPanel  
7. Tightened WeLauncher / CrashReporter PublicDependencies  
8. Demoted PlaceActors Favorites/RecentlyUsed/ActorsPanelChrome headers → Private  
9. `World` pimpl → `std::unique_ptr`  
10. Fixed `UIFRAMEWORK_API` non-Windows export  
11. Logged WeLauncher settings/JSON failures  

---

## 17. Recommended next refactor waves

### Wave A — P0 thread model (1–2 days)
Document or synchronize AssetRegistry + KindUI registries + UIRepaintGate.

### Wave B — Export completeness (2–3 days)
Add missing `*_API` on World Environment types, PlaceActors Public classes, ContentBrowser/Viewport/WorldOutliner widgets.

### Wave C — Singleton reduction (Editor) (1 week)
Route PlaceActors/UIFramework services through application context; keep Core/RHI process singletons.

### Wave D — IWYU / forward-decl (ongoing)
Per-module: drop orphan includes; forward-declare in Public; measure incremental build times.

### Wave E — Subsystem docs (ongoing)
One markdown per Public subsystem listed in §14.

---

## Success criteria (quality bar)

| Criterion | Status |
|-----------|--------|
| No Public→Private includes | Met |
| No Runtime→Editor | Met |
| No third-party in Public | Met |
| Minimal PublicDependencies | Improved; continue Wave C/D |
| Explicit thread model | Not met — Wave A |
| Complete export macros | Not met — Wave B |
| Structured errors everywhere | Partial — RHI good; config/IO improving |
| Subsystem documentation | Not met — Wave E |

The codebase now has a solid **physical** module layout (pass 1) and a clearer **logical** dependency graph (pass 2). Remaining work is engineering-quality depth: thread contracts, export completeness, DI for Editor services, and documentation.
