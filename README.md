# 🌪️ WindEffects Engine

> **A next-generation AAA game engine built from scratch in modern C++23.**

WindEffects Engine is a high-performance, modular game engine designed for creating large-scale, visually advanced games. The engine focuses on modern rendering, scalable architecture, professional development tools, and an in-house editor built on a custom retained-mode UI framework.

> **Status:** 🚧 Early Development

---

## ✨ Features

### 🖥️ Rendering

* Vulkan Renderer
* DirectX 12 (Planned)
* Physically Based Rendering (PBR)
* Deferred & Forward+ Rendering
* HDR Rendering
* Render Graph
* GPU-driven Rendering
* Bindless Resources
* Ray Tracing (Planned)
* Path Tracing (Planned)
* DLSS / FSR / XeSS Support (Planned)

### 🎮 Engine Core

* Modern C++23
* Entity Component System (ECS)
* Job System
* Memory Allocators
* Reflection System
* Asset Pipeline
* Plugin Architecture
* Serialization
* Resource Management

### 🌍 World

* World Streaming
* Large World Coordinates
* Terrain System
* Foliage System
* Water System
* Procedural Generation
* HLOD
* Occlusion Culling

### 🎨 Editor

* Custom Retained-Mode UI Framework
* Dockable Workspace
* Scene Viewport
* World Outliner
* Property Inspector
* Content Browser
* Console
* Material Editor (Planned)
* Animation Editor (Planned)
* Visual Scripting (Planned)

### 🎬 Animation

* Skeletal Animation
* Animation Graph
* Blend Trees
* Full Body IK
* Motion Matching (Planned)
* Control Rig (Planned)

### 🤖 AI

* Navigation System
* Behavior Trees
* Utility AI
* Crowd Simulation (Planned)

### 🌐 Networking

* Dedicated Servers
* Replication
* Prediction
* Multiplayer Framework

---

# 📂 Project Structure

```text
WindEffects/

├── Engine/
│   ├── Source/
│   │   ├── Runtime/
│   │   ├── Editor/
│   │   └── Programs/
│   │       └── IgniteBT/
│   ├── ThirdParty/
│   ├── Content/
│   ├── Config/
│   ├── Shaders/
│   └── Tools/
│
├── WindEffects.engine
├── we
├── we.bat
├── we.ps1
│
├── Build/
│   ├── Output/
│   │   └── Win64/
│   │       ├── Debug/
│   │       ├── Development/
│   │       └── Shipping/
│   │           ├── WindeffectsEditor.exe
│   │           ├── WECore.dll
│   │           ├── WECoreUObject.dll
│   │           ├── WEEngine.dll
│   │           ├── Runtime/
│   │           │   └── Renderer/
│   │           │       └── WERenderer.dll
│   │           ├── Editor/
│   │           │   └── ContentBrowser/
│   │           │       └── WEContentBrowser.dll
│   │           ├── Config/
│   │           ├── Content/
│   │           └── Resources/
│   ├── Intermediate/
│   │   ├── IgniteBT/
│   │   └── Win64/Debug/            # Objects, PDB, incremental link data
│   ├── Generated/
│   ├── Cache/
│   ├── Database/
│   ├── Logs/
│   ├── Reports/
│   ├── Manifest/
│   └── Temp/
│
├── Assets/
├── Legal/
└── IgniteBT.SDKs.json
```

---

# 🚀 Technology Stack

| Component       | Technology              |
| --------------- | ----------------------- |
| Language        | C++23                   |
| Graphics        | Vulkan                  |
| Build           | IgniteBT                |
| Shader Language | HLSL / SPIR-V           |
| UI Framework    | Custom Retained-Mode UI |
| Asset Pipeline  | Custom                  |
| Physics         | Planned                 |
| Audio           | Planned                 |
| Scripting       | Planned                 |

---

# 🎯 Vision

WindEffects is designed as a long-term proprietary engine capable of powering:

* Open World Games
* RPGs
* FPS Games
* Racing Games
* Simulation Games
* Multiplayer Titles

The goal is to provide a scalable engine and editor suitable for professional game development while remaining modular and maintainable.

---

# 🔧 Building

WindEffects is built exclusively with **IgniteBT**, exposed through the cross-platform **`we`** command-line interface. All generated artifacts are written outside the source tree.

## CLI setup

From the repository root:

```powershell
# PowerShell (outside Cursor/VS Code): prefix with .\ until setup is run
.\we clean
.\we build --config Debug

# After one-time setup, `we` works from any directory:
.\we setup
```

In **Cursor / VS Code** integrated terminals, `we` works immediately (the workspace root is on PATH).

```powershell
we build --config Debug
we doctor
```

Restart your terminal after `we setup` if using an external shell.

## Common commands

```powershell
we build --config Debug
we clean --config Debug
we rebuild --config Debug
we run --target Editor --config Debug
we package
we doctor
we sdk detect
we plugin list
we project list
we version
```

## Bootstrapping IgniteBT

The root `we` launchers forward to IgniteBT. On first use they fall back to `dotnet run` if IgniteBT has not been built yet:

```powershell
dotnet build Engine/Source/Programs/IgniteBT/IgniteBT.csproj -c Debug
we build --config Debug
```

Repository-root launchers (`we`, `we.bat`, `we.ps1`) are permanent thin bootstrappers. They search upward for `WindEffects.engine`, load `Build/Manifest/bootstrap.manifest`, resolve the requested tool dynamically, and forward every argument unchanged. IgniteBT regenerates the bootstrap manifest on every execution. All commands and build logic live in IgniteBT.

### Directory layout

| Path | Purpose |
|------|---------|
| `Engine/Source/` | Human-authored source code only |
| `Engine/ThirdParty/` | Third-party library sources (e.g. GLM) |
| `Build/` | All generated data — disposable, gitignored, recreated by IgniteBT |
| `Build/Intermediate/` | Object files, PDBs, incremental link data, IgniteBT tool output |
| `Build/Generated/` | Build-time generated files (export `.def` files, etc.) |
| `Build/Output/Win64/Debug/` | Final binaries in a modular layout (bootstrap EXEs/DLLs at the configuration root) |

---

## Phase 1

* [ ] Core Engine
* [ ] Window System
* [ ] Vulkan Renderer
* [ ] Asset Manager
* [ ] Scene System
* [ ] Editor Foundation

## Phase 2

* [ ] ECS
* [ ] Physics Integration
* [ ] Audio System
* [ ] Input Framework
* [ ] Material System

## Phase 3

* [ ] Animation
* [ ] Terrain
* [ ] World Streaming
* [ ] Navigation
* [ ] AI Framework

## Phase 4

* [ ] Networking
* [ ] Visual Scripting
* [ ] Packaging
* [ ] Production Tools

---

# 🤝 Contributing

Contributions, feedback, and discussions are welcome as the engine evolves.

Please open an issue before submitting large changes to discuss the proposed implementation.

---

# 📜 License

This repository is currently under active development.

License information will be added when the project reaches a stable public release.

---

<p align="center">
Built with ❤️ using Modern C++23.
</p>