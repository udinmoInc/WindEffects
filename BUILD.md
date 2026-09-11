# Build Independence Guide

This document describes how to build WindEffects Engine from a clean checkout without access to private infrastructure or developer-specific configurations.

## Core Engine Components

### Required for Core Engine Build

The following components are **required** to build the core engine:

- **Visual Studio 2022** with C++ workload
- **.NET 8.0 SDK** for building IgniteBT
- **Windows 10/11 (64-bit)** operating system
- **Git** for version control

### Provided in Repository

The following third-party libraries are included in `Engine/ThirdParty/`:

- **Vulkan-Headers**: Vulkan API headers (used by default)
- **SDL3**: Platform abstraction layer (input, windowing)
- **GLM**: Math library (optional, used by some runtime systems)
- **freetype**: Font rendering
- **harfbuzz**: Text shaping
- **lunasvg**: SVG rendering
- **msdf-atlas-gen**: MSDF font atlas generation
- **stb**: Single-file libraries for textures, images, etc.
- **nanosvg**: NanoSVG rasterizer
- **volk**: Vulkan loader

### Not Provided (Optional Dependencies)

The following dependencies are **optional** and will be automatically disabled if unavailable:

- **nlohmann/json**: JSON library for serialization
  - Used by: Crash Reporter, Asset Pipeline, Asset Importer, KindUI
  - When missing: These components will be disabled with clear warning messages
  - Core engine: Will build successfully without it

## Optional Components

### Executables

The following executables are built but the core engine does not depend on them:

- **WindeffectsEditor.exe**: Editor application
- **WeLauncher.exe**: Launcher application
- **we.exe**: Command-line tool
- **WECrashReporter.exe**: Crash reporter (requires nlohmann/json)
- **ReflectionHardening.exe**: Reflection hardening tool
- **SerializationHardening.exe**: Serialization hardening tool

If these executables are not built (e.g., due to missing dependencies), the core engine and other executables will still build successfully.

### Optional Modules

The following modules use optional dependencies:

- **CrashReporter**: Requires nlohmann/json
- **AssetPipeline**: Requires nlohmann/json
- **AssetImporter**: Requires nlohmann/json
- **AssetCooker**: Requires nlohmann/json
- **AssetProcessors**: Requires nlohmann/json
- **AssetRuntime**: Requires nlohmann/json
- **KindUI**: Requires nlohmann/json
- **World, Scene, Renderer, VulkanRHI, Terrain, ECS, Engine**: Optionally use GLM

## Build Configuration

### Local SDK Configuration

The repository includes `IgniteBT.SDKs.json.example` as a template for local SDK configuration. To use it:

1. Copy `IgniteBT.SDKs.json.example` to `IgniteBT.SDKs.json`
2. Update the paths to match your local SDK installations
3. This file is gitignored, so local paths won't be committed

Example:
```json
{
  "SDKPaths": {
    "MSVC": "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207",
    "VulkanSDK": "C:\\VulkanSDK\\1.3.290.0"
  },
  "AdditionalIncludePaths": {},
  "AdditionalLibraryPaths": {}
}
```

Without local configuration, IgniteBT will attempt to discover SDKs automatically from standard installation locations.

### Environment Variables

The build system uses the following environment variables:

- **PATH**: Must include compiler, .NET SDK, and any SDK executables
- **VULKAN_SDK**: Optional override for Vulkan SDK location
- **WindowsSdkDir**: Automatically detected by the build system

## Third-Party Dependencies

### Bundled Dependencies

All third-party dependencies in `Engine/ThirdParty/` are:
- Git submodules (Vulkan-Headers, Vulkan-Loader, SDL3, etc.)
- Included in the repository for self-contained builds
- Do not require separate installation

### External Dependencies

The following are external dependencies that must be installed separately:

- **Visual Studio 2022**: https://visualstudio.microsoft.com/
- **.NET 8.0 SDK**: https://dotnet.microsoft.com/download
- **Vulkan SDK** (optional): https://vulkan.lunarg.com/

## Build from Clean Checkout

### Minimal Build Command

To build only the core engine components:

```powershell
.\we.ps1 build --config Debug
```

This will:
1. Build IgniteBT if not already built
2. Discover available SDKs
3. Build all modules with available dependencies
4. Disable optional components when dependencies are missing
5. Produce clear warning messages for disabled components

### Expected Warnings

When optional dependencies are missing, you will see warnings like:

```
[10:19:39 WRN] Optional dependency 'nlohmann_json' is not available, related features will be disabled
[10:19:39 WRN] Optional dependency 'DotNet' is not available, related features will be disabled
```

These are expected and do not indicate a build failure. The build will continue and produce a working core engine.

## Troubleshooting

### MSVC Not Found

If MSVC is not found automatically:
1. Ensure Visual Studio 2022 with C++ workload is installed
2. Add MSVC to PATH or configure in `IgniteBT.SDKs.json`
3. Run `we sdk detect` to verify discovery

### Vulkan SDK Not Found

If Vulkan SDK is not found:
1. This is not required - bundled headers are used by default
2. If you need a specific Vulkan SDK version, install and configure in `IgniteBT.SDKs.json`

### Optional Components Disabled

If optional components are disabled:
1. This is expected behavior when dependencies are missing
2. Core engine will still build successfully
3. Install missing dependencies to enable optional features

## Platform Support

### Currently Supported

- **Windows 10/11 (64-bit)**: Primary development platform

### Not Currently Supported

- **Linux**: Not yet supported
- **macOS**: Not yet supported
- **Consoles**: Not yet supported

## Dependency Philosophy

The build system follows these principles:

1. **Core First**: The core engine must build with only provided dependencies
2. **Optional Features**: Optional components must fail independently
3. **Clear Messages**: Missing dependencies must produce actionable warnings
4. **No Private Paths**: No hardcoded developer-specific paths in repository
5. **Self-Contained**: All required third-party code is included
6. **Discovery First**: SDKs are discovered automatically, manual configuration is optional
