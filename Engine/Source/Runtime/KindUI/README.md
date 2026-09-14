# KindUI

KindUI is the retained-mode UI framework used by WindEffects editor chrome and tool applications such as WeLauncher. It owns the widget tree, layout, interaction routing, paint recording, theme resolution, and the GPU overlay path that draws UI into an RHI frame.

The framework center is the widget system (`Widget`, layout containers, `PaintContext`, `OverlayRenderer`). Composition APIs under `Compose/` are optional layers on top of that system — they are not a second framework.

Namespace: `we::runtime::kindui` (editor panel builders live in `we::editor::dsl`).

Module: `Engine/Source/Runtime/KindUI` (`KindUI.Build.cs`). Public headers under `Public/KindUI/`, implementation under `Private/`.

---

## What KindUI provides

**Widget tree.** Widgets are `std::shared_ptr<Widget>` nodes. Each implements `Measure`, `Arrange`, and `Paint`. Parents own children; geometry is stored on the widget (`GetGeometry()`). Interaction state (hover, press, focus, selection, enabled, collapsed, …) lives on the widget and drives invalidation.

**Layout.** Containers such as `Flex` / `Row` / `Column`, `Grid`, `Splitter`, `ScrollViewport`, `ScrollLayout`, and `CollapsibleGroup` turn constraints into final rectangles. Widgets report desired size; layout assigns allotted rects. Flex follows a CSS-flexbox-inspired model (direction, wrap, justify, align, grow/shrink/basis, gap).

**Input.** `EventSystem` hit-tests the tree (`Widget::HitTestPoint`), routes mouse/key/text events, and tracks hover, capture, and focus. Popups can be consulted through an overlay host when set.

**Paint.** Widgets record immediate-mode draw commands into a `PaintContext` (rects, surfaces, text, icons, lines, textures, gradients, shadows). Painting does not talk to the GPU directly.

**Render.** `OverlayRenderer` (with internal `UIWidgetAdapter`) converts draw commands to batched geometry and submits through RHI. Text uses the Text module / MSDF atlas path; icons use `IconRenderer` / LunaSVG. Hosts call `RenderUI` each frame with a root widget.

**Theme.** Semantic design tokens (`ColorToken`, `MetricToken`, `SpacingToken`, …) resolve through `IKindUITheme` / `ThemeManager`. Helpers in `ThemeAccess.h` and the `ds::` accessors in `DesignSystem.h` are the normal call sites for chrome metrics and colors. `GraphiteDarkTheme` is the built-in default.

**Panels.** Editor-facing panel chrome (`Panel`, `PanelBuilder`, toolbars, mode tabs, property rows) sits in the public `UI/` surface and is what the editor DSL builds onto.

**Hosting.** Applications wire a `WidgetContext` (theme, style resolver, command registry, event bus, resources, popup host), optionally use `ViewHost` / `ApplicationServices` for retained declarative views and dialogs, and drive `OverlayRenderer` for presentation.

**Diagnostics.** Optional public headers under `Diagnostics/` for invalidation tracing, phase timing, color/input audits, screen recording, and benchmarks. These are not part of the normal editor include path.

---

## Public vs private

Public organization describes **what consumers use**. Private organization describes **how KindUI is implemented**. The trees are intentionally different.

```text
Public/KindUI/                 Private/
  EditorUI.h                     Core/
  Export.h                       Widgets/
  Core/                          Layout/
  UI/                            Panel/
  Theme/                         Rendering/
  Host/                          Theming/
  Compose/                       App/
  Diagnostics/                   Docking/
                                 Profiling/
                                 Compose/
```

| Public area | Responsibility |
|---|---|
| `Core/` | Kernel: `Widget`, types, paint context, input events, event system, services, commands, resources, expansion, repaint gate |
| `UI/` | What you build with: controls, layout containers, panels, overlays |
| `Theme/` | Tokens, theme interfaces, palettes, typography, design-system accessors |
| `Host/` | Embedding: `ViewHost`, docking frames, `OverlayRenderer`, icon/font host services |
| `Compose/` | Optional composition: Launcher `UI` / `Element` / `ViewBuilder`, and Editor `EditorDSL` |
| `Diagnostics/` | Profiling and debug tooling |

Do not include Private headers from outside the KindUI module. Implementation details such as `UIWidgetAdapter`, paint retention stores, GPU upload helpers, and style-class resolution live in Private.

---

## Entry points

**Editor UI** — one include for normal editor panel work:

```cpp
#include <KindUI/EditorUI.h>
using we::editor::dsl::Panel;

auto inspector = Panel("Inspector", [](PanelContext& p) {
    p.Section("Transform", [](auto& s) {
        s.Property("Location", locationWidget);
    });
});
```

`EditorUI.h` pulls in core types, theme accessors, layout/panel/controls, input, and `Compose/EditorDSL.h`. It does not include Diagnostics or `OverlayRenderer`; those stay explicit specialty includes.

**Launcher / declarative apps** typically include what they need:

```cpp
#include "KindUI/Compose/UI.h"
#include "KindUI/Host/ViewHost.h"
#include "KindUI/Theme/ThemeManager.h"
// plus specific controls as required
```

**Direct widgets** remain first-class. Composition is optional. You can construct `Flex`, `Label`, `TextBox`, etc., attach children, and host the root yourself.

---

## Runtime flow

Per frame, after input has updated widget state / invalidation:

```text
Input / data change
        │
        ▼
InvalidateLayout / InvalidatePaint  ──►  UIRepaintGate (layout vs paint)
        │
        ▼
OverlayRenderer::RenderUI
        │
        ▼
UIWidgetAdapter::ProcessWidget
        │
        ├─ (if layout needed) Measure → Arrange → ClearSubtreeLayoutDirty
        │
        ▼
PaintSubtree (dirty subtrees Paint; clean retained slices may replay)
        │
        ▼
DrawCommand list
        │
        ▼
Convert → vertices / indices / batches
        │
        ▼
RHI submit (OverlayRenderer)
```

Hosts own when layout runs. `OverlayRenderer` peeks dirty state; the host decides whether to pass `needsLayout` into `ProcessWidget`. Layout is skipped when the root geometry already matches the viewport and `SubtreeNeedsLayout()` is false.

Hover/press animation uses `UIRepaintGate::MarkAnimating` so paint keeps running while damping settles without necessarily forcing layout.

---

## Widgets, layout, and invalidation

KindUI keeps widget state and layout separate. Widgets own interaction state and expose desired size; layout containers decide final geometry.

### Lifecycle

```cpp
virtual Size Measure(const Size& availableSize) = 0;
virtual void Arrange(const Rect& allottedRect) = 0;
virtual void Paint(PaintContext& context) = 0;
```

`Construct()` and `Tick(deltaTime)` exist for setup and per-frame work (for example hover damping). Prefer invalidating from setters rather than rebuilding entire trees.

### Dirty flags

Each widget tracks:

- `m_NeedsLayout` / `m_SubtreeNeedsLayout`
- `m_NeedsPaint` / `m_SubtreeNeedsPaint`
- style dirty via `InvalidateStyle()` (which paints)

`InvalidateLayout()` and `InvalidatePaint()` set local flags, mark the ancestor chain’s subtree bits, invalidate retained paint upward where needed, and notify `UIRepaintGate`. Subtree queries are O(1) — no walk required to know whether work is pending.

**Hidden / collapsed default:** when a widget is not *effectively* visible (self or any ancestor hidden), invalidation records dirty bits only and does **not** arm the gate or wipe retained paint on visible ancestors. Updates inside collapsed panels therefore stay free for the visible chrome. Becoming visible via `SetVisible` or an `Expansion` batch flushes layout/paint once. Hiding via `SetVisible` / `SetVisibleSilent` releases retained-paint stores under that subtree.

After a successful layout pass, `ClearSubtreeLayoutDirty()` clears the layout dirty chain. Paint clears via `ClearSubtreePaintDirty()` inside `PaintSubtree` (adapter only scrubs leftovers).

### UIRepaintGate

Global gate used by editor chrome and structural operations:

- `RequestLayout()` / `RequestPaint()` — separate so hover-only frames avoid Measure/Arrange
- `RequestLayoutReason` / `RequestPaintReason` — tagged reasons when `WE_UI_INVALIDATION_LOG=1`
- `ScopedBatch` / `BeginBatch`/`EndBatch` — collapse multi-step structural edits (dock/float, expansion) into one layout and one paint
- `ConsumeNeedsLayout` / `ConsumeNeedsPaint` — for the UI/render thread
- `MarkAnimating` / `MarkSettled` — animation latch for Tick/damp

`Expansion` (expand/collapse all) applies node state without per-node gate spam, then commits inside a batch so the tree rebuilds once. `Panel` and `CollapsibleGroup` both implement `IExpansionNode`, so Expand/Collapse All under a shell root covers panels and groups together.

### UIStateChangeGate

Canonical router for interaction/presentation state (hover, pressed, focus, selection, property bindings, enabled/style, expansion, visibility). Widget setters and `Observable::Bind` call `UIStateChangeGate::Notify(widget, kind)` instead of ad-hoc `InvalidatePaint`/`InvalidateLayout`:

- **Paint-only** — hover, pressed, focus, selection, property, value, style, enabled, animation (geometry unchanged)
- **Layout + paint** — expansion, visibility (geometry may change)
- **`ScopedTransaction`** — coalesce multiple notifications into one gate arm (used by `EventSystem` hover/focus, `Expansion`, multi-select)
- **Dedup** — skips widgets already marked `NeedsPaint` / `NeedsLayout`
- Integrates with `UIRepaintGate`, `UIDirtyRegionTracker`, retained paint, and input-latency audit

Future panels inherit this automatically through normal `Widget` APIs — no panel-specific state systems.

**Env:** `WE_UI_STATE_BENCH=1` runs `RunKindUIStateChangeBenchmark` (hover / focus / selection / unselection / multi-select / property / expansion with notification and gate-arm counts).

### Paint retention

On paint-only rebuilds, `PaintSubtree` may replay a retained command slice for a clean subtree instead of calling `Paint()` and walking children. Dirty subtrees repaint and refresh their slices. Retention is disabled on layout frames because geometry changes invalidate stored slices.

**Host-owned layout:** when the editor/launcher `Measure`/`Arrange` outside `UIWidgetAdapter`, it must call `UIRepaintGate::NotifyHostLayoutCompleted()` (and typically `ReleaseRetainedPaintSubtree()` on the root). Otherwise the next paint-only rebuild can replay absolute status-bar/draw commands at stale positions over panels.

---

## Painting and rendering

Widgets paint through `PaintContext`:

```cpp
void MyWidget::Paint(PaintContext& ctx) {
    ctx.DrawSurface(GetGeometry(), SurfaceRole::Panel, radius);
    ctx.DrawText(m_Label, textPos, ResolveColor(ColorToken::TextPrimary));
    ctx.DrawWindIcon(m_Icon, iconRect);
}
```

Prefer `DrawSurface` / surface roles for backgrounds so semantics stay attached for theming and diagnostics. Clip stacks (`PushClipRect`) and surface-owner scopes are available for nested chrome.

`UIWidgetAdapter` turns commands into geometry (rects, text, icons, textures, lines, shadows, gradients, outlines), then batches by texture, scissor, text vs non-text, and opaque-replace vs alpha. `OverlayRenderer` owns pipelines, buffers, icon/text services, optional submit caching when geometry is unchanged, and frame stats (`UIFrameStats`, `UiBuildPhaseTiming`).

### Canonical GPU geometry path

All KindUI screens inherit one shared upload/submit path. Do **not** add per-widget or per-panel GPU buffers.

```text
UIRepaintGate → OverlayRenderer::RenderUI
  → UIWidgetAdapter (paint + DrawCommandBatcher)
  → CPU verts/indices (capacity retained)
  → EndOverlayPass → BuildDrawList (color convert into cached UIDrawList)
  → UiImmediateRenderer::SubmitDrawList
       ├─ skip upload if geometryGeneration matches (idle / retained)
       ├─ else skip upload if content hash matches (same mesh, new generation)
       ├─ else EnsureBuffer (grow-only, FIF-safe retire of old handles)
       └─ HostVisible UpdateBuffer for this frame slot
  → secondary submission cache hit (default on) or RecordDrawList
```

| Concern | Owner |
|---|---|
| Invalidation / idle skip | `UIRepaintGate` + retained `PaintSubtree` |
| Command coalesce | `DrawCommandBatcher` (span-based index merge) |
| CPU mesh | `UIWidgetAdapter` / `OverlayRenderer` vectors |
| GPU VB/IB, upload, rotation | **`UiImmediateRenderer` only** |
| Atlas/icon texel refresh | `UpdateRgbaTexturePixels` (in-place; no recreate) |
| Metrics | `UiGpuPathStats` via `OverlayRenderer::GetGpuPathStats()` |

**Env:** `WE_UI_SUBMISSION_CACHE=0` disables secondary CB reuse (default on). `WE_UI_BUILD_PROFILE=1` logs build phases plus upload bytes / buffer creates / skips. `WE_UI_DISABLE_GLOBAL_BATCH` disables the shared batcher.

Future panels only need paint commands; they automatically get buffer reuse, content-hash skip, FIF rotation, and submission caching.

---

## Theming

UI logic should depend on tokens and roles, not raw literals scattered through widgets.

- Resolve colors/metrics via `ResolveColor`, `ResolveMetric`, `ResolvePadding`, `ResolveTypography`, or `Widget` helpers such as `ThemeColor` / `ThemeMetric` / `ResolveStyle(StyleRole)`.
- `StyleRole` maps semantic control categories to `ResolvedStyle` (background, foreground, border, radius, type, padding, …).
- `Theme/DesignSystem.h` (`we::runtime::kindui::ds`) centralizes spacing, sizing, typography sizes, chrome gaps, panel/toolbar metrics.
- `ChromeSeparation` encodes the editor gap-cut policy between adjacent surfaces.
- `TypographySystem` is the single source for role → size/weight/line-height/color-token; themes call into it for font metrics.

Initialize once per app:

```cpp
ThemeManager::Get().Initialize(std::make_shared<GraphiteDarkTheme>(), dpiScale);
```

DPI scaling is handled through `DPIContext` and theme resolution paths — paint and measure should consume already-resolved logical/layout pixels consistently with the host.

---

## Composition

`Compose/` holds two consumer models that both target the KindUI widget system. They are not interchangeable APIs and are not the architectural center of the module.

### Launcher declarative UI (`we::runtime::kindui::UI`)

`Compose/UI.h` builds `Element` trees (Row/Column/Flex, controls, `ForEach`, `Id`, modifiers). `ViewBuilder` turns elements into widgets and can `Reconcile` an existing tree by type/id. `ViewHost` retains a root, supports factories, and can `Observe` `Observable` / `ObservableList` values to refresh.

Used heavily by WeLauncher. Good fit for application pages described as data + structure.

### Editor DSL (`we::editor::dsl`)

`Compose/EditorDSL.h` builds editor panels via `Panel` / `PanelContext` / `SectionContext` / `ToolbarContext` / `TabContext`: sections, property fields (bool/float/string/color/asset/…), search, trees, toolbars, tabs. It composes KindUI panels and design-system controls; include it through `EditorUI.h` for editor work.

Do not invent a third parallel “builder” layer. Extend these models or use widgets directly.

---

## Hosting and integration

Minimum host responsibilities:

1. Create/theme/`WidgetContext` (or `ApplicationContext`) with the services widgets expect.
2. Own a root `Widget` (manual tree, `ViewHost`, or editor shell layout).
3. Feed input through `EventSystem` (or equivalent host routing that hits the same widget APIs).
4. Each frame: decide layout need via `UIRepaintGate`, then `OverlayRenderer::RenderUI(root, …)` inside an overlay pass.

```cpp
OverlayRenderer renderer;
renderer.Init(device, swapchainFormat, maxFramesInFlight);

// frame:
renderer.BeginOverlayPass(ctx);
renderer.SetTargetExtent(width, height);
renderer.RenderUI(root, frameSlot);
renderer.EndOverlayPass(ctx);
```

Docking (`DockContainer`, `FloatingPanelFrame`) and dialogs (`DialogService` via `ApplicationServices`) are host-facing. Popup presentation goes through `IPopupHost` / overlay manager paths.

---

## Input and interaction

Hit-testing walks from the root to the deepest interactive descendant, respecting clip rects, z-order, `IsPointerTransparent()`, and `IsInteractiveContainer()`.

Widgets override `OnMouseDown` / `Move` / `Up`, `OnKeyDown` / `Up`, `OnTextInput`, `OnMouseWheel` (with `CanReceiveMouseWheelAt`), `OnFocus` / `OnBlur`, and `OnCaptureLost`. `EventSystem` maintains hover chain, capture, focus, and can clear hover when the pointer leaves the window.

Focusable widgets participate in tab order (`FocusNext`). Layout containers are typically not focusable by default.

---

## Where new code belongs

| Change | Public | Private |
|---|---|---|
| Reusable control | `Public/KindUI/UI/` | `Private/Widgets/` |
| Layout container | `Public/KindUI/UI/` | `Private/Layout/` |
| Panel chrome | `Public/KindUI/UI/` | `Private/Panel/` |
| Kernel / services | `Public/KindUI/Core/` | `Private/Core/` |
| Theme / tokens | `Public/KindUI/Theme/` | `Private/Theming/` |
| Host / docking / overlay | `Public/KindUI/Host/` | `Private/App`, `Docking`, `Rendering` as appropriate |
| Launcher Element/UI helpers | `Public/KindUI/Compose/` | `Private/Compose/` or `Private/Declarative` paths if present |
| Editor DSL | `Compose/EditorDSL.h` | `Private/Compose/EditorDSL.cpp` (and related) |
| Profiling tools | `Public/KindUI/Diagnostics/` only if external | Prefer `Private/Profiling/` |

Guidelines:

- Prefer extending existing controls and tokens over copying chrome metrics into editor modules.
- Editor panels should consume KindUI widgets / DSL — do not reimplement Flex/Measure/Paint in editor code.
- Do not add public folders that mirror Private (`Rendering/`, `Widgets/`, …) for implementation structure alone.
- Do not put diagnostics into `EditorUI.h`.
- Do not make Compose the place for core widget APIs.

---

## Performance principles

These are enforced by the current implementation; treat them as constraints, not suggestions.

1. **Invalidate narrowly.** Prefer `InvalidatePaint()` for visual-only changes; use `InvalidateLayout()` when size/structure changes. Avoid reconstructing whole trees for property edits.
2. **Trust subtree dirty bits.** Gate work with `SubtreeNeedsLayout` / `SubtreeNeedsPaint` instead of full walks.
3. **Batch structural edits.** Use `UIRepaintGate::ScopedBatch` or `Expansion::ScopedTransaction` so dock/expand operations produce one rebuild.
4. **Skip layout when idle.** Paint-only frames should not Measure/Arrange when geometry is still valid.
5. **Use paint retention** on clean paint-only frames; expect it off when layout runs.
6. **Virtualize large lists/trees** via the shared path — use `ListView` (`VirtualList`) or `CompactTreeWidget`, or call `ComputeFixedVisibleRange` / `WidgetRecyclePool` from `ListVirtualization.h`. Content Browser / Outliner `TreeView` uses the same range math. No per-panel virtualization hacks; small lists use the same path (window covers all items).
7. **Keep draws atlas-friendly.** Text and icons batch best when they share atlases/descriptor sets; avoid per-frame unique textures without need.
8. **Collapse expansion changes.** Multi-section expand/collapse must go through `Expansion` so details views do not full-rebuild unnecessarily.
9. **Do not invalidate the visible shell from hidden work.** Updates under collapsed/hidden widgets are deferred automatically; do not add panel-local dirty helpers to “fix” this.
10. **Prefer framework defaults.** New panels should use `Panel` / `CollapsibleGroup` / `Flex` / `ListView` and normal `Invalidate*` APIs — performance behavior is inherited, not tuned per panel.

### Canonical list/tree virtualization

| Piece | Role |
|---|---|
| `ListVirtualization.h` | Visible-range math (fixed + variable height), height prefix sums, `WidgetRecyclePool` |
| `VirtualList` / `ListView` | Canonical list: recycle row widgets, selection on the list, Tick/Measure/Arrange/Paint only the active window + overscan |
| `CompactTreeWidget` | Shared KindUI tree: flat model + arrange/paint/hit-test of active range only |
| Editor `TreeView` (ContentBrowser) | Production outliner/browser tree: flatten model once; arrange/paint via shared `ComputeFixedVisibleRange` |

**Env:** `WE_UI_VIRT_BENCH=1` runs `RunKindUIVirtualizationBenchmark` at editor startup (scales 100 / 1k / 10k / 100k — active widgets, memory estimate, layout/paint/scroll CPU, draw commands).

### Canonical dirty-region rendering

| Piece | Role |
|---|---|
| `UIDirtyRegionTracker` | Process-wide dirty rect accumulate/merge from `InvalidatePaint` / `InvalidateLayout` / `CommitGeometry` |
| `UIWidgetAdapter` | Consumes merged regions each rebuild; hashes draw commands; **reuses prior drawgen geometry** when the command stream is unchanged (skips drawgen + GPU generation bump) |
| `UiBuildPhaseTiming` | `dirtyRegionCount`, `dirtyCoverage`, `dirtyFull`, `geometryReused` (logged via `WE_UI_BUILD_PROFILE=1`) |
| Retained `PaintSubtree` | Clean siblings still replay retained commands; dirty widgets repaint only their visual area through normal invalidation |

State-only changes (selection/hover) dirty the widget bounds. Layout expands dirty via geometry moves. Idle frames still skip rebuild via `UIRepaintGate`. Full UI submit remains required for swapchain compositing over 3D; GPU upload/submission-cache skip when geometry generation is unchanged.

**Env:** `WE_UI_DIRTY_BENCH=1` runs `RunKindUIDirtyRegionBenchmark` (selection / scroll / rebind / expansion scenarios).

### Canonical incremental layout

| Piece | Role |
|---|---|
| `LayoutIncremental.h` / `LayoutIncrementalStats` | Per-pass Measure/Arrange skip counters (`WE_UI_BUILD_PROFILE` → `layM` / `layA` / `layFull`) |
| `Widget::MeasureChild` / `ArrangeChild` | Canonical child walk — skips clean subtrees with matching available size / allotted rect |
| `CanSkipMeasure` / `CanSkipArrange` + measure cache | Measure skips on clean subtrees; **Arrange skip is leaf-only** (containers always run Arrange so custom child positioning — StatusBar, PanelBodyLayout, Flex — stays correct) |
| Smart `InvalidateLayout` propagate | `SubtreeNeedsLayout` always bubbles; ancestor `NeedsLayout` stops when grow/basis children cannot change parent intrinsic size |
| Hosts (`Editor` SyncViewport / WeLauncher SyncLayout / `UIWidgetAdapter`) | Reset stats, one root Measure/Arrange when the gate fires; tree walk is incremental |

Scroll/splitter containers do **not** skip Arrange on an identical outer rect (scroll offset / split ratio can change without resize). Future panels using `Panel` / `Flex` / `DockContainer` / `ListView` / normal `InvalidateLayout` inherit this path automatically — no per-panel layout hacks.

**Env:** `WE_UI_LAYOUT_BENCH=1` runs `RunKindUILayoutBenchmark` (property / selection / expand / scroll / resize / large-tree leaf vs full).

Phase timings (`UiBuildPhaseTiming`: clear/layout/paint/drawgen/text, retention counts, dirty coverage) and `UIFrameStats` are available when diagnosing regressions.

### Canonical text rendering

| Piece | Role |
|---|---|
| `TextEngine` layout cache | Single O(1) layout/glyph cache (1024 entries); measure and draw share it — no glyph-vector copies on hit |
| `TextUIService` geometry cache | Local-space glyph quads reused across draws with same text/font/size/weight; atlas upload only for dirty pages |
| `DrawCommandBatcher::ClusterTextRuns` | Groups non-overlapping same-clip text by font size/weight for fewer texture switches |
| Batch coalesce | Matching text batches merge across non-overlapping opaque intervening draws |
| `PaintContext::DrawText` / `TextMetrics` | All panels use this path; `TextMetrics` provider delegates to `TextUIService` (no duplicate string measure cache) |

Future panels inherit batching/caching automatically through `DrawText` / `GetTextWidth` / `Label` — no per-panel text tuning.

**Env:** `WE_UI_TEXT_BENCH=1` runs `RunKindUITextBenchmark` (cold/warm measure, many labels, repeated text, selection/property, scroll).

### Canonical overlay / popup compositor

| Piece | Role |
|---|---|
| `OverlayHost` (`OverlayManager`) | Single floating-UI compositor for dropdowns, context menus, tooltips, combo/pickers, pinned panels, and modals |
| `OverlayEntry` / `OverlayKind` | Explicit entry: widget, kind, z-order, visibility, interactive flag, anchor, placement |
| `PopupPositioner` / `IPopupHost` / `PopupService` | Shared placement + host API — no parallel popup stack |
| `UIRepaintGate::RequestOverlayLayout` | Overlay open/move/close arms overlay-only layout (not full shell Measure/Arrange) |
| `Widget::SetOverlayContent` | Invalidation under overlay content stays on the overlay gate; base editor tree is not dirtied |
| `OverlayHost::SyncOverlaysOnly` | Hosts (Editor / WeLauncher) remeasure/reposition overlays only; base arrange can be skipped |
| Dirty regions | Open/close/move marks overlay bounds; retained paint + dirty coverage stay intact |

Opening or moving one overlay must not rebuild unrelated panels. Tooltips are non-interactive (`interactive = false`). Dialogs prefer `ShowModal` on the same compositor when an `OverlayHost` is the popup host.

**Env:** `WE_UI_OVERLAY_BENCH=1` runs `RunKindUIOverlayBenchmark` (idle sync, dropdown, context, tooltip, scroll reposition, stacked, open/close, large-base isolation, modal).

### Global UI resource residency

| Piece | Role |
|---|---|
| `UIResourceResidency` | Process-wide budgets, frame clock, pin horizon, aggregate stats |
| `IconManager` | Lazy PNG→GPU upload; bounded cursor eviction; deferred `RetireTexture` |
| `TextUIService` + `FontAtlasManager` | Geometry + glyph entry budgets; incremental glyph drop (no atlas wipe) |
| `UiImmediateRenderer` | FIF-deferred texture/buffer destroy (`FlushRetiredTextures`) |
| `OverlayRenderer::PrepareForResourceEviction` | Clears retained/draw/submission caches before GPU retire |

States are lightweight: Unloaded → Loading/Uploading → Resident → Evictable. Eviction never runs every frame (`WE_UI_RESIDENCY_TICK`); never destroys textures used this frame or still in flight. Budgets are env-configurable — not per-widget limits. Future panels inherit this through existing `ResolveIcon` / `DrawText` / GPU upload paths.

**Env:** `WE_UI_RESIDENCY_BENCH=1` runs `RunKindUIResidencyBenchmark` (idle, icon-heavy, glyph-heavy, open/close, scroll, overlay, pressure eviction, re-request after eviction).

| Variable | Default / effect |
|---|---|
| `WE_UI_ICON_BUDGET` | Icon GPU budget (`48M`, supports `K`/`M` suffix) |
| `WE_UI_TEXT_GEOM_BUDGET` | Text geometry CPU budget (`8M`) |
| `WE_UI_GLYPH_BUDGET` | Atlas glyph entry cap (`8192`) |
| `WE_UI_ICON_IDLE_FRAMES` | Idle frames before opportunistic icon eviction |
| `WE_UI_TEXT_GEOM_IDLE_FRAMES` | Idle frames before geometry eviction |
| `WE_UI_RESIDENCY_MAX_EVICT` | Max evictions per tick |
| `WE_UI_RESIDENCY_TICK` | Frames between eviction passes |
| `WE_UI_RESIDENCY_GPU_DEFER` | Extra frames before deferred GPU destroy |

---

## Diagnostics (optional)

Enable only when investigating:

| Variable | Effect |
|---|---|
| `WE_UI_INVALIDATION_LOG=1` | Log gated invalidation reasons |
| `WE_PAINT_CAUSE=1` | Capture invalidation callers (`PaintCauseLog`; also interacts with screen-debug) |
| `WE_KINDUI_DUMP_LAYOUT` | Dump layout tree during ProcessWidget |
| `WE_UI_VIRT_BENCH=1` | List/tree virtualization scale bench at startup |
| `WE_UI_DIRTY_BENCH=1` | Dirty-region + command-hash reuse bench at startup |
| `WE_UI_LAYOUT_BENCH=1` | Incremental layout Measure/Arrange skip bench at startup |
| `WE_UI_STATE_BENCH=1` | State-change notification / gate coalescing bench at startup |
| `WE_UI_TEXT_BENCH=1` | Text layout/geometry/atlas batching bench at startup |
| `WE_UI_OVERLAY_BENCH=1` | Overlay compositor isolation bench at startup |
| `WE_UI_RESIDENCY_BENCH=1` | UI resource residency / eviction bench at startup |
| `WE_UI_BUILD_PROFILE=1` | Log build phases including dirty coverage / geomReuse / layM+layA |

Public diagnostic headers live under `KindUI/Diagnostics/`. Include them explicitly from editor/tool code that needs them.

---

## Dependencies

From `KindUI.Build.cs`:

- Public engine modules: Core, Platform, RHI, Renderer, Text, Icons
- Required third-party: LunaSVG (`WE_HAS_LUNASVG=1`)
- Optional: nlohmann_json (`WE_HAS_NLOHMANN_JSON`)

---

## Intended direction

KindUI stays a **widget-centric retained UI system** with a clear Measure → Arrange → Paint → submit pipeline, token-driven visuals, and dirty-subtree performance.

Composition (Launcher `Element`/`UI`, Editor DSL) remains a thin way to describe UI on top of that system. New editor features should land as KindUI widgets/panels/tokens first, then be exposed through `EditorUI.h` / DSL when the pattern is reusable — not as parallel UI stacks inside editor modules.
