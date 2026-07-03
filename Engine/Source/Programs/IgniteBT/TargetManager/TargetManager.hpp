#pragma once

#include "Core/Types.hpp"
#include "BuildGraph/BuildGraph.hpp"
#include "ModuleManager/ModuleManager.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace IgniteBT {

// Target types
enum class TargetType {
    Executable,     // Executable target
    Library,        // Library target (static or shared)
    Object,        // Object file target
    Custom,        // Custom build target
    Phony,         // Virtual target (no actual output)
    Alias          // Alias for another target
};

// Build configuration for a target
enum class BuildConfig {
    Debug,
    Release,
    RelWithDebInfo,
    MinSizeRel
};

// Target configuration
struct TargetConfig {
    std::string name;
    TargetType type;
    BuildConfig config;
    
    Path outputDir;
    Path intermediateDir;
    
    std::vector<std::string> sources;
    std::vector<std::string> headers;
    std::vector<std::string> dependencies;  // Other targets this depends on
    std::vector<std::string> compileFlags;
    std::vector<std::string> linkFlags;
    std::vector<std::string> defines;
    std::vector<std::string> includeDirs;
    std::vector<std::string> libraryDirs;
    std::vector<std::string> libraries;
    
    std::string command;  // Custom command for custom targets
    std::string aliasFor;  // For alias targets
    
    bool operator==(const TargetConfig& other) const {
        return name == other.name;
    }
};

// Build result for a target
struct BuildResult {
    std::string targetName;
    bool success;
    std::string output;
    std::string errors;
    double buildTime;  // in seconds
    Timestamp startTime;
    Timestamp endTime;
};

// Target manager for managing build targets
class TargetManager {
public:
    TargetManager();
    ~TargetManager();
    
    // Target management
    bool AddTarget(const TargetConfig& config);
    bool RemoveTarget(const std::string& name);
    TargetConfig* GetTarget(const std::string& name);
    const TargetConfig* GetTarget(const std::string& name) const;
    const std::unordered_map<std::string, TargetConfig>& GetTargets() const { return m_targets; }
    
    // Target queries
    std::vector<TargetConfig*> GetTargetsByType(TargetType type);
    std::vector<TargetConfig*> GetTargetsByConfig(BuildConfig config);
    std::vector<TargetConfig*> GetDependencyOrderedTargets();
    Result<void> ValidateDependencies();
    
    // Build operations
    Result<BuildResult> BuildTarget(const std::string& targetName);
    Result<std::vector<BuildResult>> BuildTargets(const std::vector<std::string>& targetNames);
    Result<std::vector<BuildResult>> BuildAll();
    
    // Clean operations
    bool CleanTarget(const std::string& targetName);
    bool CleanTargets(const std::vector<std::string>& targetNames);
    bool CleanAll();
    
    // Rebuild operations
    Result<BuildResult> RebuildTarget(const std::string& targetName);
    Result<std::vector<BuildResult>> RebuildTargets(const std::vector<std::string>& targetNames);
    Result<std::vector<BuildResult>> RebuildAll();
    
    // Target configuration
    void SetDefaultBuildConfig(BuildConfig config) { m_defaultConfig = config; }
    BuildConfig GetDefaultBuildConfig() const { return m_defaultConfig; }
    
    void SetOutputDirectory(const Path& dir) { m_outputDir = dir; }
    const Path& GetOutputDirectory() const { return m_outputDir; }
    
    void SetIntermediateDirectory(const Path& dir) { m_intermediateDir = dir; }
    const Path& GetIntermediateDirectory() const { return m_intermediateDir; }
    
    // Statistics
    size_t GetTargetCount() const { return m_targets.size(); }
    size_t GetSourceCount() const;
    size_t GetHeaderCount() const;
    
    // Build history
    const std::vector<BuildResult>& GetBuildHistory() const { return m_buildHistory; }
    void ClearBuildHistory() { m_buildHistory.clear(); }
    
    // Clear all targets
    void Clear();
    
    // Callback for build progress
    using BuildProgressCallback = std::function<void(const std::string&, double)>;
    void SetBuildProgressCallback(BuildProgressCallback callback) { m_buildProgressCallback = callback; }
    
    // Callback for build completion
    using BuildCompleteCallback = std::function<void(const BuildResult&)>;
    void SetBuildCompleteCallback(BuildCompleteCallback callback) { m_buildCompleteCallback = callback; }
    
private:
    // Helper methods
    Result<void> ValidateTargetConfig(const TargetConfig& config);
    Path GetTargetOutputPath(const TargetConfig& config);
    Path GetTargetIntermediatePath(const TargetConfig& config);
    Result<void) PrepareBuildDirectories(const TargetConfig& config);
    
    // Helper for dependency ordering
    void VisitTargetForOrder(const std::string& targetName,
                            std::unordered_set<std::string>& visited,
                            std::unordered_set<std::string>& recursionStack,
                            std::vector<TargetConfig*>& order,
                            bool& hasCycle);
    
    // Build execution helpers
    Result<BuildResult> ExecuteBuild(const TargetConfig& config);
    Result<BuildResult> BuildExecutableTarget(const TargetConfig& config);
    Result<BuildResult> BuildLibraryTarget(const TargetConfig& config);
    Result<BuildResult> BuildObjectTarget(const TargetConfig& config);
    Result<BuildResult> BuildCustomTarget(const TargetConfig& config);
    
    std::unordered_map<std::string, TargetConfig> m_targets;
    BuildConfig m_defaultConfig;
    Path m_outputDir;
    Path m_intermediateDir;
    std::vector<BuildResult> m_buildHistory;
    BuildProgressCallback m_buildProgressCallback;
    BuildCompleteCallback m_buildCompleteCallback;
};

// Utility functions for target management
namespace TargetUtils {
    // Get default output directory for a target
    Path GetDefaultOutputDirectory(const TargetConfig& config);
    
    // Get default intermediate directory for a target
    Path GetDefaultIntermediateDirectory(const TargetConfig& config);
    
    // Get output filename for a target
    std::string GetOutputFilename(const TargetConfig& config);
    
    // Detect target type from configuration
    TargetType DetectTargetType(const TargetConfig& config);
    
    // Validate target configuration
    Result<void> ValidateTargetConfig(const TargetConfig& config);
    
    // Resolve relative paths in target config
    void ResolvePaths(TargetConfig& config, const Path& basePath);
    
    // Merge target configurations (for inheritance)
    TargetConfig MergeTargetConfigs(const TargetConfig& base, const TargetConfig& override);
    
    // Check if a target is a dependency of another
    bool IsDependency(const TargetConfig& target, const TargetConfig& potentialDep);
    
    // Get all transitive dependencies of a target
    std::vector<std::string> GetTransitiveDependencies(const TargetManager& manager,
                                                       const std::string& targetName);
    
    // Convert build config to string
    std::string BuildConfigToString(BuildConfig config);
    
    // Parse build config from string
    Result<BuildConfig> ParseBuildConfig(const std::string& config);
    
    // Get compile flags for a build config
    std::vector<std::string> GetDefaultCompileFlags(BuildConfig config);
    
    // Get link flags for a build config
    std::vector<std::string> GetDefaultLinkFlags(BuildConfig config);
}

} // namespace IgniteBT
