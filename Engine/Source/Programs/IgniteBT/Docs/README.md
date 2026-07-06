# 🚀 IgniteBT Build System

> IgniteBT is the build system component of WindEffects Engine, designed for large-scale C++ projects. Built on .NET 8.0, IgniteBT provides build automation with advanced dependency management, distributed compilation, and intelligent caching for the WindEffects Engine. The system addresses the complex needs of game engine development where projects encompass thousands of source files, hundreds of dependencies, and multiple target platforms. IgniteBT is architected as an integral part of WindEffects Engine to handle these challenges while maintaining the speed and reliability required for professional development workflows.

![.NET](https://img.shields.io/badge/.NET-8.0-purple.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)

---

## 📖 Overview

IgniteBT is the build system component of WindEffects Engine, handling compilation workflows across multiple configurations and platforms. The system is architected for scalability, supporting projects with thousands of source files and hundreds of dependencies while maintaining fast incremental build times. IgniteBT solves the fundamental challenges of large-scale C++ development within WindEffects Engine: managing complex dependency graphs, optimizing compilation throughput, and providing developers with immediate feedback on their changes. The system achieves these goals through dependency analysis, parallel execution strategies, and intelligent caching mechanisms that minimize redundant work.

The build system is designed for WindEffects Engine development where iteration speed is critical. The engine requires frequent rebuilds as developers modify core systems, add new features, or fix bugs. IgniteBT addresses this by implementing incremental build techniques that only recompile what has actually changed, even when modifications occur in header files that affect many translation units. This approach, combined with parallel compilation across multiple CPU cores and distributed build capabilities, ensures that developers can maintain their productivity as the WindEffects Engine project grows in size and complexity.

### ✨ Key Capabilities

IgniteBT provides a comprehensive set of capabilities designed to address the full spectrum of build system requirements for large-scale C++ projects. The system's advanced dependency resolution automatically detects and manages inter-module dependencies, eliminating the need for manual dependency specification and reducing the risk of build errors caused by missing or incorrect dependency declarations. This capability is particularly valuable in large codebases where manually tracking dependencies would be error-prone and time-consuming. The dependency analysis is performed at a granular level, taking into account not only direct dependencies between modules but also transitive dependencies and the impact of header file changes across the entire codebase.

| Capability | Description |
|------------|-------------|
| 🔗 **Advanced Dependency Resolution** | Automatic detection and management of inter-module dependencies with granular analysis including transitive dependencies and header file impact tracking |
| ⚡ **Parallel Compilation** | Sophisticated multi-threaded build execution with dependency-aware scheduling that scales to utilize all available CPU cores |
| 💾 **Intelligent Caching** | Multi-level caching system including object files, precompiled headers, and dependency graphs with content-addressable storage |
| 🌐 **Distributed Compilation** | Optional master-worker architecture for distributing build work across multiple machines with fault tolerance and automatic retry |
| 🛠️ **SDK Management** | Unified SDK detection and validation for required libraries with diagnostic capabilities |
| 🖥️ **Cross-Platform Support** | Configurable build targets for Windows, Linux, and macOS with platform abstraction layer |
| 🔍 **Real-Time Diagnostics** | Structured logging at multiple levels with comprehensive environment validation and dependency verification |

Parallel compilation is another cornerstone of IgniteBT's performance optimization strategy. The system implements a sophisticated multi-threaded build execution engine with configurable job scheduling that can automatically scale to utilize all available CPU cores. The parallel execution is not limited to simple file-level parallelism; IgniteBT performs dependency-aware scheduling that respects the build dependency graph while maximizing parallel execution of independent tasks. This approach ensures that build times scale linearly with the number of available CPU cores, making it possible to complete even large builds in a reasonable amount of time. The job scheduling algorithm takes into account factors such as task duration estimates, resource availability, and critical path analysis to optimize overall build throughput.

Intelligent caching is implemented throughout the system to minimize redundant compilation and build operations. IgniteBT maintains a comprehensive build cache that stores compilation results, dependency information, and intermediate artifacts. When a build is initiated, the system checks the cache to determine which artifacts can be reused and which need to be rebuilt. This caching operates at multiple levels, including object file caching, precompiled header caching, and dependency graph caching. The cache is content-addressable, meaning that artifacts are stored and retrieved based on cryptographic hashes of their inputs, ensuring cache correctness even in the presence of complex dependency changes. The caching system also implements intelligent invalidation policies that balance cache hit rates with cache size management.

For organizations with multiple build machines, IgniteBT offers optional distributed compilation support that can distribute build work across multiple machines. This capability is particularly valuable for very large projects where even parallel compilation on a single machine may not provide sufficient throughput. The distributed build system implements a master-worker architecture where a master node coordinates the distribution of build tasks to worker nodes, aggregates results, and manages the overall build process. The system handles network-transparent file access, fault tolerance, and automatic retry of failed tasks, ensuring that distributed builds are as reliable as local builds. Distributed builds can significantly reduce build times for large teams by leveraging the combined computational power of multiple machines.

SDK management is unified within IgniteBT, providing detection and validation of development SDKs and required libraries. The system automatically detects installed SDKs from standard installation locations, validates SDK versions against project requirements, and manages SDK path configuration. This unified approach simplifies the setup process for new developers and reduces configuration errors that can lead to build failures. The SDK management system also provides diagnostic capabilities that help identify missing or misconfigured SDKs, making it easier to troubleshoot environment setup issues.

Cross-platform support is built into IgniteBT from the ground up, with configurable build targets for Windows, Linux, and macOS. The system abstracts platform-specific differences such as compiler selection, library naming conventions, and file system paths, allowing developers to use a consistent set of commands across all supported platforms. This cross-platform capability is essential for projects that need to target multiple operating systems or that have team members working on different platforms. The platform abstraction layer handles the complexity of platform-specific build requirements while maintaining a unified build system interface.

Real-time diagnostics and comprehensive logging provide developers with immediate visibility into the build process. IgniteBT implements structured logging at multiple levels, from high-level progress information to detailed diagnostic output for troubleshooting. The logging system can output to both console and file, with configurable log levels and rolling log file management. In addition to logging, the system provides diagnostic commands such as the doctor command that performs comprehensive environment validation, dependency verification, and configuration checking. These diagnostic capabilities help developers quickly identify and resolve build issues, reducing the time spent troubleshooting build failures.

---

## 🏗️ Architecture

IgniteBT is organized into several core subsystems, each responsible for a specific aspect of the build process. This modular architecture allows for clear separation of concerns, making the system easier to understand, maintain, and extend. The subsystems communicate through well-defined interfaces, enabling independent development and testing of individual components. The architecture is designed to be both performant and maintainable, with careful attention paid to minimizing overhead in critical paths while keeping the codebase organized and comprehensible.

### 🧱 Core Infrastructure

The core infrastructure layer provides foundational services that are used throughout the IgniteBT system. This layer includes filesystem operations, hashing algorithms, threading primitives, and database management. The filesystem abstraction provides unified access to file operations across different platforms, handling platform-specific differences such as path separators, file permissions, and symbolic links. This abstraction ensures that the rest of the system can work with files in a platform-independent manner, reducing the complexity of cross-platform support. The hashing system provides fast cryptographic hashing using algorithms such as XXHash3, which is used for content-addressable storage, cache key generation, and integrity verification. The threading primitives implement a thread pool with work-stealing scheduling, providing efficient parallel execution of build tasks. The database layer uses SQLite for persistent storage of build metadata, dependency information, and cache state. This infrastructure is optimized for performance and reliability in enterprise environments, with careful attention paid to minimizing overhead in critical paths.

### ⚙️ Build Orchestration

The build orchestration layer is responsible for managing the compilation pipeline from source files to final binaries. This layer implements the core build logic, including compiler invocation, linker coordination, and dependency management. The compiler invocation system handles the complexity of calling different compilers with appropriate flags and parameters, abstracting platform-specific compiler differences behind a unified interface. The linker coordination system manages the linking process, including library resolution, symbol export generation, and incremental linking support. Precompiled header generation and utilization is managed at this layer, with automatic detection of PCH opportunities and intelligent invalidation when PCH-incompatible changes occur. Unity build optimization is implemented to reduce compilation overhead by combining multiple source files into fewer compilation units, reducing header parsing overhead and improving compilation speed. The toolchain abstraction provides a unified interface to different compilers and linkers across platforms, enabling cross-platform builds with minimal platform-specific code.

### 📁 Workspace Management

The workspace management system handles project structure, module discovery, and SDK configuration. This layer is responsible for understanding the organization of the project, discovering build modules, and ensuring that all required dependencies are properly configured. The module discovery system automatically scans the source tree to find build modules, parsing their configuration and building a comprehensive view of the project structure. SDK configuration management detects installed development SDKs, validates their versions against project requirements, and manages SDK path configuration. The workspace system maintains a comprehensive view of the development environment and ensures all required dependencies are properly configured before build initiation. This proactive validation helps prevent build failures due to missing or misconfigured dependencies, providing developers with clear error messages when configuration issues are detected.

### 💻 Command Interface

IgniteBT exposes a unified command-line interface through the `we` launcher, providing consistent access to all build operations across different development environments. The command interface layer is responsible for parsing command-line arguments, routing commands to appropriate handlers, and presenting results to the user. The command dispatcher implements a flexible command routing system that makes it easy to add new commands without modifying the core command parsing logic. Each command is implemented as a separate handler with well-defined inputs and outputs, promoting modularity and testability. The command interface also provides help generation, usage information, and error reporting, ensuring that users have the information they need to use the system effectively. The unified interface means that developers can use the same commands regardless of their platform or development environment, reducing cognitive overhead and simplifying the development workflow.

---

## 🚀 Quick Start

### 📦 Installation

> **💡 Note:** IgniteBT is integrated directly into WindEffects Engine projects, requiring no separate installation process. The build system is designed to be bootstrapped automatically on first use, meaning that developers can simply clone the repository and begin building without needing to manually install or configure the build system. This seamless integration is achieved through the bootstrap launcher, which is responsible for detecting whether IgniteBT has been built and, if not, automatically building it using the .NET SDK. The bootstrap process is transparent to the user and only runs on the first invocation, after which the compiled build system is cached for subsequent uses. This approach eliminates the traditional friction associated with setting up build tools and allows developers to focus on their work rather than tool configuration.

### 🎯 Basic Usage

The basic usage of IgniteBT revolves around the `we` command-line launcher, which provides a simple and intuitive interface for all build operations. The most common task is building the project, which can be accomplished with a single command specifying the desired configuration. The system supports three primary configurations: Debug for full debugging information with no optimizations, Development for optimized builds with debugging symbols suitable for daily development, and Shipping for fully optimized release builds. Developers can clean build artifacts to force a complete rebuild, which is useful when troubleshooting build issues or when major changes have been made that require a fresh start. The rebuild command combines cleaning and building into a single operation for convenience. For running the built executables, such as the editor or game, the run command launches the specified target with the appropriate configuration.

```powershell
# Build the project in Development configuration
we build --config Development

# Clean build artifacts
we clean --config Debug

# Rebuild with specified target
we rebuild --target WECore --config Development

# Run the editor after building
we run --target Editor --config Development
```

---

## 📚 Documentation Structure

The IgniteBT documentation is part of the WindEffects Engine documentation and covers the build system component:

| Document | Description | Link |
|----------|-------------|------|
| 📖 **Getting Started Guide** | Instructions for setting up the WindEffects Engine build environment and performing your first build | [GETTING-STARTED.md](./GETTING-STARTED.md) |
| 🏗️ **Architecture Documentation** | Design of the IgniteBT build system component within WindEffects Engine | [ARCHITECTURE.md](./ARCHITECTURE.md) |
| 💻 **Command Reference** | Documentation for the `we` CLI commands used in WindEffects Engine development | [COMMANDS.md](./COMMANDS.md) |
| ⚙️ **Configuration Guide** | SDK configuration and build settings for WindEffects Engine | [CONFIGURATION.md](./CONFIGURATION.md) |

These guides are designed for WindEffects Engine developers working with the build system. The Getting Started Guide helps new developers set up their environment. The Architecture Documentation explains how the build system works within the engine. The Command Reference documents the `we` commands used in daily development. The Configuration Guide covers SDK setup and build configuration for WindEffects Engine.

---

## ⚙️ System Requirements

| Requirement | Version/Details | Purpose |
|-------------|----------------|---------|
| 🖥️ **Operating System** | Windows 10/11 (Primary), Linux, macOS | Platform support |
| 🟦 **.NET SDK** | 8.0 | Build system execution |
| 🎨 **Visual Studio** | 2022 with C++ workload | MSVC compiler and toolchain |

> **⚠️ Important:** The .NET SDK should be downloaded from the official Microsoft website and added to the system PATH to ensure it can be invoked from any directory. Visual Studio 2022 installation should include the latest updates to ensure compatibility with the latest compiler features and fixes.

---

## 🆘 Support

For technical documentation, issue reporting, or support, refer to the main WindEffects Engine documentation. IgniteBT is a component of the WindEffects Engine, and support is provided through the same channels as the main engine.

**Troubleshooting Steps:**

1. 📖 Consult the WindEffects Engine documentation to ensure configuration is correct
2. 🔍 Run `we doctor` for comprehensive environment diagnostics
3. 🐛 Report issues through the official WindEffects Engine support channels with detailed information

Users experiencing issues with the build system should first consult the WindEffects Engine documentation to ensure that their configuration is correct and that they are using the system as intended. If issues persist, the doctor command can be used to perform comprehensive diagnostics of the build environment. For issues that cannot be resolved through documentation or diagnostics, users should report the problem through the official WindEffects Engine support channels.

---

## 📄 License

IgniteBT is part of the WindEffects Engine project and is subject to the WindEffects Engine End User License Agreement (EULA). Use of IgniteBT is governed by the same terms and conditions as the WindEffects Engine. For detailed information about terms and conditions, usage rights, and any restrictions that may apply, refer to the LICENSE file in the root of the repository or Legal/EULA.md. Organizations or individuals considering using IgniteBT or WindEffects Engine should review the license terms carefully and, if necessary, contact the development team for clarification or to discuss licensing arrangements.

---

## 🔗 Quick Links

- 📖 [Getting Started Guide](./GETTING-STARTED.md)
- 🏗️ [Architecture Documentation](./ARCHITECTURE.md)
- 💻 [Command Reference](./COMMANDS.md)
- ⚙️ [Configuration Guide](./CONFIGURATION.md)
- 📋 [Changelog](../../../../../CHANGELOG.MD)
