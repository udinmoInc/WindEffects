#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>
#include <functional>
#include <unordered_map>
#include <filesystem>

namespace IgniteBT {

// Forward declarations
class BuildContext;
class BuildTarget;
class BuildModule;
class BuildGraph;

// Platform enum
enum class Platform {
    Windows,
    Linux,
    Mac,
    Unknown
};

// Architecture enum
enum class Architecture {
    x64,
    ARM64,
    Unknown
};

// Build configuration
enum class BuildConfiguration {
    Debug,
    Development,
    Profile,
    Shipping,
    Custom
};

// Build target type
enum class TargetType {
    Executable,
    SharedLibrary,
    StaticLibrary,
    Interface,
    Module
};

// Compiler type
enum class CompilerType {
    MSVC,
    GCC,
    Clang,
    AppleClang,
    Unknown
};

// Build status
enum class BuildStatus {
    Success,
    Failed,
    Skipped,
    Cached
};

// File path alias
using Path = std::filesystem::path;

// Time duration alias
using Duration = std::chrono::milliseconds;

// Timestamp alias
using Timestamp = std::chrono::steady_clock::time_point;

// String view for performance
using StringView = std::string_view;

// Function callback aliases
template<typename... Args>
using Callback = std::function<void(Args...)>;

// Result type for operations
template<typename T>
class Result {
public:
    static Result Success(T value) { return Result(std::move(value), true); }
    static Result Failure(std::string error) { return Result(std::move(error), false); }
    
    bool IsSuccess() const { return m_success; }
    bool IsFailure() const { return !m_success; }
    
    const T& GetValue() const { return m_value; }
    T& GetValue() { return m_value; }
    
    const std::string& GetError() const { return m_error; }
    
private:
    Result(T value, bool success) : m_value(std::move(value)), m_success(success) {}
    Result(std::string error, bool success) : m_error(std::move(error)), m_success(success) {}
    
    T m_value;
    std::string m_error;
    bool m_success;
};

// Version structure
struct Version {
    int major = 0;
    int minor = 0;
    int patch = 0;
    
    std::string ToString() const;
    static Version Parse(const std::string& versionString);
    
    bool operator==(const Version& other) const;
    bool operator<(const Version& other) const;
};

// Build statistics
struct BuildStatistics {
    int totalTargets = 0;
    int successfulTargets = 0;
    int failedTargets = 0;
    int skippedTargets = 0;
    int cachedTargets = 0;
    
    Duration totalBuildTime{0};
    Duration totalCompileTime{0};
    Duration totalLinkTime{0};
    
    int cacheHits = 0;
    int cacheMisses = 0;
    
    double GetCacheHitRate() const;
    void Print() const;
};

} // namespace IgniteBT
