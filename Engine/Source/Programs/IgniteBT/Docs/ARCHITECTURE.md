# IgniteBT Architecture

IgniteBT is the build system component of WindEffects Engine, designed with a layered architecture separating concerns across infrastructure, build orchestration, workspace management, and user interface layers.

---

## System Overview

IgniteBT follows a modular architecture where each subsystem operates independently through well-defined interfaces. Built on .NET 8.0 with modern C# features (pattern matching, nullable reference types, async patterns).

### Architectural Layers

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

## Core Infrastructure

Foundational services optimized for performance and reliability.

### Filesystem Operations

Unified access to file operations across platforms.

| Feature | Description |
|---------|-------------|
| Atomic Operations | Complete or no partial writes |
| Path Normalization | Consistent path representations |
| Symbolic Link Handling | Works with symlinks |
| File Watching | Hot-reload support |

### Hashing and Integrity

Content-addressable storage using XXHash3 for build cache invalidation, dependency change detection, artifact deduplication, and integrity verification.

> **Key Benefit**: Cryptographic hashes of file contents determine rebuilds accurately, even when modification times change without content changes.

| Use Case | Implementation |
|----------|----------------|
| Cache Invalidation | Hash-based file change detection |
| Dependency Detection | Accurate include dependency tracking |
| Artifact Deduplication | Single storage for identical artifacts |
| Integrity Verification | Cache corruption detection |

### Threading Model

Thread pool with work-stealing scheduler, priority-based execution, and cancellation token propagation.

| Component | Function |
|-----------|----------|
| Thread Pool | Efficient management of limited threads |
| Work-Stealing Scheduler | Load balancing via idle thread work stealing |
| Priority Execution | Important tasks processed first |
| Cancellation Tokens | Clean cancellation of long operations |

### Database Layer

SQLite for build cache metadata, dependency graph persistence, module configuration, and build history/analytics.

| Data Type | Purpose |
|-----------|---------|
| Build Cache Metadata | Cached artifacts and validity conditions |
| Dependency Graph | Persistent module/file dependencies |
| Module Configuration | Build module configurations |
| Build History | Timing data, success rates, resource usage |

---

## Build Orchestration

Manages compilation pipeline from source to final artifacts.

### Dependency Graph

Analyzes source file dependencies, resolves inter-module dependencies, detects circular dependencies, and optimizes build order.

| Capability | Description |
|------------|-------------|
| Source Analysis | File dependencies through include statements |
| Transitive Resolution | Dependencies through entire chain |
| Circular Detection | Identifies impossible circular deps |
| Order Optimization | Maximizes parallel execution |

### Compiler Integration

| Platform | Compiler | Features |
|----------|----------|----------|
| Windows | MSVC | Full Visual Studio integration |
| Linux | Clang | GCC compatibility mode |
| macOS | Clang | Apple toolchain support |

Abstraction layer handles compiler command-line differences.

### Linker Coordination

| Feature | Benefit |
|---------|---------|
| Auto Resolution | Correct library ordering |
| Export Generation | Automatic DLL export definitions |
| Incremental Linking | Reuses previous link work |
| Debug Symbols | Automatic symbol file generation |

### Build Scheduler

| Optimization | Impact |
|--------------|--------|
| Topological Sorting | Correct build order |
| Parallel Execution | Configurable concurrency |
| Critical Path | Prioritizes bottleneck tasks |
| Resource Awareness | Avoids CPU/memory/I/O contention |

---

## Workspace Management

Manages project structure and development environment configuration.

### Module System

| Capability | Description |
|------------|-------------|
| Auto Discovery | Scans source tree for modules |
| Rule Validation | Syntactically and semantically valid |
| Dependency Resolution | Build order from module dependencies |
| Layout Configuration | Output placement by configuration/platform |

### SDK Detection

Detects installed development SDKs, validates versions, manages paths, provides availability diagnostics. Platform-aware detection handles OS-specific installation locations.

### Configuration Management

| Source | Priority |
|--------|----------|
| Command Line | Highest (overrides all) |
| Config Files | Medium (overrides defaults) |
| Defaults | Lowest |
| Environment Variables | Integrated with config system |

---

## Command Interface

User-facing CLI through the `we` launcher.

### Command Dispatcher

| Function | Description |
|----------|-------------|
| Argument Parsing | Flags, options, positional arguments |
| Command Routing | Flexible routing to handlers |
| Parameter Validation | Type, range, logical validation |
| Help Generation | Auto-generated from metadata |

### Command Categories

| Category | Commands | Purpose |
|----------|----------|---------|
| Build | `build`, `clean`, `rebuild`, `package` | Create build artifacts |
| Execution | `run`, `daemon` | Execute programs, manage services |
| Project | `project`, `plugin`, `modules` | Project-level operations |
| SDK | `sdk`, `setup` | SDK detection and setup |
| Diagnostic | `doctor`, `version`, `graph`, `benchmark` | Diagnostics and analysis |

---

## Build Layout System

### Directory Structure

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
| Output | Binaries, libraries | Final build products |
| Intermediate | Object files | Incremental build support |
| Generated | Auto-generated code | Code generation artifacts |
| Cache | Build cache | Incremental acceleration |
| Database | Metadata, dependencies | Persistent build state |
| Logs | Build logs | Troubleshooting |
| Manifest | Build descriptors | Build documentation |

### Output Resolution

| Resolution Type | Description |
|-----------------|-------------|
| Target-Specific | Separate directories per build target |
| Configuration-Specific | Separates Debug, Development, Shipping |
| Platform-Specific | Handles platform binary format differences |
| Debug Symbol Storage | Separate storage for large debug symbols |

---

## Distributed Build System

Optional distributed compilation across multiple machines.

### Distribution Architecture

| Component | Role |
|-----------|------|
| Master Node | Coordinates build, distributes tasks, aggregates results |
| Worker Nodes | Execute tasks, return results |
| Job Distribution | Load balancing and network optimization |
| Network-Transparent Access | Local caching + network file access |
| Fault Tolerance | Automatic retry and worker failure handling |

### Caching Strategy

| Feature | Benefit |
|---------|---------|
| Artifact Sharing | Build once, reuse across workers |
| Hit Optimization | Fast cache availability checking |
| Invalidation Propagation | Prevents stale artifact usage |
| Storage Management | Eviction policies and garbage collection |

---

## Performance Optimizations

### Incremental Building

| Technique | Impact |
|-----------|--------|
| Dependency-Based Invalidation | Only rebuild affected files |
| Minimal Recompilation | Skip unnecessary rebuilds (comments, whitespace) |
| Header Change Detection | Precise header impact tracking |
| PCH Utilization | Dramatically reduce header parsing |

### Parallel Execution

| Method | Description |
|--------|-------------|
| Multi-Threading | Simultaneous compilation on multiple cores |
| Target Parallelization | Build independent targets simultaneously |
| I/O Pipelining | Overlap I/O with computation |
| Workload Distribution | Balance load across CPU cores |

### Caching

| Cache Type | Contents | Benefit |
|------------|----------|---------|
| Object Files | Compiled object files | Basic incremental speedup |
| Build Results | Linked libraries, products | Higher-level result reuse |
| Dependency Graph | Computed dependencies | Avoid expensive recomputation |
| SDK Detection | SDK scan results | Faster build startup |

---

## Error Handling and Diagnostics

### Error Reporting

| Feature | Description |
|---------|-------------|
| Structured Messages | Consistent formatting for parsing |
| Source Location | File, line, column precision |
| Suggested Resolutions | Actionable fix suggestions |
| Error Documentation | Reference for error codes |

### Diagnostics

The `doctor` command provides:
- Environment validation (tools, SDKs)
- Dependency verification (conflicts, missing, circular)
- Configuration checking (config files, settings)
- Performance profiling (bottlenecks, optimization opportunities)

### Logging

| Output | Purpose |
|--------|---------|
| Console | Real-time progress and error feedback |
| File | Historical analysis and investigation |
| Level Filtering | Control verbosity (critical to debug) |
| Structured Format | Machine-parseable for automation |

---

## Extension Points

### Custom Commands

Register through command dispatcher for project-specific operations.

| Use Case | Example |
|----------|---------|
| Code Generation | Project-specific generation workflows |
| Testing Automation | Automated test execution and reporting |
| Deployment | Automated deployment and release |
| Validation | Project-specific validation and checks |

### Build Modules

Add new modules by implementing the module interface and placing module rules in source tree.

| Capability | Description |
|------------|-------------|
| Auto Discovery | Automatic registration of custom modules |
| Custom Types | Specialized build logic |
| Rule Definition | Source files, dependencies, flags, output type |
| Integration | Seamless integration with existing system |

### SDK Providers

Custom SDK detection through provider interface.

| Feature | Benefit |
|---------|---------|
| Custom Detection | Support for specialized SDKs |
| Version Validation | Custom version requirement checking |
| Path Configuration | Non-standard installation support |
| Auto Registration | Seamless integration with SDK system |

---

## Security Considerations

| Security Measure | Protection |
|------------------|------------|
| Path Validation | Prevents directory traversal |
| Injection Prevention | Sanitizes user input for commands |
| Secure File Operations | Proper permissions and access controls |
| Credential Handling | Encryption and secure protocols |

---

## Future Enhancements

| Enhancement | Description |
|-------------|-------------|
| Distributed Coordination | Better load balancing, fault tolerance, network optimization |
| ML-Based Prediction | Historical data analysis for build time prediction |
| Advanced Caching | Predictive caching, intelligent eviction policies |
| Cloud Integration | On-demand scaling with cloud build resources |
| Real-Time Collaboration | Shared build state and collaborative debugging |

---

## Related Documentation

- [Getting Started Guide](./GETTING-STARTED.md)
- [Command Reference](./COMMANDS.md)
- [Configuration Guide](./CONFIGURATION.md)
- [Changelog](../../../../../CHANGELOG.md)