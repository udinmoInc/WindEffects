# Getting Started with IgniteBT

This guide walks you through setting up the WindEffects Engine build environment using IgniteBT. IgniteBT is integrated into WindEffects Engine and requires no separate installation.

---

## Prerequisites

| Software | Version | Purpose | Download |
|----------|---------|---------|----------|
| .NET SDK | 8.0 | Build system runtime | [dotnet.microsoft.com](https://dotnet.microsoft.com) |
| Visual Studio | 2022 | C++ compiler and toolchain | [visualstudio.microsoft.com](https://visualstudio.microsoft.com) |
| Git | Latest | Version control | [git-scm.com](https://git-scm.com) |

**Installation Tips:**
- Install Visual Studio 2022 with "Desktop development with C++" workload
- Ensure latest Visual Studio updates are installed
- Add all tools to system PATH during installation
- Verify with `dotnet --version`, `git --version`

---

## Initial Setup

### 1. Clone the Repository

```powershell
git clone https://github.com/udinmoInc/windeffects.git
cd windeffects
```

### 2. Verify Environment

Run the diagnostic command:

```powershell
we doctor
```

The doctor command checks:
- .NET SDK installation and version
- Visual Studio installation and C++ toolchain
- Other required tools and dependencies

If issues are detected, the command provides guidance on resolving them.

### 3. Configure SDK Paths

If doctor reports missing SDKs or you have SDKs in non-standard locations, configure paths in `IgniteBT.SDKs.json`:

```json
{
  "SDKPaths": {},
  "AdditionalIncludePaths": {},
  "AdditionalLibraryPaths": {}
}
```

Then run:

```powershell
we sdk detect
we sdk validate
```

### 4. Install `we` Command Globally (Optional)

```powershell
we setup --global
```

This adds `we` to your system PATH. Restart your terminal for changes to take effect.

---

## Your First Build

### Building in Development Configuration

```powershell
we build --config Development
```

**First Build Steps:**

| Step | Description | Time Impact |
|------|-------------|-------------|
| Compile IgniteBT | Build the build system itself | One-time |
| Dependencies | Download and build third-party libraries | One-time |
| Engine Modules | Compile all engine components | Every build |
| Generated Artifacts | Generate export files and code | Every build |

The first build takes longer due to one-time setup. Subsequent builds are significantly faster due to incremental building and caching.

### Build Output

Build artifacts are organized in the `Build` directory:

```
Build/
├── Output/
│   └── Development/
│       └── Win64/
│           ├── WECore.dll
│           ├── WERenderer.dll
│           └── Editor.exe
├── Intermediate/
│   └── Development/
│       └── Win64/
│           └── [Object files and PDBs]
└── Logs/
    └── IgniteBT-[date].log
```

| Directory | Contents | Purpose |
|-----------|----------|---------|
| Output | Final binaries and libraries | Primary build products |
| Intermediate | Object files and PDBs | Incremental build support |
| Logs | Build and runtime logs | Troubleshooting |

### Running the Editor

```powershell
we run --target Editor --config Development
```

The run command locates the built executable, sets up the environment, and launches with correct settings.

---

## Build Configurations

| Configuration | Optimizations | Debug Symbols | Use Case | Performance |
|---------------|---------------|---------------|----------|-------------|
| Debug | None | Maximum | Complex debugging | Slowest |
| Development | Enabled | Included | Daily development | Good |
| Shipping | Maximum | None | Final releases | Best |

### Debug Configuration

```powershell
we build --config Debug
```

- Maximum debugging information
- No compiler optimizations
- Slowest execution, largest binary size
- Use only when actively debugging difficult issues

### Development Configuration (Recommended)

```powershell
we build --config Development
```

- Debugging symbols included
- Compiler optimizations enabled
- Good performance for interactive testing
- Best compromise for daily development

### Shipping Configuration

```powershell
we build --config Shipping
```

- No debugging information
- Maximum optimizations
- Best performance, smallest binary size
- Use for final distribution builds

---

## Building Specific Modules

For faster iteration, build specific modules:

```powershell
# Build only the core module
we build --target WECore --config Development

# Build only the renderer
we build --target WERenderer --config Development
```

List available modules:
```powershell
we modules
```

Building specific modules reduces build time by focusing on active work. Dependencies are handled automatically.

---

## Cleaning Build Artifacts

```powershell
# Clean specific configuration
we clean --config Debug

# Clean specific module
we clean --target WECore --config Development

# Clean all artifacts
we clean
```

Safe operation: removes build directory files only, not source or configuration.

---

## Rebuilding

```powershell
# Rebuild entire project
we rebuild --config Development

# Rebuild specific module
we rebuild --target WECore --config Debug
```

Useful for troubleshooting stale artifacts or when configuration changes require a complete rebuild.

---

## Parallel Builds

```powershell
# Use 8 parallel jobs
we build --config Development --jobs 8

# Use all available cores (auto-detect)
we build --config Development --jobs 0
```

---

## Verbose Output

```powershell
we build --config Development --verbose
```

Shows: exact compiler commands, file-by-file progress, dependency resolution, linker output.

Best for: diagnosing build failures or understanding change impact.

---

## Troubleshooting

### Build Failures

| Step | Command | Purpose |
|------|---------|---------|
| 1 | Check `Build/Logs/IgniteBT-[date].log` | Detailed error messages |
| 2 | `we doctor --verbose` | Comprehensive diagnostics |
| 3 | `we rebuild --config Debug --verbose` | Clean rebuild with details |
| 4 | `we sdk validate` | Verify SDK configuration |

### Missing Dependencies

| Step | Action |
|------|--------|
| 1 | Check internet connectivity (some deps downloaded remotely) |
| 2 | Check `Build/Logs` for detailed errors |
| 3 | Clean `Build/Intermediate/ThirdParty` to force rebuild |

### SDK Detection Issues

| Step | Action |
|------|--------|
| 1 | Verify SDK installation paths |
| 2 | Update `IgniteBT.SDKs.json` with explicit paths |
| 3 | Run `we sdk detect` |
| 4 | Run `we sdk validate` |

### Out-of-Memory Errors

| Solution | Command/Action |
|----------|----------------|
| Reduce parallel jobs | `--jobs 4` |
| Close memory-intensive apps | Frees up memory |
| Increase page file | System settings |
| Use 64-bit tools (default) | Addresses more memory |

---

## Next Steps

After your first build:

| Resource | Description |
|----------|-------------|
| [Command Reference](./COMMANDS.md) | Detailed command information |
| [Architecture Documentation](./ARCHITECTURE.md) | System design |
| [Configuration Guide](./CONFIGURATION.md) | SDK configuration and advanced tuning |

---

## Performance Tips

### Optimize Build Times

| Strategy | Implementation |
|----------|----------------|
| Use Development Config | `--config Development` |
| Enable Parallel Builds | `--jobs <core_count>` |
| Build Specific Modules | `--target <module>` |
| Use Incremental Builds | Avoid `clean` unless necessary |
| SSD Storage | Keep build directory on SSD |

The Development configuration provides the best balance of build time, runtime performance, and debugging capability.

### Cache Management

```powershell
# Clear the cache
we clean --cache
```

Clear cache if you experience cache-related issues (incorrect artifacts reused, corruption). Subsequent builds will rebuild from a clean cache.

---

## Continuous Integration

Recommended CI/CD pipeline:

```powershell
# CI build script
we doctor
we sdk validate
we clean --config Shipping
we build --config Shipping --jobs 0 --verbose
we package --config Shipping
```

| Step | Purpose |
|------|---------|
| `we doctor` | Verify build environment |
| `we sdk validate` | Ensure dependencies available |
| `we clean` | Fresh start per CI build |
| `we build` | Maximum parallelism with verbose output |
| `we package` | Prepare artifacts for distribution |

---

## Getting Help

| Resource | How to Use |
|----------|------------|
| Build Logs | Check `Build/Logs/` for detailed errors |
| Diagnostics | Run `we doctor --verbose` |
| Command Reference | Review [COMMANDS.md](./COMMANDS.md) |
| Engine Documentation | Consult main WindEffects Engine docs |

---

## Environment Variables

Configure defaults without specifying options on every command:

```powershell
# Default build configuration
set IGNITEBT_CONFIG=Development

# Default parallel job count
set IGNITEBT_JOBS=8

# Enable verbose output globally
set IGNITEBT_VERBOSE=1
```

| Variable | Purpose | Example |
|----------|---------|---------|
| `IGNITEBT_CONFIG` | Default build configuration | `Development` |
| `IGNITEBT_JOBS` | Default parallel job count | `8` |
| `IGNITEBT_VERBOSE` | Enable verbose output globally | `1` |

Useful for consistent defaults across environments or CI/CD configuration.

---

## Uninstalling

| Step | Action |
|------|--------|
| 1 | `Remove-Item -Recurse Build` |
| 2 | Remove global `we` command from system PATH |
| 3 | Delete environment variables |

---

## Quick Reference

```powershell
# Build project
we build --config Development

# Run editor
we run --target Editor --config Development

# Clean and rebuild
we rebuild --config Development

# Run diagnostics
we doctor
```

---

## Related Documentation

- [Command Reference](./COMMANDS.md)
- [Architecture Documentation](./ARCHITECTURE.md)
- [Configuration Guide](./CONFIGURATION.md)
- [Changelog](../../../../../CHANGELOG.md)