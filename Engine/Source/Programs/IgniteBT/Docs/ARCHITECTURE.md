# 🏗️ IgniteBT Architecture

> IgniteBT is the build system component of WindEffects Engine, designed with a layered architecture that separates concerns across infrastructure, build orchestration, workspace management, and user interface layers. This document describes the system architecture, component interactions, and design principles in detail. The architecture has been carefully designed to support the complex requirements of WindEffects Engine while maintaining performance, reliability, and maintainability. Each layer has a specific responsibility and communicates with other layers through well-defined interfaces, promoting modularity and making the system easier to understand, test, and extend.

---

## 🌐 System Overview

IgniteBT follows a modular architecture where each subsystem operates independently through well-defined interfaces. The system is built on .NET 8.0 and leverages modern C# features including pattern matching, nullable reference types, and asynchronous programming patterns. This choice of technology provides a robust foundation for the WindEffects Engine build system, allowing developers to write clean, maintainable code while taking advantage of the latest language features for improved productivity and code safety. The modular architecture means that different components can be developed, tested, and maintained independently, reducing the risk of changes in one area inadvertently affecting other areas of the system.

### 📐 Architectural Layers

The IgniteBT architecture for WindEffects Engine is organized into four distinct layers, each building upon the layer below it:

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
| 💻 **Command Interface** | User-facing CLI and interaction | Command dispatcher, argument parser, help system |
| ⚙️ **Build Orchestration** | Compilation pipeline management | Scheduler, compiler, linker, dependency graph |
| 📁 **Workspace Management** | Project structure and environment | Module discovery, SDK detection, configuration |
| 🧱 **Core Infrastructure** | Foundational services | Filesystem, hashing, threading, database, logging |

---

## 🧱 Core Infrastructure

The infrastructure layer provides foundational services used throughout the system. These components are optimized for performance and reliability, forming the foundation upon which all other layers are built. The infrastructure layer is designed to be as efficient as possible, as its operations are called frequently throughout the build process. Performance-critical operations such as file hashing, threading, and database access have been carefully optimized to minimize overhead and maximize throughput. The components in this layer are also designed to be platform-agnostic where possible, providing abstractions that hide platform-specific differences from the layers above.

### 📁 Filesystem Operations

The filesystem abstraction provides unified access to file operations across different platforms, handling the complexities of different operating systems and file systems through a consistent interface. This abstraction is essential for cross-platform support, as it allows the rest of the system to work with files without worrying about platform-specific details such as path separators, file permissions, or case sensitivity.

| Feature | Description |
|---------|-------------|
| 🔒 **Atomic Operations** | Ensures file operations either complete fully or not at all, preventing partial writes |
| 🔄 **Path Normalization** | Treats different path representations consistently, preventing issues with relative paths |
| 🔗 **Symbolic Link Handling** | Works correctly with projects using symbolic links for organization |
| 👁️ **File Watching** | Supports hot-reload scenarios by detecting file changes automatically |

### 🔐 Hashing and Integrity

Content-addressable storage using XXHash3 provides fast file hashing for build cache invalidation, dependency change detection, artifact deduplication, and integrity verification. The choice of XXHash3 is deliberate, as it provides an excellent balance between speed and collision resistance, making it ideal for build system applications where many files need to be hashed quickly.

> **💡 Key Benefit:** By computing cryptographic hashes of file contents rather than relying on modification times, the build system can accurately determine which files need to be rebuilt even in complex scenarios involving file copies, restores, or other operations that might affect modification times without changing content.

| Use Case | Implementation |
|----------|----------------|
| 🔄 **Cache Invalidation** | Hash-based detection of file changes |
| 🔗 **Dependency Detection** | Accurate tracking of include dependencies |
| 💾 **Artifact Deduplication** | Single storage for identical artifacts |
| ✅ **Integrity Verification** | Cache corruption detection |

### 🧵 Threading Model

The threading system provides thread pool management for parallel operations, work-stealing scheduler for load balancing, priority-based task execution, and cancellation token propagation.

| Component | Function |
|-----------|----------|
| 🏊 **Thread Pool** | Efficient management of limited threads for many concurrent tasks |
| ⚖️ **Work-Stealing Scheduler** | Load balancing by allowing idle threads to steal work |
| 🎯 **Priority Execution** | Important tasks processed before less critical ones |
| ❌ **Cancellation Tokens** | Clean cancellation of long-running operations |

### 🗄️ Database Layer

The SQLite database manages build cache metadata, dependency graph persistence, module configuration storage, and build history and analytics. SQLite was chosen for its reliability, portability, and excellent performance for the read-heavy workloads typical of build systems.

| Data Type | Purpose |
|-----------|---------|
| 📊 **Build Cache Metadata** | Tracks cached artifacts and validity conditions |
| 🔗 **Dependency Graph** | Persistent storage of module/file dependencies |
| ⚙️ **Module Configuration** | Maintains build module configurations |
| 📈 **Build History** | Stores timing data, success rates, resource usage |

---

## ⚙️ Build Orchestration

The build orchestration layer manages the compilation pipeline from source to final artifacts, implementing the core logic that transforms source code into executable binaries and libraries. This layer is responsible for coordinating the various steps involved in building a C++ project, including parsing source files, invoking compilers, managing dependencies, and linking the final outputs.

### 🔗 Dependency Graph

The dependency graph system analyzes source file dependencies, resolves inter-module dependencies, detects circular dependencies, and optimizes build order for parallel execution.

| Capability | Description |
|------------|-------------|
| 🔍 **Source Analysis** | Determines file dependencies through include statements |
| 🌐 **Transitive Resolution** | Tracks dependencies through entire dependency chain |
| 🔄 **Circular Detection** | Identifies impossible circular dependency situations |
| ⚡ **Order Optimization** | Maximizes parallel execution through dependency analysis |

### 🛠️ Compiler Integration

The compiler abstraction layer supports MSVC (Microsoft Visual C++) on Windows, Clang on Linux and macOS, compiler flag management and validation, precompiled header (PCH) generation, and unity build configuration.

| Platform | Compiler | Features |
|----------|----------|----------|
| 🪟 **Windows** | MSVC | Full Visual Studio integration |
| 🐧 **Linux** | Clang | GCC compatibility mode |
| 🍎 **macOS** | Clang | Apple toolchain support |

> **⚠️ Note:** The abstraction layer handles differences in compiler command-line syntax, flag names, and output formats, allowing the rest of the system to work with a unified compiler model.

### 🔗 Linker Coordination

The linker system handles library resolution and ordering, symbol export generation, incremental linking support, and debug symbol generation.

| Feature | Benefit |
|---------|---------|
| 📚 **Auto Resolution** | Correct library ordering based on dependencies |
| 📤 **Export Generation** | Automatic DLL export definition files |
| ⚡ **Incremental Linking** | Reuses work from previous links for speed |
| 🐛 **Debug Symbols** | Automatic symbol file generation and management |

### 📅 Build Scheduler

The scheduler provides topological sorting of build targets, parallel job execution with configurable concurrency, critical path optimization, and resource-aware scheduling.

| Optimization | Impact |
|--------------|--------|
| 🔀 **Topological Sorting** | Correct build order respecting dependencies |
| ⚡ **Parallel Execution** | Configurable concurrency for throughput |
| 🎯 **Critical Path** | Prioritizes bottleneck tasks to reduce total time |
| 💻 **Resource Awareness** | Avoids contention for CPU, memory, I/O |

---

## 📁 Workspace Management

The workspace layer manages project structure and development environment configuration, providing the context in which builds are executed. This layer is responsible for understanding the organization of the project, discovering the modules that need to be built, and ensuring that the development environment is properly configured with all required SDKs and dependencies.

### 📦 Module System

The module system discovers and configures build modules through automatic module discovery from source tree, module rule parsing and validation, inter-module dependency resolution, and module output layout configuration.

| Capability | Description |
|------------|-------------|
| 🔍 **Auto Discovery** | Scans source tree for modules without manual registration |
| ✅ **Rule Validation** | Ensures module definitions are syntactically and semantically valid |
| 🔗 **Dependency Resolution** | Determines build order based on module dependencies |
| 📂 **Layout Configuration** | Determines output placement based on configuration/platform |

### 🛠️ SDK Detection

The SDK management system detects installed development SDKs, validates SDK versions against requirements, manages SDK path configuration, and provides SDK availability diagnostics.

> **💡 Tip:** SDK detection is platform-aware, looking in appropriate locations for each operating system and handling platform-specific differences in SDK installation.

### ⚙️ Configuration Management

Configuration handling includes project descriptor parsing (`.engine` files), SDK configuration (`.SDKs.json`), build parameter resolution, and environment variable integration.

| Source | Priority |
|--------|----------|
| 💻 **Command Line** | Highest (overrides all) |
| 📄 **Config Files** | Medium (overrides defaults) |
| 🔧 **Defaults** | Lowest (used if not specified) |
| 🌍 **Environment Variables** | Integrated with config system |

---

## 💻 Command Interface

The command interface layer provides the user-facing CLI through the `we` launcher, serving as the primary point of interaction between developers and the build system. This layer is responsible for presenting the functionality of IgniteBT in an intuitive and discoverable way.

### 🎯 Command Dispatcher

The command system parses command-line arguments, routes commands to appropriate handlers, validates command parameters, and provides usage information and help.

| Function | Description |
|----------|-------------|
| 📝 **Argument Parsing** | Handles flags, options, positional arguments |
| 🔀 **Command Routing** | Flexible routing to command handlers |
| ✅ **Parameter Validation** | Type checking, range checking, logical validation |
| ❓ **Help Generation** | Auto-generated from command metadata |

### 📂 Command Categories

Commands are organized by functionality to make the system easier to understand and navigate.

| Category | Commands | Purpose |
|----------|----------|---------|
| 🔨 **Build** | `build`, `clean`, `rebuild`, `package` | Create build artifacts |
| ▶️ **Execution** | `run`, `daemon` | Execute programs, manage services |
| 📁 **Project** | `project`, `plugin`, `modules` | Project-level operations |
| 🛠️ **SDK** | `sdk`, `setup` | SDK detection and setup |
| 🔍 **Diagnostic** | `doctor`, `version`, `graph`, `benchmark` | Diagnostics and analysis |

---

## 📂 Build Layout System

The build layout system defines how artifacts are organized in the build directory, providing a structured and predictable organization for all build outputs.

### 📁 Directory Structure

```
Build/
├── Output/           # Final binaries and libraries
├── Intermediate/     # Object files and compilation artifacts
├── Generated/        # Generated source files
├── Cache/           # Build cache for incremental builds
├── Database/        # Build metadata and dependency info
├── Logs/            # Build and runtime logs
└── Manifest/        # Build manifests and descriptors
```

| Directory | Contents | Purpose |
|-----------|----------|---------|
| 📦 **Output** | Binaries, libraries | Final build products |
| 🔨 **Intermediate** | Object files | Incremental build support |
| 🤖 **Generated** | Auto-generated code | Code generation artifacts |
| 💾 **Cache** | Build cache | Incremental build acceleration |
| 🗄️ **Database** | Metadata, dependencies | Persistent build state |
| 📋 **Logs** | Build logs | Troubleshooting and analysis |
| 📄 **Manifest** | Build descriptors | Build process documentation |

### 🎯 Output Resolution

The output resolver determines target-specific output directories, configuration-specific artifact placement, platform-specific binary organization, and debug symbol storage.

| Resolution Type | Description |
|-----------------|-------------|
| 🎯 **Target-Specific** | Separate directories per build target |
| ⚙️ **Configuration-Specific** | Separates Debug, Development, Shipping |
| 🖥️ **Platform-Specific** | Handles platform binary format differences |
| 🐛 **Debug Symbol Storage** | Separate storage for large debug symbols |

---

## 🌐 Distributed Build System

IgniteBT supports optional distributed compilation across multiple machines, allowing organizations to leverage the combined computational power of multiple build servers to reduce build times for large projects.

### 🏗️ Distribution Architecture

The distributed build system implements a master-worker coordination model with job distribution and result aggregation, network-transparent file access, and fault tolerance and retry logic.

| Component | Role |
|-----------|------|
| 👑 **Master Node** | Coordinates build, distributes tasks, aggregates results |
| 👷 **Worker Nodes** | Execute tasks, return results to master |
| 🔄 **Job Distribution** | Load balancing and network optimization |
| 🔗 **Network-Transparent Access** | Local caching + network file access |
| 🛡️ **Fault Tolerance** | Automatic retry and worker failure handling |

### 💾 Caching Strategy

The distributed cache provides artifact sharing across build machines, cache hit optimization, cache invalidation propagation, and storage management and cleanup.

| Feature | Benefit |
|---------|---------|
| 🔄 **Artifact Sharing** | Build once, reuse across all workers |
| ⚡ **Hit Optimization** | Fast cache availability checking |
| 🚫 **Invalidation Propagation** | Prevents stale artifact usage |
| 🧹 **Storage Management** | Eviction policies and garbage collection |

---

## ⚡ Performance Optimizations

IgniteBT incorporates several performance optimizations designed to minimize build times and maximize developer productivity.

### 🔄 Incremental Building

Incremental building is implemented through dependency-based invalidation, minimal recompilation, header change detection, and precompiled header utilization.

| Technique | Impact |
|-----------|--------|
| 🔗 **Dependency-Based Invalidation** | Only rebuild affected files |
| 🎯 **Minimal Recompilation** | Skip unnecessary rebuilds (comments, whitespace) |
| 📄 **Header Change Detection** | Precise tracking of header impact |
| 🚀 **PCH Utilization** | Dramatically reduce header parsing time |

### 🚀 Parallel Execution

Parallel execution is achieved through multi-threaded compilation, independent target parallelization, I/O-bound operation pipelining, and CPU-bound workload distribution.

| Method | Description |
|-----------|-------------|
| 🧵 **Multi-Threading** | Simultaneous compilation on multiple cores |
| 🎯 **Target Parallelization** | Build independent targets simultaneously |
| 📊 **I/O Pipelining** | Overlap I/O with computation |
| ⚖️ **Workload Distribution** | Balance load across CPU cores |

### 💾 Caching

Caching is implemented through object file caching, build result memoization, dependency graph caching, and SDK detection caching.

| Cache Type | Contents | Benefit |
|------------|----------|---------|
| 📦 **Object Files** | Compiled object files | Basic incremental build speedup |
| 🎯 **Build Results** | Linked libraries, products | Higher-level result reuse |
| 🔗 **Dependency Graph** | Computed dependencies | Avoid expensive recomputation |
| 🛠️ **SDK Detection** | SDK scan results | Faster build startup |

---

## 🚨 Error Handling and Diagnostics

The system provides comprehensive error handling designed to help developers quickly identify and resolve issues.

### 📢 Error Reporting

Error reporting includes structured error messages with context, source location information, suggested resolutions, and error code documentation.

| Feature | Description |
|---------|-------------|
| 📝 **Structured Messages** | Consistent formatting for parsing |
| 📍 **Source Location** | File, line, column precision |
| 💡 **Suggested Resolutions** | Actionable fix suggestions |
| 📚 **Error Documentation** | Reference information for error codes |

### 🔍 Diagnostics

The `doctor` command provides environment validation, dependency verification, configuration checking, and performance profiling.

| Check | Purpose |
|-------|---------|
| ✅ **Environment Validation** | Verify tools and SDKs are installed correctly |
| 🔗 **Dependency Verification** | Detect conflicts, missing dependencies, circular deps |
| ⚙️ **Configuration Checking** | Validate config files and settings |
| 📈 **Performance Profiling** | Identify bottlenecks and optimization opportunities |

### 📋 Logging

Structured logging with multiple outputs includes console output for real-time feedback, file logging for historical analysis, log level filtering, and structured log format for parsing.

| Output | Purpose |
|--------|---------|
| 💻 **Console** | Real-time progress and error feedback |
| 📄 **File** | Historical analysis and investigation |
| 🔍 **Level Filtering** | Control verbosity (critical to debug) |
| 🤖 **Structured Format** | Machine-parseable for automation |

---

## 🔧 Extension Points

IgniteBT provides several extension mechanisms designed to allow the build system to be customized and extended to meet specific project requirements.

### 🎯 Custom Commands

Developers can register custom commands through the command dispatcher, allowing them to add project-specific build operations or automate project-specific workflows.

| Use Case | Example |
|----------|---------|
| 🤖 **Code Generation** | Project-specific code generation workflows |
| 🧪 **Testing Automation** | Automated test execution and reporting |
| 🚀 **Deployment** | Automated deployment and release processes |
| ✅ **Validation** | Project-specific validation and checks |

### 📦 Build Modules

New build modules can be added by implementing the module interface and placing module rules in the source tree.

| Capability | Description |
|------------|-------------|
| 🔍 **Auto Discovery** | Automatic registration of custom modules |
| ⚙️ **Custom Types** | Specialized build logic beyond standard types |
| 📝 **Rule Definition** | Source files, dependencies, flags, output type |
| 🔗 **Integration** | Seamless integration with existing build system |

### 🛠️ SDK Providers

Custom SDK detection logic can be added through the SDK provider interface, allowing projects to add support for SDKs that are not natively supported by IgniteBT.

| Feature | Benefit |
|---------|---------|
| 🔍 **Custom Detection** | Support for specialized SDKs |
| 📏 **Version Validation** | Custom version requirement checking |
| 📍 **Path Configuration** | Non-standard installation support |
| 🤝 **Auto Registration** | Seamless integration with SDK system |

---

## 🔒 Security Considerations

The build system incorporates security best practices including path validation and sanitization, command injection prevention, secure file operations, and credential handling for distributed builds.

| Security Measure | Protection |
|------------------|------------|
| 🛡️ **Path Validation** | Prevents directory traversal attacks |
| 💉 **Injection Prevention** | Sanitizes user input for command execution |
| 🔒 **Secure File Operations** | Proper permissions and access controls |
| 🔐 **Credential Handling** | Encryption and secure protocols for credentials |

---

## 🚀 Future Architecture Enhancements

Planned architectural improvements include enhanced distributed build coordination, machine learning-based build time prediction, advanced caching strategies, cloud-based build farm integration, and real-time collaboration features.

| Enhancement | Description |
|-------------|-------------|
| 🌐 **Distributed Coordination** | Better load balancing, fault tolerance, network optimization |
| 🤖 **ML-Based Prediction** | Historical data analysis for build time prediction |
| 💾 **Advanced Caching** | Predictive caching, intelligent eviction policies |
| ☁️ **Cloud Integration** | On-demand scaling with cloud build resources |
| 👥 **Real-Time Collaboration** | Shared build state and collaborative debugging |

---

## 🔗 Related Documentation

- 📖 [Getting Started Guide](./GETTING-STARTED.md)
- 💻 [Command Reference](./COMMANDS.md)
- ⚙️ [Configuration Guide](./CONFIGURATION.md)
- 📋 [Changelog](../../../../../CHANGELOG.md)
