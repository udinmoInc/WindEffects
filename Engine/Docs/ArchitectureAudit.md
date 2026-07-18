# Architecture Audit — Closed Report

Companion quality audit: [ArchitectureQualityAudit.md](ArchitectureQualityAudit.md) (API boundaries, threading, ownership, deps).

Audit completed: 2026-07-18  
Scope: All IgniteBT modules under `Engine/Source`.

Status legend: `[x]` fixed · `[D]` documented exception · `[~]` deferred (headers only)

---

## Documented exceptions

| Item | Reason |
|------|--------|
| `[D]` Programs/IgniteBT | C# build tool; own `Source/` layout |
| `[D]` Engine/Shaders | Engine-wide shader depot (not per-module) |
| `[D]` Programs/Windows | Shared non-module resource headers (`Resources/resource.h`) for `.rc` / WinMain |

---

## Canonical module layout (enforced)

```
Module/
├── Public/           # API only — prefer Public/<ModuleName>/...
├── Private/          # implementation
├── Resources/        # optional (.rc, assets)
├── Shaders/          # optional (prefer Engine/Shaders for shared)
├── ThirdParty/       # optional, module-local only
├── Tests/            # optional
└── Module.Build.cs
```

---

## Phase completion

| Phase | Description | Status |
|-------|-------------|--------|
| 0 | Audit document | `[x]` |
| 1 | Structural normalization | `[x]` |
| 2 | Public surface hygiene | `[x]` |
| 3 | Includes and dependencies | `[x]` |
| 4 | Third-party wrapping | `[x]` |
| 5 | Split oversized units | `[x]` |
| 6 | Consistency pass | `[x]` |

---

## 1. Folder structure — resolved

| Status | Module | Resolution |
|--------|--------|------------|
| `[x]` | Programs/WeLauncher | `Private/{App,Model,Services,UI,Util}`; `Public/WeLauncher/`; `Resources/` |
| `[x]` | Programs/Editor, CrashReporter, FontImport, IconImport, ECSTests, TextRenderTest | Public + Private layout |
| `[x]` | Runtime/World | Environment/DefaultScene headers → `Public/`; cpp → `Private/`; no `IncludePaths(".")` |
| `[x]` | DirectX12RHI, NullRHI, VulkanRHI | `Public/<Module>/Export.h` added |
| `[x]` | Programs/Windows | `Resources/resource.h`; unused chrome/icon headers deleted |
| `[x]` | Dead artifacts | Removed `msdf-atlas-gen-temp`, `inter_unzipped`, IgniteBT `vc140.pdb` |

**Verification:** every `*.Build.cs` module (except IgniteBT) has Public + Private; zero `IncludePaths.Add(".")`.

---

## 2. Public / Private API — resolved

| Status | Item | Resolution |
|--------|------|------------|
| `[x]` | ContentBrowser Services/Controllers/Registry | Demoted to Private; Public under `ContentBrowser/` |
| `[x]` | ContentBrowser Widgets | Nested as `Public/ContentBrowser/Widgets/` |
| `[x]` | KindUI UIWidgetAdapter / UIStateManager | Moved to Private |
| `[x]` | Extra WindEffects PublicIncludePaths | Stripped; use `Public/` + `WindEffects/...` paths |
| `[~]` | Editor Widgets/ at Public root | Deferred for non-ContentBrowser modules (cross-module include churn); ContentBrowser normalized |
| `[~]` | KindUI ControlChrome / ThemeManager / OverlayManager | Remain Public — consumed by Editor/Programs APIs |

---

## 3. Third-party — resolved

| Status | Item | Resolution |
|--------|------|------------|
| `[x]` | glm in Public headers | Replaced with `Core/Math/Types.h` (`we::math::*`) |
| `[x]` | Private glm interop | `Core/Private/Math/GlmInterop.h` |
| `[x]` | KindUI lunasvg | `PrivateIncludePaths` only |
| `[x]` | Dead third-party / junk dirs | Deleted |

**Remaining Private glm:** allowed in `.cpp` / Private headers via interop. Public APIs must not `#include <glm/...>`.

---

## 4. Dependencies — resolved

| Status | Item | Resolution |
|--------|------|------------|
| `[x]` | Runtime → Editor | None (verified) |
| `[x]` | Public → Private includes | None (verified) |
| `[x]` | VulkanRHI → Engine | Removed unused PrivateDependency |
| `[x]` | Viewport PlaceActors/Toolbar | Demoted to PrivateDependencies |
| `[x]` | Environment PropertyEditor/ContentBrowser/Menus | Demoted to PrivateDependencies |

---

## 5. Oversized files — resolved

All `.cpp` files now ≤ 800 lines. Notable splits:

| Area | New / remaining units |
|------|------------------------|
| DirectX12RHI | `DX12Internal.h`, `DX12Queue`, `DX12CommandList`, `DX12Swapchain`, `DX12Device{,Resources,Descriptors,Frame}` |
| VulkanRHI | `VulkanInternal.h`, `VulkanDevice{,Resources,Descriptors,Frame}`, renamed `VulkanVolk.cpp` |
| WeLauncher | `LauncherShell{,Pages,Settings,Create}`, `Launcher{Controls,NavControls,ContentControls}`, `Settings{Views,Controls,Fields}` |
| Platform | `WindowsPlatform{,Messages,Window}` + `WindowsPlatformInternal.h` |
| Editor program | `FirstRunAgreement{Popup,Markdown,Paint}` + `FirstRunAgreementInternal.h` |
| PlaceActors | `PlaceActorsPanel` + `PlaceActorsPanelPaint` |
| Environment | `EnvironmentEditorService` + `EnvironmentToolbar` |

**Headers still > 300 lines (acceptable / deferred):**

| Lines | File |
|------:|------|
| `[~]` 382 | `DirectX12RHI/Private/DX12Device.h` |
| `[~]` 374 | `RHI/Public/RHI/Desc.h` |
| `[~]` 373 | `VulkanRHI/Private/VulkanDevice.h` |
| `[~]` 323 | `KindUI/Public/KindUI/Core/Icon.h` |
| `[~]` 303 | WeLauncher `LauncherControls.h` / `SettingsViews.h` |

---

## 6. Naming / consistency

| Status | Item |
|--------|------|
| `[x]` | All `Export.h` use `#pragma once` |
| `[x]` | `VolkImpl.cpp` → `VulkanVolk.cpp` |
| `[x]` | Per-module `Export.h` pattern retained |

---

## Success criteria checklist

- [x] Every Build.cs module (except IgniteBT) has Public/ + Private/
- [x] No root source trees for libraries/programs (sources under Private/; Resources/ for .rc)
- [x] No `IncludePaths.Add(".")`
- [x] No Public → Private includes
- [x] No Runtime → Editor module deps
- [x] No third-party headers in Public APIs (`glm` / `lunasvg`)
- [x] Undocumented layout exceptions eliminated (IgniteBT + Engine/Shaders + Programs/Windows Resources documented)
