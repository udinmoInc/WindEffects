# ⚙️ IgniteBT Configuration Guide

> This guide covers configuring IgniteBT for your development environment, including SDK setup, project configuration, and build system customization. Proper configuration is essential for ensuring that the build system operates correctly and efficiently in your specific development environment. Configuration involves setting up SDKs, defining project structure, specifying build parameters, and tuning performance settings to match your hardware and workflow requirements. This guide provides comprehensive information about all aspects of IgniteBT configuration, from basic setup to advanced customization options.

---

## 📄 Configuration Files

IgniteBT uses several configuration files to manage the build environment, each serving a specific purpose in the overall configuration system.

| File | Purpose | Location | Version Control |
|------|---------|----------|----------------|
| 📝 `.engine` | Project descriptor and structure settings | Repository root | ✅ Yes |
| 🛠️ `IgniteBT.SDKs.json` | SDK path configuration | Repository root | ⚠️ Partial (exclude local paths) |
| 🔄 `bootstrap.manifest` | Build system bootstrap configuration | Auto-generated | ❌ No |

> **💡 Tip:** These files work together to provide a comprehensive and flexible configuration framework that can adapt to different project requirements and development environments.

---

## 🏗️ Project Configuration

### .engine File

The `.engine` file is the primary project descriptor located in the repository root, defining the project structure and build system settings.

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
bootstrap.entry.source=IgniteBT/IgniteBT.csproj
```

#### Configuration Parameters

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
| `bootstrap.entry.source` | Bootstrap source project | `IgniteBT/IgniteBT.csproj` |

> **⚠️ Important:** The `.engine` file is typically committed to version control as it contains essential project-wide configuration. However, certain values may need to be adjusted for local development requirements.

---

## 🛠️ SDK Configuration

### IgniteBT.SDKs.json

The SDK configuration file defines paths to required development SDKs, allowing explicit specification of where SDKs are installed on your system.

**Example:**

```json
{
  "SDKPaths": {
    "VulkanSDK": "F:\\Coding\\windeffects\\Engine\\ThirdParty",
    "SDL3": "F:\\Coding\\windeffects\\Engine\\ThirdParty\\SDL3"
  },
  "AdditionalIncludePaths": {
    "MyCustomLibrary": "C:\\Libraries\\MyCustomLibrary\\include"
  },
  "AdditionalLibraryPaths": {
    "MyCustomLibrary": "C:\\Libraries\\MyCustomLibrary\\lib"
  }
}
```

### Supported SDKs

IgniteBT supports detection and configuration of the following SDKs.

#### 🔺 Vulkan SDK

Required for graphics development. WindEffects Engine uses Vulkan for graphics rendering.

| Platform | Default Paths | Environment Variable |
|----------|--------------|---------------------|
| 🪟 Windows | `C:\VulkanSDK\[version]` | `VULKAN_SDK` |

**Configuration Example:**

```json
{
  "SDKPaths": {
    "VulkanSDK": "C:\\VulkanSDK\\1.3.290.0"
  }
}
```

#### 🎮 SDL3

Used for platform abstraction and input handling, providing cross-platform access to windowing and input devices.

**Configuration Example:**

```json
{
  "SDKPaths": {
    "SDL3": "C:\\SDL3"
  }
}
```

### Additional Include and Library Paths

For custom libraries that are not detected automatically, specify additional paths in the SDK configuration file.

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

> **💡 Use Case:** This capability is important for integrating third-party libraries or custom components that are not part of the standard SDK set.

### SDK Detection Commands

| Command | Purpose | When to Use |
|---------|---------|-------------|
| `we sdk detect` | Force rescan for SDKs | After installing/updating SDKs |
| `we sdk validate` | Validate SDK configuration | To verify configuration is correct |
| `we sdk list` | List all detected SDKs | To see what SDKs are available |

```powershell
# Trigger detection manually
we sdk detect

# Validate SDK configuration
we sdk validate

# List all detected SDKs
we sdk list
```

---

## 🔨 Build Configuration

### Build Configurations

IgniteBT supports three standard build configurations, each optimized for different scenarios.

| Configuration | Optimizations | Debug Symbols | Performance | Binary Size | Use Case |
|--------------|---------------|---------------|-------------|-------------|----------|
| 🐛 **Debug** | None | Maximum | Slowest | Largest | Complex debugging |
| ⚡ **Development** | Enabled | Included | Good | Reasonable | Daily development |
| 🚀 **Shipping** | Maximum | None | Best | Smallest | Release builds |

#### Debug Configuration

Maximum debugging information with no compiler optimizations.

```powershell
we build --config Debug
```

**Characteristics:**
- 🔍 Maximum debugging information
- 🚫 No optimizations
- 🐌 Slowest execution
- 📦 Largest binary size

> **⚠️ Use Only When:** Actively debugging difficult issues that cannot be resolved with the Development configuration.

---

#### Development Configuration

Optimized build with debugging symbols - recommended for daily development.

```powershell
we build --config Development
```

**Characteristics:**
- 🔍 Debugging symbols included
- ⚡ Compiler optimizations enabled
- 🎯 Good performance
- 📦 Reasonable binary size

> **✅ Recommended For:** Most development work. Best compromise between build time, runtime performance, and debugging capability.

---

#### Shipping Configuration

Fully optimized release build with no debugging information.

```powershell
we build --config Shipping
```

**Characteristics:**
- 🚫 No debugging information
- ⚡ Maximum optimizations
- 🏆 Best performance
- 📦 Smallest binary size

> **🎯 Use For:** Final builds that will be distributed to users.

---

### Platform Configuration

IgniteBT supports multiple target platforms for cross-platform development.

| Platform | Description | Default |
|----------|-------------|---------|
| 🪟 **Win64** | Windows 64-bit | ✅ Yes |
| 🖥️ **Windows** | Windows platform-agnostic | No |
| 🐧 **Linux** | Linux 64-bit | No |
| 🍎 **Mac** | macOS 64-bit | No |

```powershell
we build --config Development --platform Win64
```

### Compiler Configuration

IgniteBT uses platform-appropriate compilers automatically.

| Platform | Compiler | Notes |
|----------|----------|-------|
| 🪟 Windows | MSVC (Microsoft Visual C++) | Full Visual Studio integration |
| 🐧 Linux | Clang | GCC compatibility mode |
| 🍎 macOS | Clang | Apple toolchain support |

> **💡 Abstraction:** The build system handles differences in compiler command-line syntax, flag names, and output formats automatically.

---

## 📦 Module Configuration

### Module Rules

Build modules are configured through module rule files, which define compilation settings, dependencies, and output configuration.

**Module rules define:**
- 📄 Source file inclusion patterns
- 🏗️ Compiler flags and defines
- 🔗 Public and private dependencies
- 📋 Precompiled header configuration
- 🔄 Unity build settings

### Module Dependencies

Modules declare dependencies on other modules to specify relationships between components.

| Dependency Type | Description | Transitive |
|-----------------|-------------|------------|
| 🌐 **Public** | Exposed to consumers | Yes |
| 🔒 **Private** | Internal to module | No |

**Example:**

```csharp
PublicDependencies = new[] { "WECore", "WERenderer" };
PrivateDependencies = new[] { "ThirdParty/SDL3" };
```

### Module Output Configuration

Modules specify their output type, which determines what kind of build artifact is produced.

| Output Type | Description | Platform Examples |
|-------------|-------------|-------------------|
| 📱 **Executable** | Standalone applications | Editor.exe, Game.exe |
| 📚 **Shared Library** | Runtime-loaded libraries | .dll (Windows), .so (Linux) |
| 📦 **Static Library** | Link-time libraries | .lib (Windows), .a (Linux/macOS) |

---

## 🌍 Environment Configuration

### Environment Variables

Configure IgniteBT behavior with environment variables for flexible default behavior.

#### Build Configuration Variables

| Variable | Purpose | Example |
|----------|---------|---------|
| `IGNITEBT_CONFIG` | Default build configuration | `Development` |
| `IGNITEBT_PLATFORM` | Default target platform | `Win64` |
| `IGNITEBT_JOBS` | Default parallel job count | `8` |

```powershell
# Default build configuration
set IGNITEBT_CONFIG=Development

# Default target platform
set IGNITEBT_PLATFORM=Win64

# Default parallel job count
set IGNITEBT_JOBS=8
```

#### Output Configuration Variables

| Variable | Purpose | Example |
|----------|---------|---------|
| `IGNITEBT_BUILD_ROOT` | Custom build directory path | `C:\Builds\WindEffects` |
| `IGNITEBT_VERBOSE` | Enable verbose output globally | `1` |

```powershell
# Custom build directory
set IGNITEBT_BUILD_ROOT=C:\Builds\WindEffects

# Enable verbose output globally
set IGNITEBT_VERBOSE=1
```

#### SDK Configuration Variables

| Variable | Purpose | Example |
|----------|---------|---------|
| `VULKAN_SDK` | Override Vulkan SDK path | `C:\VulkanSDK\1.3.290.0` |

```powershell
# Override Vulkan SDK path
set VULKAN_SDK=C:\VulkanSDK\1.3.290.0
```

### Persistent Configuration

| Method | Scope | When to Use |
|--------|-------|-------------|
| 🪟 **System Properties** | System-wide | All applications and users |
| 💻 **PowerShell Profile** | User session | PowerShell-specific configuration |
| 📁 **.env file** | Project-specific | Version-controlled project settings |

---

## 📂 Build Layout Configuration

### Directory Structure

The build output directory structure is configurable through the build layout system.

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
| 📦 **Output** | Final binaries and libraries | Primary build products |
| 🔨 **Intermediate** | Object files and PDBs | Incremental build support |
| 🤖 **Generated** | Generated source files | Code generation artifacts |
| 💾 **Cache** | Build cache | Incremental build acceleration |
| 🗄️ **Database** | Metadata and dependencies | Persistent build state |
| 📋 **Logs** | Build and runtime logs | Troubleshooting and analysis |
| 📄 **Manifest** | Build descriptors | Build process documentation |

### Custom Build Root

To use a custom build directory, set the environment variable:

```powershell
set IGNITEBT_BUILD_ROOT=D:\Builds\WindEffects
```

Or modify the `.engine` file:

```ini
BuildRoot=D:\Builds\WindEffects
```

> **💡 Benefit:** Useful for separating build artifacts from source code or placing build artifacts on a different drive for performance.

---

## 🌐 Distributed Build Configuration

### Worker Configuration

For distributed builds, configure worker processes to handle build tasks across multiple machines.

```powershell
we daemon --workers 4 --port 8080
```

| Parameter | Description | Default |
|-----------|-------------|---------|
| `--workers` | Number of worker processes | Auto-detected |
| `--port` | Port for daemon to listen on | Default port |

### Cache Configuration

Distributed build cache settings enable sharing of build artifacts across multiple build machines.

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

## ⚡ Performance Tuning

### Parallel Job Configuration

Optimize the parallel job count based on your system's capabilities.

```powershell
# For CPU-bound builds
we build --jobs 8

# For I/O-bound builds
we build --jobs 4

# Use all available cores
we build --jobs 0
```

> **💡 Auto-Detection:** When you specify a job count of zero, IgniteBT automatically determines the optimal number based on your system's hardware.

### Cache Configuration

Build cache settings affect incremental build performance.

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

Enable precompiled headers for faster compilation by reducing header parsing time.

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

Configure unity builds to reduce compilation overhead by combining multiple source files.

```csharp
UnityBuildEnabled = true;
UnityBuildBatchSize = 16;
```

| Setting | Description |
|---------|-------------|
| `UnityBuildEnabled` | Enable unity builds for module |
| `UnityBuildBatchSize` | Files per unity build batch |

---

## 🔍 Diagnostic Configuration

### Logging Configuration

Configure logging verbosity and output to control information displayed during builds.

```powershell
# Enable verbose logging
we build --verbose

# Set log level via environment
set IGNITEBT_LOG_LEVEL=Verbose
```

> **🔧 Troubleshooting:** Verbose logging provides detailed information including exact compiler commands, file-by-file progress, dependency resolution, and linker output.

### Build Reports

Generate detailed build reports to analyze performance and identify optimization opportunities.

```powershell
we build --report
```

Reports are saved to `Build/Logs/` with:
- ⏱️ Total build time
- 📊 Time per build phase
- 📄 Individual file compilation times
- 🔗 Linking times
- 💾 Cache hit rates

---

## ✅ Validation and Testing

### Configuration Validation

Validating your configuration ensures the build system is correctly configured.

| Command | Purpose | What It Checks |
|---------|---------|---------------|
| `we doctor` | Full environment diagnostics | SDKs, tools, permissions, dependencies |
| `we sdk validate` | Validate SDK configuration | SDK availability and version requirements |
| `we project validate` | Validate project configuration | Project structure and consistency |

```powershell
# Run full diagnostics
we doctor

# Validate SDK configuration
we sdk validate

# Validate project configuration
we project validate
```

### Test Builds

Testing your configuration with a clean build is the ultimate validation step.

```powershell
we clean
we build --config Development --verbose
```

> **✅ Best Practice:** Run a clean build after making configuration changes to ensure everything works from a fresh start.

---

## 🎯 Common Configuration Scenarios

### Development Workstation

Optimized for daily development work, prioritizing iteration speed and debugging capability.

```powershell
set IGNITEBT_CONFIG=Development
set IGNITEBT_JOBS=8
set IGNITEBT_VERBOSE=0
```

**Characteristics:**
- ⚡ Fast incremental builds
- 🔍 Debugging capability when needed
- 🎯 Good balance of optimization and debug info

---

### CI/CD Pipeline

Optimized for automated builds, prioritizing reproducibility and detailed logging.

```powershell
set IGNITEBT_CONFIG=Shipping
set IGNITEBT_JOBS=0
set IGNITEBT_VERBOSE=1
```

**Characteristics:**
- 🔄 Consistent and reproducible builds
- 📋 Detailed logs for troubleshooting
- 🚀 Release-quality binaries

---

### Release Build

Optimized for final releases, prioritizing performance and binary size.

```powershell
set IGNITEBT_CONFIG=Shipping
set IGNITEBT_JOBS=16
set IGNITEBT_BUILD_ROOT=D:\Releases\WindEffects
```

**Characteristics:**
- 🏆 Maximum optimizations
- 📦 Smallest binary size
- 🎯 Separate release artifacts

---

## 🚨 Troubleshooting Configuration

### SDK Not Found

If an SDK is not detected, follow these steps:

| Step | Action | Purpose |
|------|--------|---------|
| 1️⃣ | Verify SDK installation | Confirm SDK is installed and version meets requirements |
| 2️⃣ | Check `IgniteBT.SDKs.json` paths | Ensure paths are correct and properly escaped |
| 3️⃣ | Run `we sdk detect` | Refresh detection process |
| 4️⃣ | Run `we sdk validate` | Verify configuration is correct |

---

### Build Path Issues

If build paths are incorrect:

| Step | Action | Purpose |
|------|--------|---------|
| 1️⃣ | Check `BuildRoot` in `.engine` file | Ensure path is correctly specified |
| 2️⃣ | Verify `IGNITEBT_BUILD_ROOT` environment variable | Check for overrides |
| 3️⃣ | Ensure write permissions to build directory | Verify user has write access |
| 4️⃣ | Check for path length limitations | Windows has maximum path length limits |

---

### Module Not Found

If a module is not discovered:

| Step | Action | Purpose |
|------|--------|---------|
| 1️⃣ | Verify module is in source tree | Confirm location under ProgramsRoot |
| 2️⃣ | Check module rule file syntax | Ensure file follows expected format |
| 3️⃣ | Run `we modules` to list discovered modules | See what modules are found |
| 4️⃣ | Check `ProgramsRoot` configuration | Ensure it points to correct directory |

---

## 🔧 Advanced Configuration

### Custom Build Steps

Add custom build steps through module rules to extend the build process.

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

---

### Conditional Compilation

Configure conditional compilation based on platform or configuration.

```csharp
Defines = new[]
{
    "PLATFORM_WINDOWS",
    "CONFIG_DEVELOPMENT",
    "ENABLE_PROFILING"
};
```

> **💡 Use Case:** Create code that builds for different platforms or configurations without maintaining separate codebases.

---

### Toolchain Customization

Customize compiler and linker flags to fine-tune the build process.

```csharp
CompilerFlags = new[] { "/W4", "/WX" };
LinkerFlags = new[] { "/INCREMENTAL:NO" };
```

| Flag | Purpose |
|------|---------|
| `/W4` | Set warning level to 4 (high) |
| `/WX` | Treat warnings as errors |
| `/INCREMENTAL:NO` | Disable incremental linking |

---

## 📋 Configuration Best Practices

Following these best practices ensures maintainable, reliable configuration.

| Practice | Description | Benefit |
|----------|-------------|---------|
| ✅ **Version Control Config Files** | Commit `.engine` and `IgniteBT.SDKs.json` (exclude local paths) | Consistent team configuration |
| 🔄 **Use Relative Paths** | Prefer relative over absolute paths | Portable across environments |
| 📝 **Document Custom Settings** | Add comments explaining non-standard configurations | Helps other developers understand |
| 🔍 **Validate Regularly** | Run `we doctor` periodically | Catch configuration drift early |
| 🌍 **Separate Local Settings** | Use environment variables for machine-specific settings | Reduce merge conflicts |
| 🧪 **Test Clean Builds** | Periodically test clean builds | Ensure configuration completeness |

---

## 🔒 Security Considerations

Security is important when configuring build systems.

| Consideration | Description | Best Practice |
|---------------|-------------|----------------|
| 🔐 **Sensitive Paths** | Avoid committing paths with sensitive information | Use environment variables for credentials |
| 🛡️ **Access Control** | Ensure build directories have appropriate permissions | Restrict access to authorized users |
| ✅ **SDK Validation** | Validate SDK integrity before use | Obtain SDKs from trusted sources |
| 🔗 **Network Configuration** | Secure distributed build cache connections | Use authentication and encryption |

---

## 🔄 Migration Guide

### Upgrading Configuration

When upgrading IgniteBT to a new version:

| Step | Action | Purpose |
|------|--------|---------|
| 1️⃣ | Backup existing configuration files | Safety net for rollback |
| 2️⃣ | Review changelog for breaking changes | Understand what has changed |
| 3️⃣ | Run `we doctor` to validate new configuration | Catch configuration problems |
| 4️⃣ | Test clean build with new configuration | Ensure everything works correctly |
| 5️⃣ | Update deprecated settings | Use new recommended approaches |

---

### Legacy Configuration

For projects with legacy build systems, IgniteBT provides migration tools to import existing configurations.

> **💡 Migration Tools:** Can import configuration from various sources including other build systems, project files, and custom formats. Complex configurations may require manual adjustment after automated migration.

---

## 🆘 Support

For configuration issues not covered in this guide:

| Resource | How to Use |
|----------|------------|
| 🔍 `we doctor --verbose` | Detailed diagnostics of build environment |
| 📋 Build logs in `Build/Logs/` | Detailed information about build operations |
| 📖 [Command Reference](./COMMANDS.MD) | Detailed command documentation |
| 📚 Main Engine Documentation | Additional context and guidance |

---

## 🔗 Related Documentation

- 📖 [Getting Started Guide](./GETTING-STARTED.MD)
- 💻 [Command Reference](./COMMANDS.MD)
- 🏗️ [Architecture Documentation](./ARCHITECTURE.MD)
- 📋 [Changelog](../../CHANGELOG.MD)
