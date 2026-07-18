# Production Stabilization Report

**Date:** 2026-07-18  
**Scope:** Compile health, math migration finish, module/deps, CI hard gates, packaging, smoke validation  
**Architecture stance:** No folder moves or architectural rewrites; correctness and ship gates only.

---

## Verdict

### Recommendation: **Ready for daily production development**

### Production readiness score: **82 / 100**

Previous audit score was **44 / 100** (Editor Development did not link). This pass brings all requested Editor configurations and Unity modes to green locally, implements packaging, hardens CI, and restores automated test targets.

---

## Build results

| Configuration | Result |
|---------------|--------|
| Editor Development (Unity OFF) | Pass — `WindeffectsEditor.exe` links |
| Editor Development (Unity ON) | Pass (after Unity planner fix) |
| Editor Debug | Pass |
| Editor Shipping | Pass |
| Incremental Development | Pass (cache / link path) |
| Clean rebuild (`--clean`) | Pass after clean-arg forwarding fix |
| ECSTests | Pass — **34 / 34** |
| TextRenderTest | Pass — glyph layout OK |
| `we package --config Shipping` | Pass — zip under `Build/Packages/` (~4.1 MB compressed, 133 entries including Assets) |
| Editor smoke (launch ≥8s) | Pass — process stayed alive until stopped |

---

## Issues fixed

### Compile / link
- Restored missing `FirstRunAgreementPopup::BuildDefaultContent()` (linker LNK2019).
- Finished remaining `we::math` ↔ glm Private interop (public APIs stay on `we::math`; Private uses `GlmInterop`).
- Fixed KindUI / Text transitive include paths (IgniteBT walks PublicDependencies).
- Fixed Windows platform preprocessor balance (`#endif` / namespace closures).
- Made DX12/Vulkan internal helpers `inline` (LNK2005 ODR).
- Fixed ECSTests `worldMatrix` access for column-major `we::math::Mat4` (`m[12]`).

### Unity builds
- **Bug:** `UnityBuildPlanner` dropped TUs under `MinLinesForUnity` (e.g. `Environment.cpp`, `FrameCounter.cpp`) → unresolved symbols with `--unity`.
- **Fix:** leftover sources are always emitted as solo blobs.
- **Bug:** orchestrator no-op ignored unity flags in config hash → Unity ON/OFF could falsely no-op.
- **Fix:** `ComputeConfigHash` includes build flags (unity/jobs/target).

### Output layout / subset targets
- **Bug:** building ECSTests/TextRenderTest deleted Editor root RHI DLLs (`PruneConfigurationRoot` treated `.dll` as sidecars; legacy cleanup deleted unrelated root binaries).
- **Fix:** prune only `.lib/.exp/.ilk/.pdb`; remove only misplaced binaries for modules in the current build.

### Packaging / CLI
- Implemented `we package` (stage product tree → `Build/Packages/*.zip` + `package.json`).
- `--clean` on build now runs Clean with filtered args (no longer rejects `--jobs`/`--clean`).
- Rebuild forwards only clean-compatible args.

### CI
- `.github/workflows/ignitebt-ci.yml`: removed `continue-on-error` on engine builds; expands paths to `Engine/**`; gates Editor Development (Unity OFF/ON), Debug, Shipping, ECSTests, TextRenderTest, and packaging.

---

## Files modified (high level)

| Area | Paths |
|------|--------|
| FirstRun | `Engine/Source/Programs/Editor/Private/FirstRunAgreementMarkdown.cpp` |
| Math / Glm | `Core/Public/Core/Math/Types.h`, `GlmInterop.h`, Scene/ECS/Renderer/Engine Private TUs |
| Platform | `WindowsPlatform*.cpp` preprocessor closures |
| RHI | `DX12Internal.h`, `VulkanInternal.h` (`inline`) |
| IgniteBT | `UnityBuildPlanner.cs`, `GraphSerializer.cs`, `BuildOrchestrator.cs`, `FastNoOpProbe.cs`, `BuildCommand.cs`, `RebuildCommand.cs`, `PackageCommand.cs`, `OutputLayout.cs`, `Program.cs`, `CommandLineParser.cs` |
| Tests | `ECSTestsMain.cpp`, `UnityBuildPlannerTests.cs`, `FastNoOpProbeTests.cs` |
| CI | `.github/workflows/ignitebt-ci.yml` |

---

## Test results

| Suite | Result |
|-------|--------|
| IgniteBT.Tests (xUnit) | Pass (17 tests including Unity planner) |
| ECSTests.exe | Pass — Passed=34 Failed=0 |
| TextRenderTest.exe | Pass — exit 0 |
| Editor smoke launch | Pass — alive after 8s |

---

## Remaining technical debt

1. **GitHub-hosted runners** may lack the local MSVC/Vulkan/SDL layout; CI hard-gates assume a capable Windows image or self-hosted agent with IgniteBT SDKs.
2. **Shader warning:** missing `Engine/Shaders/Editor/EditorBackground.hlsl` (bytecode path still stages related assets).
3. **Icon atlas:** `we-icon-compile` not found — atlas refresh skipped.
4. **Optional SDKs:** DotNet / Android / OpenXR / DirectX SDK detection warnings (non-blocking for Win64 Editor).
5. **glm remains in Private** for transforms/camera — intentional; public surface is `we::math`.
6. **LNK4197** duplicate export warnings across several modules — noisy, not link-blocking.
7. **Runtime memory/threading/editor panel deep validation** not instrumented in this pass (smoke only: process start).
8. **No-op detector** still does not verify on-disk product binaries exist before skipping link (subset-build damage was fixed at prune time; missing-binary repair still needs an explicit rebuild).

---

## Score breakdown

| Area | Score | Notes |
|------|------:|-------|
| Build matrix | 95 | Dev/Debug/Shipping + Unity ON/OFF green locally |
| Math / API hygiene | 88 | Public `we::math`; Private glm via interop |
| Module / includes | 85 | Transitive PublicDependencies; KindUI/Text fixed |
| CI | 75 | Hard gates present; runner SDK readiness is residual risk |
| Packaging | 85 | Real zip + assets; not a stub |
| Automated tests | 90 | ECS + Text + IgniteBT unit tests |
| Runtime / editor depth | 55 | Launch smoke only |
| **Overall** | **82** | Ready for daily prod development with known debt |

---

## How to verify locally

```powershell
$env:IGNITEBT_SKIP_DAEMON = "1"
.\we.ps1 build --target Editor --config Development --jobs 4
.\we.ps1 build --target Editor --config Development --jobs 4 --unity
.\we.ps1 build --target Editor --config Debug --jobs 4
.\we.ps1 build --target Editor --config Shipping --jobs 4
.\we.ps1 build --target ECSTests --config Development --jobs 4
.\Build\Output\Win64\Development\ECSTests.exe
.\we.ps1 build --target TextRenderTest --config Development --jobs 4
.\Build\Output\Win64\Development\TextRenderTest.exe
.\we.ps1 package --target Editor --config Shipping --skip-build
```
