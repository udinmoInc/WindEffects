# IgniteBT Build System

IgniteBT is the build system component of WindEffects Engine, designed for large-scale C++ projects. Built on .NET 8.0, it provides build automation with advanced dependency management, distributed compilation, and intelligent caching.

[![.NET](https://img.shields.io/badge/.NET-8.0-purple.svg)](https://dotnet.microsoft.com/)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)](https://github.com/udinmoInc/WindEffects)

---

## Overview

IgniteBT handles compilation workflows across multiple configurations and platforms. The system is architected for scalability, supporting projects with thousands of source files and hundreds of dependencies while maintaining fast incremental build times.

IgniteBT solves the fundamental challenges of large-scale C++ development: managing complex dependency graphs, optimizing compilation throughput, and providing immediate feedback on changes. It achieves this through dependency analysis, parallel execution, and intelligent caching that minimizes redundant work.

### Key Capabilities

| Capability | Description |
|------------|-------------|
| **Advanced Dependency Resolution** | Automatic detection and management of inter-module dependencies with granular analysis including transitive dependencies and header file impact tracking |
| **Parallel Compilation** | Multi-threaded build execution with dependency-aware scheduling that scales to all available CPU cores |
| **Intelligent Caching** | Multi-level caching including object files, precompiled headers, and dependency graphs with content-addressable storage |
| **Distributed Compilation** | Optional master-worker architecture for distributing work across multiple machines with fault tolerance |
| **SDK Management** | Unified SDK detection and validation with diagnostic capabilities |
| **Cross-Platform Support** | Configurable build targets for Windows, Linux, and macOS with platform abstraction |
| **Real-Time Diagnostics** | Structured logging with environment validation and dependency verification |

---

## Architecture

IgniteBT is organized into four layers with well-defined interfaces:

```
┌─────────────────────────────────────────────────────────────┐
│                    Command Interface Layer                   │
│  (CLI Commands, Argument Parsing, User Interaction)          │
└─────────────────────────────────────────────────────────────┘
                               │
┌─────────────────────────────────────────────────────────────┐
│                   Build Orchestration Layer                   │
│  (Scheduler, Compiler, Linker, Dependency Graph)             │
└─────────────────────────────────────────────────────────────┘
                               │
┌─────────────────────────────────────────────────────────────┐
│                    Workspace Management Layer                 │
│  (Module Discovery, SDK Detection, Configuration)             │
└─────────────────────────────────────────────────────────────┘
                               │
┌─────────────────────────────────────────────────────────────┐
│                      Core Infrastructure                     │
│  (Filesystem, Hashing, Threading, Database, Logging)         │
└─────────────────────────────────────────────────────────────┘
```

| Layer | Responsibility | Key Components |
|-------|---------------|----------------|
| Command Interface | User-facing CLI | Command dispatcher, argument parser, help system |
| Build Orchestration | Compilation pipeline | Scheduler, compiler, linker, dependency graph |
| Workspace Management | Project structure | Module discovery, SDK detection, configuration |
| Core Infrastructure | Foundational services | Filesystem, hashing, threading, database, logging |

---

## Quick Start

### Installation

IgniteBT is integrated directly into WindEffects Engine projects. No separate installation is required. The bootstrap launcher automatically builds IgniteBT on first use using the .NET SDK.

### Basic Usage

```powershell
# Build in Development configuration
we build --config Development

# Clean build artifacts
we clean --config Debug

# Rebuild specific target
we rebuild --target WECore --config Development

# Run the editor
we run --target Editor --config Development
```

---

## Documentation

| Document | Description |
|----------|-------------|
| [Getting Started Guide](./GETTING-STARTED.md) | Environment setup and first build |
| [Architecture Documentation](./ARCHITECTURE.md) | System design and component organization |
| [Command Reference](./COMMANDS.md) | `we` CLI commands |
| [Configuration Guide](./CONFIGURATION.md) | SDK configuration and build settings |

---

## System Requirements

| Requirement | Version | Purpose |
|-------------|---------|---------|
| Operating System | Windows 10/11 (primary), Linux, macOS | Platform support |
| .NET SDK | 8.0 | Build system runtime |
| Visual Studio | 2022 with C++ workload | MSVC compiler and toolchain |

> **Important**: Download .NET SDK from the official Microsoft website and add to system PATH. Visual Studio 2022 should include the latest updates.

---

## Support

IgniteBT is a component of WindEffects Engine. Support is provided through the same channels as the main engine.

**Troubleshooting:**
1. Consult the WindEffects Engine documentation
2. Run `we doctor` for comprehensive environment diagnostics
3. Report issues through official WindEffects Engine support channels with detailed information

---

## License

IgniteBT is part of the WindEffects Engine project and subject to the WindEffects Engine EULA. See [Legal/EULA.md](../../../../../Legal/EULA.md) for terms and conditions.

---

## Quick Links

- [Getting Started Guide](./GETTING-STARTED.md)
- [Architecture Documentation](./ARCHITECTURE.md)
- [Command Reference](./COMMANDS.md)
- [Configuration Guide](./CONFIGURATION.md)
- [Changelog](../../../../../CHANGELOG.md)