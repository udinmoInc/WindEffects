#pragma once

#include "Compilation/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::compilation {

struct COMPILATION_API CompileId {
    std::uint64_t value = 0;
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const CompileId& o) const noexcept {
        return value == o.value;
    }
};

struct COMPILATION_API CompileIdHash {
    [[nodiscard]] std::size_t operator()(const CompileId& id) const noexcept {
        return static_cast<std::size_t>(id.value);
    }
};

enum class CompilerKind : std::uint16_t {
    Unknown = 0,
    ShaderHLSL,
    ShaderGLSL,
    SpirV,
    Dxil,
    Metal,
    Vulkan,
    DirectX,
    OpenGL,
    Compute,
    Material,
    MaterialGraph,
    AnimationGraph,
    VisualScriptGraph,
    UiShader,
    TerrainShader,
    MeshProcess,
    Custom = 1000,
};

enum class CompilePriority : std::uint8_t {
    Background = 0,
    Low,
    Normal,
    High,
    Critical,
};

enum class CompileStatus : std::uint8_t {
    Queued = 0,
    Scheduled,
    Running,
    Succeeded,
    Failed,
    Cancelled,
    CacheHit,
    Invalidated,
};

enum class CompileSeverity : std::uint8_t {
    Info = 0,
    Warning,
    Error,
    Fatal,
};

enum class CompileTargetFormat : std::uint8_t {
    Auto = 0,
    SpirV,
    Dxil,
    Metallib,
    Glsl,
    BinaryBlob,
    ReflectionMeta,
};

struct COMPILATION_API CompileDiagnostic {
    CompileSeverity severity = CompileSeverity::Error;
    std::string code;
    std::string message;
    std::string file;
    std::uint32_t line = 0;
    std::uint32_t column = 0;
};

struct COMPILATION_API CompileSourceDesc {
    std::string path;
    std::string virtualPath;
    std::vector<std::uint8_t> inlineBytes; // optional in-memory source
    std::uint64_t contentHash = 0;
};

struct COMPILATION_API CompileOptions {
    CompileTargetFormat target = CompileTargetFormat::Auto;
    std::string entryPoint = "main";
    std::string stage; // VS / PS / CS / GS / ...
    std::vector<std::string> defines;
    std::vector<std::string> includePaths;
    bool forceRebuild = false;
    bool enableCaching = true;
    bool deterministic = true;
    std::uint32_t optimizationLevel = 1;
};

struct COMPILATION_API CompilationConfig {
    std::uint32_t workerCount = 0; // 0 = hardware_concurrency
    std::uint32_t maxQueueDepth = 100'000;
    bool enableBackgroundCompilation = true;
    bool enableIncremental = true;
    bool enableDistributedHooks = true;
    bool deterministicBuilds = true;
    std::string cacheDirectory;
    std::string databasePath;
    std::uint32_t compilerVersion = 1;
};

} // namespace we::runtime::compilation
