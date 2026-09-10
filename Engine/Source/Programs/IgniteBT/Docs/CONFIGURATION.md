# IgniteBT Configuration Guide

This guide covers configuring IgniteBT for WindEffects Engine development, including SDK setup, project configuration, and build system customization.

---

## Configuration Files

| File | Purpose | Location | Version Control |
|------|---------|----------|-----------------|
| `.engine` | Project descriptor and structure settings | Repository root | Yes |
| `IgniteBT.SDKs.json` | SDK path configuration | Repository root | Partial (exclude local paths) |
| `bootstrap.manifest` | Build system bootstrap configuration | Auto-generated | No |

---

## Project Configuration

### .engine File

Primary project descriptor located in repository root.

**Example:**
```ini
# WindEffects engine installation descriptor
schema=1
engine.version=0.1.0
ProgramsRoot=Engine/Source/Programs
BuildRoot=Build
AssetsRoot=Assets
ProjectsRoot=Projects
bootstrap.manifest=Build/Manifest/bootstrap.manifest
bootstrap.entry=IgniteBT
bootstrap.entry.source=IgniteBT/Source/IgniteBT.csproj
```

| Parameter | Description | Example |
|-----------|-------------|---------|
| `schema` | Descriptor file format version | `1` |
| `engine.version` | Engine version identifier | `0.1.0` |
| `ProgramsRoot` | Path to build tools and programs | `Engine/Source/Programs` |
| `BuildRoot` | Build output directory | `Build` |
| `AssetsRoot` | Project assets directory | `Assets` |
| `ProjectsRoot` | Project files directory | `Projects` |
| `bootstrap.manifest` | Bootstrap manifest file path | `Build/Manifest/bootstrap.manifest` |
| `bootstrap.entry` | Bootstrap entry point | `IgniteBT` |
| `bootstrap.entry.source` | Bootstrap source project | `IgniteBT/Source/IgniteBT.csproj` |

> **Note**: The `.engine` file is typically committed to version control. Adjust values for local development as needed.

---

## SDK Configuration

### IgniteBT.SDKs.json

Defines paths to required development SDKs.

**Example:**
```json
{
  "SDKPaths": {},
  "AdditionalIncludePaths": {
    "MyCustomLibrary": "C:\\Libraries\\MyCustomLibrary\\include"
  },
  "AdditionalLibraryPaths": {
    "MyCustomLibrary": "C:\\Libraries\\MyCustomLibrary\\lib"
  }
}
```

### Additional Include and Library Paths

For custom libraries not detected automatically:

```json
{
  "AdditionalIncludePaths": {
    "OpenAL": "C:\\OpenAL\\include",
    "CustomLib": "C:\\Libraries\\CustomLib\\include"
  },
  "AdditionalLibraryPaths": {
    "OpenAL": "C:\\OpenAL\\lib",
    "CustomLib": "C:\\Libraries\\CustomLib\\lib"
  }
}
```

### SDK Detection Commands

| Command | Purpose | When to Use |
|---------|---------|-------------|
| `we sdk detect` | Force rescan for SDKs | After installing/updating SDKs |
| `we sdk validate` | Validate SDK configuration | To verify configuration |
| `we sdk list` | List all detected SDKs | To see available SDKs |

---

## Build Configuration

### Build Configurations

| Configuration | Optimizations | Debug Symbols | Performance | Binary Size | Use Case |
|---------------|---------------|---------------|-------------|-------------|----------|
| Debug | None | Maximum | Slowest | Largest | Complex debugging |
| Development | Enabled | Included | Good | Reasonable | Daily development |
| Shipping | Maximum | None | Best | Smallest | Release builds |

### Platform Configuration

| Platform | Description | Default |
|----------|-------------|---------|
| Win64 | Windows 64-bit | Yes |
| Windows | Windows platform-agnostic | No |
| Linux | Linux 64-bit | No |
| Mac | macOS 64-bit | No |

```powershell
we build --config Development --platform Win64
```

### Compiler Configuration

| Platform | Compiler | Notes |
|----------|----------|-------|
| Windows | MSVC | Full Visual Studio integration |
| Linux | Clang | GCC compatibility mode |
| macOS | Clang | Apple toolchain support |

The build system handles compiler command-line differences automatically.

---

## Module Configuration

### Module Rules

Build modules are configured through module rule files defining:
- Source file inclusion patterns
- Compiler flags and defines
- Public and private dependencies
- Precompiled header configuration
- Unity build settings

### Module Dependencies

| Dependency Type | Description | Transitive |
|-----------------|-------------|------------|
| Public | Exposed to consumers | Yes |
| Private | Internal to module | No |

**Example:**
```csharp
PublicDependencies = new[] { "WECore", "WERenderer" };
PrivateDependencies = new[] { "ThirdParty/SomeLibrary" };
```

### Module Output Configuration

| Output Type | Description | Platform Examples |
|-------------|-------------|-------------------|
| Executable | Standalone applications | Editor.exe, Game.exe |
| Shared Library | Runtime-loaded libraries | .dll (Windows), .so (Linux) |
| Static Library | Link-time libraries | .lib (Windows), .a (Linux/macOS) |

---

## Environment Configuration

### Environment Variables

#### Build Configuration Variables

| Variable | Purpose | Example |
|----------|---------|---------|
| `IGNITEBT_CONFIG` | Default build configuration | `Development` |
| `IGNITEBT_PLATFORM` | Default target platform | `Win64` |
| `IGNITEBT_JOBS` | Default parallel job count | `8` |

```powershell
set IGNITEBT_CONFIG=Development
set IGNITEBT_PLATFORM=Win64
set IGNITEBT_JOBS=8
```

#### Output Configuration Variables

| Variable | Purpose | Example |
|----------|---------|---------|
| `IGNITEBT_BUILD_ROOT` | Custom build directory path | `C:\Builds\WindEffects` |
| `IGNITEBT_VERBOSE` | Enable verbose output globally | `1` |

```powershell
set IGNITEBT_BUILD_ROOT=C:\Builds\WindEffects
set IGNITEBT_VERBOSE=1
```

#### Persistent Configuration

| Method | Scope | When to Use |
|--------|-------|-------------|
| System Properties | System-wide | All applications and users |
| PowerShell Profile | User session | PowerShell-specific |
| .env file | Project-specific | Version-controlled project settings |

---

## Build Layout Configuration

### Directory Structure

```
Build/
├── Output/              # Final binaries
├── Intermediate/        # Object files
├── Generated/           # Generated source
├── Cache/              # Build cache
├── Database/           # Metadata
├── Logs/               # Build logs
└── Manifest/           # Build manifests
```

| Directory | Contents | Purpose |
|-----------|----------|---------|
| Output | Binaries, libraries | Primary build products |
| Intermediate | Object files, PDBs | Incremental build support |
| Generated | Auto-generated code | Code generation artifacts |
| Cache | Build cache | Incremental acceleration |
| Database | Metadata, dependencies | Persistent build state |
| Logs | Build and runtime logs | Troubleshooting |
| Manifest | Build descriptors | Build process documentation |

### Custom Build Root

Via environment variable:
```powershell
set IGNITEBT_BUILD_ROOT=D:\Builds\WindEffects
```

Via `.engine` file:
```ini
BuildRoot=D:\Builds\WindEffects
```

---

## Distributed Build Configuration

### Worker Configuration

```powershell
we daemon --workers 4 --port 8080
```

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--workers` | Number of worker processes | Auto-detected |
| `--port` | Port for daemon | Default port |

### Cache Configuration

```json
{
  "DistributedCache": {
    "Enabled": true,
    "ServerAddress": "build-cache.example.com",
    "Port": 9090,
    "Authentication": "token"
  }
}
```

| Setting | Description |
|---------|-------------|
| `Enabled` | Whether distributed cache is active |
| `ServerAddress` | Hostname or IP of cache server |
| `Port` | Port number for cache server |
| `Authentication` | Authentication method and credentials |

---

## Performance Tuning

### Parallel Job Configuration

```powershell
# CPU-bound builds
we build --jobs 8

# I/O-bound builds
we build --jobs 4

# Use all available cores
we build --jobs 0
```

Auto-detection (job count 0) determines optimal count based on hardware.

### Cache Configuration

```json
{
  "Cache": {
    "Enabled": true,
    "MaxSizeGB": 50,
    "CompressionEnabled": true
  }
}
```

| Setting | Description | Impact |
|---------|-------------|--------|
| `Enabled` | Whether build cache is active | Disabling slows incremental builds |
| `MaxSizeGB` | Maximum cache size in GB | Prevents excessive disk usage |
| `CompressionEnabled` | Compress cache entries | Reduces size, adds CPU overhead |

### Precompiled Headers

```csharp
PCHEnabled = true;
PCHHeader = "PCH.h";
PCHSource = "PCH.cpp";
```

| Setting | Description |
|---------|-------------|
| `PCHEnabled` | Enable precompiled headers for module |
| `PCHHeader` | Header file to precompile |
| `PCHSource` | Source file for PCH generation |

### Unity Builds

```csharp
UnityBuildEnabled = true;
UnityBuildBatchSize = 16;
```

| Setting | Description |
|---------|-------------|
| `UnityBuildEnabled` | Enable unity builds for module |
| `UnityBuildBatchSize` | Files per unity build batch |

---

## Diagnostic Configuration

### Logging

```powershell
# Enable verbose logging
we build --verbose

# Set log level via environment
set IGNITEBT_LOG_LEVEL=Verbose
```

Verbose logging shows: exact compiler commands, file-by-file progress, dependency resolution, linker output.

### Build Reports

```powershell
we build --report
```

Reports saved to `Build/Logs/` with:
- Total build time
- Time per build phase
- Individual file compilation times
- Linking times
- Cache hit rates

---

## Validation and Testing

### Configuration Validation

| Command | Purpose | What It Checks |
|---------|---------|----------------|
| `we doctor` | Full environment diagnostics | SDKs, tools, permissions, dependencies |
| `we sdk validate` | Validate SDK configuration | SDK availability and versions |
| `we project validate` | Validate project configuration | Project structure and consistency |

### Test Builds

```powershell
we clean
we build --config Development --verbose
```

Best practice: run a clean build after configuration changes.

---

## Common Configuration Scenarios

### Development Workstation

```powershell
set IGNITEBT_CONFIG=Development
set IGNITEBT_JOBS=8
set IGNITEBT_VERBOSE=0
```

- Fast incremental builds
- Debugging capability when needed
- Good balance of optimization and debug info

### CI/CD Pipeline

```powershell
set IGNITEBT_CONFIG=Shipping
set IGNITEBT_JOBS=0
set IGNITEBT_VERBOSE=1
```

- Consistent, reproducible builds
- Detailed logs for troubleshooting
- Release-quality binaries

### Release Build

```powershell
set IGNITEBT_CONFIG=Shipping
set IGNITEBT_JOBS=16
set IGNITEBT_BUILD_ROOT=D:\Releases\WindEffects
```

- Maximum optimizations
- Smallest binary size
- Separate release artifacts

---

## Troubleshooting Configuration

### SDK Not Found

| Step | Action |
|------|--------|
| 1 | Verify SDK installation and version |
| 2 | Check `IgniteBT.SDKs.json` paths |
| 3 | Run `we sdk detect` |
| 4 | Run `we sdk validate` |

### Build Path Issues

| Step | Action |
|------|--------|
| 1 | Check `BuildRoot` in `.engine` |
| 2 | Verify `IGNITEBT_BUILD_ROOT` environment variable |
| 3 | Ensure write permissions to build directory |
| 4 | Check for path length limitations |

### Module Not Found

| Step | Action |
|------|--------|
| 1 | Verify module in source tree under ProgramsRoot |
| 2 | Check module rule file syntax |
| 3 | Run `we modules` to list discovered modules |
| 4 | Check `ProgramsRoot` configuration |

---

## Advanced Configuration

### Custom Build Steps

```csharp
CustomBuildSteps = new[]
{
    new CustomBuildStep
    {
        Command = "python",
        Arguments = "generate_code.py",
        WorkingDirectory = "$(ModuleDir)"
    }
};
```

| Parameter | Description |
|-----------|-------------|
| `Command` | Executable to run |
| `Arguments` | Command-line arguments |
| `WorkingDirectory` | Directory for execution (use `$(ModuleDir)` for module directory) |

### Conditional Compilation

```csharp
Defines = new[]
{
    "PLATFORM_WINDOWS",
    "CONFIG_DEVELOPMENT",
    "ENABLE_PROFILING"
};
```

### Toolchain Customization

```csharp
CompilerFlags = new[] { "/W4", "/WX" };
LinkerFlags = new[] { "/INCREMENTAL:NO" };
```

| Flag | Purpose |
|------|---------|
| `/W4` | Warning level 4 (high) |
| `/WX` | Treat warnings as errors |
| `/INCREMENTAL:NO` | Disable incremental linking |

---

## Configuration Best Practices

| Practice | Description | Benefit |
|----------|-------------|---------|
| Version Control Config Files | Commit `.engine` and `IgniteBT.SDKs.json` (exclude local paths) | Consistent team configuration |
| Use Relative Paths | Prefer relative over absolute paths | Portable across environments |
| Document Custom Settings | Comment non-standard configurations | Helps other developers |
| Validate Regularly | Run `we doctor` periodically | Catch configuration drift early |
| Separate Local Settings | Use environment variables for machine-specific settings | Reduce merge conflicts |
| Test Clean Builds | Periodically test clean builds | Ensure configuration completeness |

---

## Security Considerations

| Consideration | Best Practice |
|---------------|---------------|
| Sensitive Paths | Use environment variables for credentials |
| Access Control | Restrict build directory access |
| SDK Validation | Obtain SDKs from trusted sources |
| Network Configuration | Use authentication and encryption for distributed cache |

---

## Migration Guide

### Upgrading Configuration

| Step | Action |
|------|--------|
| 1 | Backup existing configuration files |
| 2 | Review changelog for breaking changes |
| 3 | Run `we doctor` to validate new configuration |
| 4 | Test clean build with new configuration |
| 5 | Update deprecated settings |

### Legacy Configuration

Migration tools can import from other build systems, project files, and custom formats. Complex configurations may require manual adjustment.

---

## Support

| Resource | How to Use |
|----------|------------|
| `we doctor --verbose` | Detailed diagnostics |
| Build logs in `Build/Logs/` | Detailed build information |
| [Command Reference](./COMMANDS.md) | Command documentation |
| Main Engine Documentation | Additional context |

---

## Related Documentation

- [Getting Started Guide](./GETTING-STARTED.md)
- [Command Reference](./COMMANDS.md)
- [Architecture Documentation](./ARCHITECTURE.md)
- [Changelog](../../../../../CHANGELOG.md)