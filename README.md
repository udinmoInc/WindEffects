# WindEffects Engine

WindEffects Engine is a next-generation game engine designed for professional game development, built from the ground up using modern C++23 and cutting-edge graphics technologies. This document provides a comprehensive overview of the engine, its architecture, features, and how to get started with building and using it.

## Table of Contents

- [Introduction](#introduction)
- [Architecture Overview](#architecture-overview)
- [Core Features](#core-features)
- [Rendering System](#rendering-system)
- [Editor Interface](#editor-interface)
- [Getting Started](#getting-started)
- [Building the Engine](#building-the-engine)
- [Project Structure](#project-structure)
- [Development Workflow](#development-workflow)
- [Roadmap](#roadmap)
- [Contributing](#contributing)
- [License](#license)

## Introduction

WindEffects Engine represents a modern approach to game engine design, leveraging the latest advancements in C++23, Vulkan graphics API, and contemporary software architecture patterns. The engine is designed to be modular, performant, and extensible, making it suitable for a wide range of game development projects from indie titles to AAA productions.

The project began with the goal of creating a proprietary engine that could compete with established industry leaders while maintaining a clean, maintainable codebase. WindEffects focuses on providing developers with powerful tools through an integrated editor while keeping the underlying systems accessible and well-documented.

### Design Philosophy

The engine follows several core design principles:

- **Modularity**: Each system is designed as an independent module that can be developed, tested, and maintained in isolation
- **Performance**: Critical paths are optimized for modern hardware architectures with multi-threading and GPU acceleration
- **Modern C++**: Extensive use of C++23 features including concepts, modules, and improved standard library facilities
- **Data-Oriented Design**: Memory layouts and data structures are optimized for cache efficiency and SIMD operations
- **Editor-First**: The development experience centers around a powerful, intuitive editor interface

## Architecture Overview

WindEffects Engine is organized into several major subsystems, each responsible for specific aspects of game development. This modular architecture allows teams to work on different systems independently while maintaining clear interfaces between components.

### Engine Core

The core system provides foundational services that all other subsystems depend on. This includes memory management, threading primitives, reflection systems, and the entity-component-system (ECS) architecture. The core is designed to be lightweight and efficient, with minimal overhead for the systems built on top of it.

The memory management system uses custom allocators optimized for different allocation patterns - small object allocations, large resource buffers, and GPU memory are each handled by specialized allocators that reduce fragmentation and improve allocation speed.

### Build System

WindEffects uses IgniteBT, a custom build system written in C#, as its primary build tool. IgniteBT provides a unified command-line interface through the `we` command, handling dependency resolution, compilation, linking, and packaging. The build system is designed to keep all generated artifacts outside the source tree, maintaining a clean separation between source and build outputs.

The `we` command serves as the primary interface for all build operations, from simple compilation to complex packaging and deployment scenarios. It automatically detects the development environment, manages dependencies, and provides helpful diagnostics through the `doctor` command.

## Core Features

### Entity Component System

At the heart of WindEffects lies a modern Entity Component System (ECS) architecture. Unlike traditional object-oriented game object hierarchies, ECS separates data from behavior, enabling efficient data-oriented processing and cache-friendly memory layouts.

Entities serve as unique identifiers, components hold pure data, and systems operate on components matching specific queries. This architecture enables:
- Efficient parallel processing of game logic
- Cache-friendly data access patterns
- Flexible composition of game objects
- Easy serialization and networking

The ECS implementation supports archetype-based storage, where entities with the same component types are stored contiguously in memory, maximizing cache locality during system updates.

### Job System

The job system provides a framework for parallelizing work across multiple CPU cores. It supports task dependencies, work stealing, and affinity scheduling to maximize hardware utilization. Developers can submit jobs either as individual tasks or as parallel-for loops that automatically partition work across available threads.

The job system integrates closely with the ECS, allowing systems to schedule parallel updates for entity queries. This enables massive entity counts to be processed efficiently by distributing the workload across all available CPU cores.

### Asset Pipeline

WindEffects includes a custom asset pipeline that handles importing, processing, and managing game assets. The pipeline supports various asset types including meshes, textures, materials, animations, and audio files. Assets are processed offline into optimized formats ready for runtime loading.

The asset system includes hot-reloading capabilities during development, allowing artists and designers to see changes immediately without restarting the editor. Asset references use stable GUIDs, enabling safe refactoring and asset management without breaking existing content.

### Reflection System

A comprehensive reflection system enables runtime type information, serialization, and editor integration. The reflection system is built at compile time using C++ attributes and code generation, providing zero runtime overhead while enabling powerful metaprogramming capabilities.

This system powers the property inspector, serialization, and scripting interfaces, allowing the editor to inspect and modify any reflected type without manual binding code.

## Rendering System

The rendering system is built on Vulkan, providing low-level access to modern GPU capabilities while maintaining a higher-level abstraction for common rendering tasks. The renderer supports multiple rendering techniques and is designed to scale from integrated graphics to high-end GPUs.

### Graphics Features

- **Physically Based Rendering (PBR)**: Material system based on modern PBR workflows with support for metallic/roughness and specular/glossiness workflows
- **Deferred and Forward+ Rendering**: Multiple rendering paths optimized for different scenarios - deferred for complex lighting, forward+ for transparency and performance
- **HDR Rendering**: High dynamic range rendering with tone mapping and exposure control for realistic lighting
- **Render Graph**: Frame graph system that automatically manages render pass dependencies and resource transitions
- **GPU-Driven Rendering**: Compute-based culling and draw dispatch for handling massive scene complexity
- **Bindless Resources**: Descriptor indexing for efficient resource access without descriptor set limits
- **Ray Tracing**: Hardware-accelerated ray tracing for realistic reflections, shadows, and global illumination (planned)
- **Path Tracing**: Full path tracing for cinematic-quality rendering (planned)

### Shader Pipeline

Shaders are written in HLSL and compiled to SPIR-V for Vulkan consumption. The shader pipeline includes a unified shader format that can target different APIs, making future DirectX 12 support straightforward. The system supports automatic shader permutation generation for different quality settings and feature combinations.

## Editor Interface

The WindEffects Editor provides a comprehensive development environment for game creation. Built on a custom retained-mode UI framework, the editor offers a responsive and intuitive interface that can be customized to fit individual workflows.

### Main Components

**Scene Viewport**: The central workspace for viewing and interacting with your game world. The viewport supports real-time rendering with multiple camera modes, including perspective, orthographic, and custom camera views. Gizmos and manipulation tools enable precise object placement and transformation.

**World Outliner**: A hierarchical view of all entities in the current scene. The outliner supports filtering, searching, and organizing entities into groups for complex scene management. Entity selection in the outliner syncs with the viewport and property inspector.

**Property Inspector**: Displays and allows editing of all components attached to the selected entity. The inspector uses the reflection system to automatically generate appropriate UI controls for each property, supporting custom editors for complex types.

**Content Browser**: Asset management interface for browsing, importing, and organizing project assets. The browser supports previewing assets, filtering by type, and drag-and-drop operations for quick asset placement.

**Console**: Integrated command console for executing engine commands, viewing log output, and debugging. The console supports command auto-completion and custom command registration.

### Workspace Customization

The editor features a fully dockable workspace layout. Panels can be rearranged, docked, undocked, or tabbed together to create custom layouts. Multiple workspace configurations can be saved and switched between depending on the current task.

## Getting Started

### Prerequisites

Before building WindEffects Engine, ensure your development environment meets the following requirements:

- **Operating System**: Windows 10 or Windows 11 (64-bit)
- **Compiler**: Visual Studio 2022 with C++ workload and latest updates
- **.NET SDK**: .NET 8.0 SDK for building the IgniteBT build system
- **Vulkan SDK**: Latest Vulkan SDK from Vulkan SDK for graphics development
- **Git**: For version control operations
- **CMake**: (Optional) Some third-party dependencies may use CMake

### Initial Setup

1. Clone the repository to your local machine
2. Ensure all prerequisites are installed and accessible from your command line
3. Open a terminal in the repository root
4. Run the setup command to configure the build environment

The first build will take longer as it compiles the IgniteBT build system and downloads any required dependencies.

## Building the Engine

WindEffects uses the `we` command-line tool for all build operations. This tool is a thin wrapper around the IgniteBT build system and provides a consistent interface across different platforms and development environments.

### Basic Build Commands

From the repository root, the following commands are available:

```powershell
# Build the engine in Debug configuration
we build --config Debug

# Build in Development configuration (optimized with debugging symbols)
we build --config Development

# Build in Shipping configuration (fully optimized)
we build --config Shipping

# Clean build artifacts for a specific configuration
we clean --config Debug

# Rebuild (clean then build) for a specific configuration
we rebuild --config Debug

# Run the editor after building
we run --target Editor --config Debug
```

### Advanced Build Options

The build system supports additional options for fine-grained control:

```powershell
# Build specific modules or targets
we build --target WECore --config Debug
we build --target WERenderer --config Debug

# Build with increased parallelism
we build --config Debug --jobs 8

# Generate build reports
we build --config Debug --report

# Verbose output for debugging build issues
we build --config Debug --verbose
```

### Diagnostic Commands

Several diagnostic commands help troubleshoot build and environment issues:

```powershell
# Check environment and dependencies
we doctor

# Display version information
we version

# Detect installed SDKs and tools
we sdk detect

# List available plugins
we plugin list

# List configured projects
we project list
```

### Bootstrapping IgniteBT

On first use, the `we` command may need to bootstrap the IgniteBT build system. If IgniteBT hasn't been built yet, the launcher will automatically fall back to using `dotnet run`:

```powershell
dotnet build Engine/Source/Programs/IgniteBT/Source/IgniteBT.csproj -c Debug
we build --config Debug
```

After the initial bootstrap, IgniteBT is compiled and cached, making subsequent builds faster.

## Project Structure

Understanding the project structure is essential for navigating the codebase and knowing where to make changes:

```
WindEffects/
├── Engine/
│   ├── Source/              # All engine source code
│   │   ├── Runtime/         # Runtime engine systems
│   │   ├── Editor/          # Editor-specific code
│   │   └── Programs/        # Standalone tools
│   │       └── IgniteBT/    # Build system (Source, Launcher, Tests, Docs)
│   ├── ThirdParty/          # Third-party library sources
│   ├── Content/             # Engine assets and resources
│   ├── Config/              # Configuration files
│   ├── Shaders/             # Shader source files
│   └── Tools/               # Build and development tools
├── Build/                   # Generated build artifacts (gitignored)
│   ├── Output/              # Final compiled binaries
│   ├── Intermediate/        # Object files and incremental data
│   ├── Generated/           # Generated source files
│   ├── Cache/               # Build cache
│   ├── Database/            # Asset database
│   ├── Logs/                # Build and runtime logs
│   └── Manifest/            # Build manifests
├── Assets/                  # Project-specific assets
├── Legal/                   # Legal documents and licenses
├── WindEffects.engine       # Project descriptor file
├── we                       # Unix-style launcher script
├── we.bat                   # Windows batch launcher
├── we.ps1                   # PowerShell launcher
└── IgniteBT.SDKs.json       # SDK configuration
```

### Source Organization

The `Engine/Source` directory is the heart of the codebase:

- **Runtime**: Contains all engine systems that run in both editor and shipped games. This includes the core systems, renderer, physics, audio, and game framework code.
- **Editor**: Contains editor-specific functionality including UI, tools, and editor-only systems. This code is not included in shipped games.
- **Programs**: Contains standalone tools and utilities. IgniteBT (the build system) lives under `Programs/IgniteBT/` with its source, launcher, tests, and documentation colocated.

### Build Artifacts

The `Build` directory contains all generated files and is excluded from version control:

- **Output**: Contains the final compiled binaries organized by configuration and platform. This is where you'll find the editor executable and engine DLLs.
- **Intermediate**: Contains object files, PDB debug symbols, and incremental link data. This directory can be safely deleted to force a clean rebuild.
- **Generated**: Contains files generated at build time, such as export definition files and generated reflection code.
- **Cache**: Contains cached data to speed up subsequent builds.
- **Database**: Contains the asset database used by the content browser and asset pipeline.
- **Logs**: Contains build logs and runtime logs from the editor and engine.
- **Manifest**: Contains build manifests used by the launcher and build system.

## Development Workflow

### Typical Development Cycle

A typical development session might follow this pattern:

1. **Pull latest changes**: Ensure your local repository is up to date
2. **Build the engine**: Run `we build --config Development` to compile changes
3. **Launch the editor**: Run `we run --target Editor --config Development`
4. **Work in the editor**: Create content, test features, iterate on gameplay
5. **Make code changes**: Edit source files as needed
6. **Rebuild**: Run `we build --config Development` to compile changes
7. **Test**: Verify changes work as expected in the editor
8. **Commit**: Commit changes with descriptive messages

### Debugging

For debugging engine code, use the Development configuration which includes debug symbols while maintaining reasonable performance. The Debug configuration includes full debugging but may be too slow for interactive work.

To attach a debugger:
1. Build in Development or Debug configuration
2. Launch the editor through your debugger of choice
3. Set breakpoints in engine source code
4. Interact with the editor to trigger the code paths you want to debug

### Hot Reloading

During development, many changes can be hot-reloaded without restarting the editor:
- Shader changes are automatically recompiled and reloaded
- Asset changes trigger automatic reimport
- C++ code changes currently require a rebuild and editor restart (hot-reload is planned for future releases)

## Roadmap

WindEffects Engine is under active development with a planned roadmap of features:

### Phase 1: Foundation (Current)
- Core engine systems and architecture
- Window system and platform abstraction
- Vulkan renderer with basic PBR
- Asset manager and basic asset pipeline
- Scene system and entity management
- Editor foundation and basic UI

### Phase 2: Core Systems
- Entity Component System implementation
- Physics integration (planned)
- Audio system (planned)
- Input framework and action mapping
- Material system and shader editor

### Phase 3: World Systems
- Animation system and skeletal meshes
- Terrain system with heightmaps and splatmaps
- World streaming for large environments
- Navigation mesh generation and AI pathfinding
- AI framework with behavior trees

### Phase 4: Advanced Features
- Networking and multiplayer support
- Visual scripting system
- Packaging and deployment tools
- Production-ready profiling tools
- Additional platform support (Linux, consoles)

## Contributing

WindEffects Engine is currently in early development and is not yet accepting external contributions. However, feedback, bug reports, and discussions are welcome as the project evolves.

When the project opens for contributions, guidelines will be provided covering:
- Code style and formatting standards
- Pull request process
- Testing requirements
- Documentation expectations

### Reporting Issues

If you encounter issues while using WindEffects, please provide:
- Steps to reproduce the problem
- Expected behavior vs. actual behavior
- Environment information (OS, hardware, configuration)
- Relevant log files or error messages
- Screenshots or recordings if applicable

## License

WindEffects Engine is currently under active development and is not yet publicly licensed. License information will be provided when the project reaches a stable public release.

For licensing inquiries or questions about using WindEffects Engine in commercial projects, please contact the development team.

## Acknowledgments

WindEffects Engine incorporates third-party software and assets. Each component is subject to its respective license terms. See the Legal/THIRD_PARTY_NOTICES.md file for detailed information about third-party components and their licenses.

## Support

For technical support, questions, or inquiries about WindEffects Engine, please contact the development team through the official channels provided in the project documentation.

---

WindEffects Engine is built with passion and dedication to advancing the state of game development technology. We hope it serves as a solid foundation for creating amazing games and experiences.