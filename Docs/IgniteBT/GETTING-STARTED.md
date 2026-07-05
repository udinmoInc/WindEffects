# 🚀 Getting Started with IgniteBT

> This guide walks you through setting up IgniteBT and performing your first build, providing a comprehensive introduction to the build system for WindEffects Engine. IgniteBT is the build system for WindEffects Engine and is automatically included with the engine, requiring no separate installation or download. The guide covers everything from initial environment setup to performing your first build and troubleshooting common issues. By following this guide, you will have a fully configured development environment and a working build of the engine, ready for development work.

---

## 📋 Prerequisites

Before using IgniteBT, ensure your development environment meets these requirements. Having the correct tools and SDKs installed is essential for a smooth build process and to avoid common issues that can arise from missing or misconfigured dependencies. The prerequisites are divided into required software that must be installed for the build system to function and optional software that may be needed depending on the specific project configuration or third-party dependencies.

### ✅ Required Software

| Software | Version | Purpose | Download |
|----------|---------|---------|----------|
| 🟦 **.NET SDK** | 8.0 | Build system runtime | [dotnet.microsoft.com](https://dotnet.microsoft.com) |
| 🎨 **Visual Studio** | 2022 | C++ compiler and toolchain | [visualstudio.microsoft.com](https://visualstudio.microsoft.com) |
| 🔺 **Vulkan SDK** | Latest | Graphics development dependencies | [vulkan.lunarg.com](https://vulkan.lunarg.com) |
| 📦 **Git** | Latest | Version control | [git-scm.com](https://git-scm.com) |

> **💡 Installation Tips:**
> - Install Visual Studio 2022 with the "Desktop development with C++" workload
> - Ensure the latest updates are installed for Visual Studio
> - Add all tools to your system PATH during installation
> - Verify installations with `dotnet --version`, `git --version`, etc.

### 🔧 Optional Software

| Software | Purpose | When Needed |
|----------|---------|-------------|
| 🛠️ **CMake** | Build third-party dependencies | Projects with CMake-based dependencies |
| 🐍 **Python 3** | Build scripts and tools | Projects using Python build scripts |
| 🐫 **Perl** | Third-party library builds | Specific libraries requiring Perl |

---

## 🛠️ Initial Setup

### 1️⃣ Clone the Repository

The first step in setting up your development environment is to clone the WindEffects Engine repository to your local machine. This will give you access to all the source code, build configuration, and other files needed to build the engine.

```powershell
git clone https://github.com/your-org/windeffects.git
cd windeffects
```

> **📁 Repository Contents:** The repository contains the entire engine source code along with the IgniteBT build system, so cloning it gives you everything you need in one operation.

---

### 2️⃣ Verify Environment

After cloning the repository, you should verify that your development environment is correctly configured by running the diagnostic command. The doctor command performs a comprehensive check of your environment.

```powershell
we doctor
```

The doctor command checks:
- ✅ .NET SDK installation and version
- ✅ Visual Studio installation and C++ toolchain
- ✅ Vulkan SDK availability and configuration
- ✅ Other required tools and dependencies

> **⚠️ If Issues Are Detected:** The doctor command will provide clear guidance on resolving them, including information about missing tools, version incompatibilities, or configuration problems.

---

### 3️⃣ Configure SDK Paths

If the doctor command reports missing SDKs or if you have SDKs installed in non-standard locations, you will need to configure the SDK paths in the `IgniteBT.SDKs.json` file.

```json
{
  "SDKPaths": {
    "VulkanSDK": "C:\\VulkanSDK\\1.3.290.0",
    "SDL3": "C:\\SDL3"
  },
  "AdditionalIncludePaths": {},
  "AdditionalLibraryPaths": {}
}
```

After updating the SDK configuration file, run the SDK detection and validation commands:

```powershell
we sdk detect
we sdk validate
```

---

### 4️⃣ Install the `we` Command Globally (Optional)

For convenient access from any directory, you can install the `we` command globally on your system.

```powershell
we setup --global
```

> **💡 Benefit:** Global installation adds the `we` command to your system PATH, making it available system-wide just like other command-line tools. You may need to restart your terminal for PATH changes to take effect.

---

## 🔨 Your First Build

### Building in Development Configuration

The Development configuration provides optimized builds with debugging symbols, making it ideal for active development work. This configuration strikes a balance between performance and debuggability.

```powershell
we build --config Development
```

#### What Happens During First Build?

| Step | Description | Time Impact |
|------|-------------|-------------|
| 🏗️ **Compile IgniteBT** | Build the build system itself | One-time setup |
| 📦 **Dependencies** | Download and build third-party libraries | One-time setup |
| 🔧 **Engine Modules** | Compile all engine components | Every build |
| 🤖 **Generated Artifacts** | Generate export files and code | Every build |

> **⏱️ First Build Time:** The first build will take longer than subsequent builds due to one-time setup tasks. Subsequent builds will be significantly faster due to incremental building and caching.

---

### Build Output

Build artifacts are organized in the `Build` directory, which is created automatically the first time you build.

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
| 📦 **Output** | Final binaries and libraries | Primary build products |
| 🔨 **Intermediate** | Object files and PDBs | Incremental build support |
| 📋 **Logs** | Build and runtime logs | Troubleshooting and analysis |

---

### Running the Editor

After a successful build, you can launch the editor to begin working with the engine.

```powershell
we run --target Editor --config Development
```

> **🎯 Automatic Setup:** The run command automatically locates the built executable, sets up the required environment, and launches the program with correct settings.

---

## ⚙️ Common Build Configurations

IgniteBT supports three main configurations, each optimized for different scenarios and stages of development.

| Configuration | Optimizations | Debug Symbols | Use Case | Performance |
|--------------|---------------|---------------|----------|-------------|
| 🐛 **Debug** | None | Maximum | Debugging complex issues | Slowest |
| ⚡ **Development** | Enabled | Included | Daily development work | Good |
| 🚀 **Shipping** | Maximum | None | Final releases and distribution | Best |

### Debug Configuration

Full debugging information with no compiler optimizations.

```powershell
we build --config Debug
```

**Characteristics:**
- 🔍 Maximum debugging information for comprehensive inspection
- 🚫 No compiler optimizations for accurate debugging
- 🐌 Slowest execution due to lack of optimizations
- 📦 Largest binary size due to embedded symbols

> **⚠️ Use Only When:** Actively debugging difficult issues. Performance penalty makes it impractical for regular development.

---

### Development Configuration

Optimized build with debugging symbols - recommended for daily development.

```powershell
we build --config Development
```

**Characteristics:**
- 🔍 Debugging symbols included for debugging when needed
- ⚡ Compiler optimizations enabled for performance
- 🎯 Good performance for interactive testing
- 📦 Reasonable binary size balancing debug info and optimization

> **✅ Recommended For:** Most development work. Best compromise between build time, runtime performance, and debugging capability.

---

### Shipping Configuration

Fully optimized release build suitable for final distribution.

```powershell
we build --config Shipping
```

**Characteristics:**
- 🚫 No debugging information to minimize binary size
- ⚡ Maximum optimizations for best runtime performance
- 🏆 Best performance among all configurations
- 📦 Smallest binary size for efficient distribution

> **🎯 Use For:** Final builds that will be distributed to users. Optimal combination of small size and fast execution.

---

## 🎯 Building Specific Modules

For faster iteration during development, you can build specific modules rather than the entire project.

```powershell
# Build only the core module
we build --target WECore --config Development

# Build only the renderer
we build --target WERenderer --config Development
```

To see which modules are available in the project:

```powershell
we modules
```

> **💡 Benefit:** Building specific modules significantly reduces build time by focusing compilation on only the parts you're actively working on. The build system automatically handles dependencies.

---

## 🧹 Cleaning Build Artifacts

Cleaning build artifacts removes intermediate and output files, forcing a complete rebuild on the next build.

```powershell
# Clean specific configuration
we clean --config Debug

# Clean specific module
we clean --target WECore --config Development

# Clean all artifacts
we clean
```

> **🔒 Safe Operation:** The clean command removes files from the build directory but does not affect source code or configuration files, making it safe to run at any time.

---

## 🔄 Rebuilding

The rebuild command combines cleaning and building into a single operation.

```powershell
# Rebuild entire project
we rebuild --config Development

# Rebuild specific module
we rebuild --target WECore --config Debug
```

> **🎯 Use Case:** Particularly useful when troubleshooting build issues caused by stale artifacts or when configuration changes require a complete rebuild.

---

## ⚡ Parallel Builds

Controlling the number of parallel jobs allows you to tune the build process for your specific hardware.

```powershell
# Use 8 parallel jobs
we build --config Development --jobs 8

# Use all available cores
we build --config Development --jobs 0
```

> **💡 Auto-Detection:** When you specify a job count of zero, IgniteBT automatically determines the optimal number based on your system's hardware capabilities.

---

## 🔍 Verbose Output

Enabling verbose output provides detailed information about the build process.

```powershell
we build --config Development --verbose
```

**Verbose output shows:**
- 📝 Exact compiler commands with all flags and parameters
- 📄 File-by-file compilation progress
- 🔗 Dependency resolution details
- 🔨 Linker output and warnings

> **🔧 Best For:** Diagnosing build failures or understanding the impact of changes on the build process.

---

## 🚨 Troubleshooting

### Build Failures

Build failures can occur for various reasons. Follow this systematic approach to diagnose and resolve issues.

| Step | Command | Purpose |
|------|---------|---------|
| 1️⃣ | Check `Build/Logs/IgniteBT-[date].log` | Review detailed error messages |
| 2️⃣ | `we doctor --verbose` | Run comprehensive diagnostics |
| 3️⃣ | `we rebuild --config Debug --verbose` | Try clean rebuild with details |
| 4️⃣ | `we sdk validate` | Verify SDK configuration |

---

### Missing Dependencies

Missing dependencies can occur when third-party libraries fail to build or required tools are not installed.

| Step | Action | Purpose |
|------|--------|---------|
| 1️⃣ | Check internet connectivity | Some dependencies are downloaded remotely |
| 2️⃣ | Verify CMake, Perl, Python | Required for third-party builds |
| 3️⃣ | Check `Build/Logs` | Review detailed error messages |
| 4️⃣ | Clean `Build/Intermediate/ThirdParty` | Force rebuild of dependencies |

---

### SDK Detection Issues

SDK detection issues occur when the build system cannot locate required SDKs.

| Step | Action | Purpose |
|------|--------|---------|
| 1️⃣ | Verify SDK installation paths | Confirm actual installation locations |
| 2️⃣ | Update `IgniteBT.SDKs.json` | Explicitly specify SDK paths |
| 3️⃣ | Run `we sdk detect` | Refresh detection process |
| 4️⃣ | Run `we sdk validate` | Verify configuration is correct |

---

### Out-of-Memory Errors

Out-of-memory errors can occur when building very large projects with high parallelism.

| Solution | Command/Action | Effect |
|----------|----------------|--------|
| 🔽 Reduce parallel jobs | `--jobs 4` | Reduces peak memory usage |
| 🗑️ Close applications | Close memory-intensive apps | Frees up memory |
| 💾 Increase page file | System settings | More virtual memory |
| 🖥️ Use 64-bit tools | Default | Addresses more memory |

---

## 📚 Next Steps

After completing your first build and verifying that everything works correctly, explore these resources to deepen your understanding of IgniteBT.

| Resource | Description | Link |
|----------|-------------|------|
| 📖 **Command Reference** | Detailed information about all commands | [COMMANDS.md](./COMMANDS.md) |
| 🏗️ **Architecture Documentation** | System design and component organization | [ARCHITECTURE.md](./ARCHITECTURE.md) |
| ⚙️ **Configuration Guide** | SDK configuration and advanced tuning | [CONFIGURATION.md](./CONFIGURATION.md) |

---

## ⚡ Performance Tips

### Optimize Build Times

Several strategies can significantly reduce build times.

| Strategy | Impact | Implementation |
|----------|--------|----------------|
| ⚡ **Use Development Config** | Faster than Debug, more useful than Shipping | `--config Development` |
| 🧵 **Enable Parallel Builds** | Utilize multiple CPU cores | `--jobs <core_count>` |
| 🎯 **Build Specific Modules** | Focus on active work | `--target <module>` |
| 🔄 **Use Incremental Builds** | Reuse work from previous builds | Avoid `clean` unless necessary |
| 💾 **SSD Storage** | Faster I/O operations | Keep build directory on SSD |

> **💡 Pro Tip:** The Development configuration is the optimal choice for most development work, providing the best balance of build time, runtime performance, and debugging capability.

---

### Cache Management

IgniteBT maintains a build cache to speed up incremental builds.

```powershell
# Clear the cache
we clean --cache
```

> **🧹 When to Clear Cache:** If you experience cache-related issues such as incorrect artifacts being reused or cache corruption. Clearing forces a fresh rebuild but subsequent builds will benefit from a clean cache.

---

## 🔄 Continuous Integration

For CI/CD pipelines, use these recommended practices for reliable automated builds.

```powershell
# CI build script example
we doctor
we sdk validate
we clean --config Shipping
we build --config Shipping --jobs 0 --verbose
we package --config Shipping
```

| Step | Purpose |
|------|---------|
| 🔍 `we doctor` | Verify build environment is configured |
| ✅ `we sdk validate` | Ensure all required dependencies are available |
| 🧹 `we clean` | Fresh start for each CI build |
| ⚡ `we build` | Build with maximum parallelism and verbose output |
| 📦 `we package` | Prepare artifacts for distribution |

---

## ❓ Getting Help

If you encounter issues not covered in this guide, use these resources.

| Resource | How to Use |
|----------|------------|
| 📋 **Build Logs** | Check `Build/Logs/` for detailed error messages |
| 🔍 **Diagnostics** | Run `we doctor --verbose` for comprehensive checks |
| 📖 **Command Reference** | Review [COMMANDS.md](./COMMANDS.md) for detailed command info |
| 📚 **Engine Documentation** | Consult main WindEffects Engine documentation |

---

## 🌍 Environment Variables

Configure default behavior with environment variables to customize the build system without specifying options on every command.

```powershell
# Set default configuration
set IGNITEBT_CONFIG=Development

# Set default parallel job count
set IGNITEBT_JOBS=8

# Enable verbose output globally
set IGNITEBT_VERBOSE=1
```

| Variable | Purpose | Example |
|----------|---------|---------|
| `IGNITEBT_CONFIG` | Default build configuration | `Development` |
| `IGNITEBT_JOBS` | Default parallel job count | `8` |
| `IGNITEBT_VERBOSE` | Enable verbose output globally | `1` |

> **💡 Use Case:** Environment variables are particularly useful for setting up consistent defaults across your development environment or for configuring CI/CD pipelines.

---

## 🗑️ Uninstalling

If you need to remove IgniteBT from your system, follow these steps.

| Step | Command/Action | Purpose |
|------|----------------|---------|
| 1️⃣ | `Remove-Item -Recurse Build` | Delete build directory and artifacts |
| 2️⃣ | Remove global `we` command | Remove from system PATH |
| 3️⃣ | Delete environment variables | Clean up environment configuration |

---

## 🎉 Congratulations!

You've successfully set up IgniteBT and completed your first build. You're now ready to develop with WindEffects Engine.

### Quick Reference

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

## 🔗 Related Documentation

- 📖 [Command Reference](./COMMANDS.md)
- 🏗️ [Architecture Documentation](./ARCHITECTURE.md)
- ⚙️ [Configuration Guide](./CONFIGURATION.md)
- 📋 [Changelog](../../CHANGELOG.md)
