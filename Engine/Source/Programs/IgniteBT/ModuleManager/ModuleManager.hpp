#pragma once

#include "Core/Types.hpp"
#include "BuildGraph/BuildGraph.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace IgniteBT {

// Module types
enum class ModuleType {
    Executable,     // Standalone executable
    StaticLibrary,  // Static library (.lib, .a)
    SharedLibrary,  // Shared library (.dll, .so, .dylib)
    HeaderOnly,     // Header-only library
    Interface,      // Interface/abstract module
    Plugin          // Loadable plugin
};

// Build configuration for a module
struct ModuleConfig {
    std::string name;
    ModuleType type;
    Path rootPath;
    Path sourceDir;
    Path includeDir;
    Path outputDir;
    
    std::vector<std::string> sources;
    std::vector<std::string> headers;
    std::vector<std::string> dependencies;  // Other modules this depends on
    std::vector<std::string> compileFlags;
    std::vector<std::string> linkFlags;
    std::vector<std::string> defines;
    
    std::string targetName;  // Output file name (without extension)
    
    bool operator==(const ModuleConfig& other) const {
        return name == other.name && rootPath == other.rootPath;
    }
};

// Module discovery result
struct ModuleDiscoveryResult {
    std::vector<ModuleConfig> modules;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

// Module manager for auto-discovery and management of build modules
class ModuleManager {
public:
    ModuleManager();
    ~ModuleManager();
    
    // Module discovery
    Result<ModuleDiscoveryResult> DiscoverModules(const Path& searchPath);
    Result<ModuleDiscoveryResult> DiscoverModulesRecursive(const Path& rootPath);
    
    // Module management
    bool AddModule(const ModuleConfig& config);
    bool RemoveModule(const std::string& name);
    ModuleConfig* GetModule(const std::string& name);
    const ModuleConfig* GetModule(const std::string& name) const;
    const std::unordered_map<std::string, ModuleConfig>& GetModules() const { return m_modules; }
    
    // Module queries
    std::vector<ModuleConfig*> GetModulesByType(ModuleType type);
    std::vector<ModuleConfig*> GetDependencyOrderedModules();
    Result<void> ValidateDependencies();
    
    // Build graph integration
    Result<BuildGraph> CreateBuildGraph();
    Result<BuildGraph> CreateBuildGraphForTargets(const std::vector<std::string>& targetNames);
    
    // Module configuration
    void SetSearchPaths(const std::vector<Path>& paths) { m_searchPaths = paths; }
    const std::vector<Path>& GetSearchPaths() const { return m_searchPaths; }
    
    // Module filtering
    void SetIncludePatterns(const std::vector<std::string>& patterns) { m_includePatterns = patterns; }
    void SetExcludePatterns(const std::vector<std::string>& patterns) { m_excludePatterns = patterns; }
    
    // Statistics
    size_t GetModuleCount() const { return m_modules.size(); }
    size_t GetSourceCount() const;
    size_t GetHeaderCount() const;
    
    // Clear all modules
    void Clear();
    
    // Callback for module discovery
    using DiscoveryCallback = std::function<void(const ModuleConfig&)>;
    void SetDiscoveryCallback(DiscoveryCallback callback) { m_discoveryCallback = callback; }
    
private:
    // Helper methods for discovery
    Result<ModuleConfig> DiscoverModuleInDirectory(const Path& dirPath);
    bool IsModuleDirectory(const Path& dirPath);
    ModuleConfig ParseModuleConfig(const Path& configPath);
    std::vector<Path> FindSourceFiles(const Path& dir, const std::vector<std::string>& extensions);
    std::vector<Path> FindHeaderFiles(const Path& dir, const std::vector<std::string>& extensions);
    
    // Helper for dependency ordering
    void VisitModuleForOrder(const std::string& moduleName, 
                            std::unordered_set<std::string>& visited,
                            std::unordered_set<std::string>& recursionStack,
                            std::vector<ModuleConfig*>& order,
                            bool& hasCycle);
    
    // Helper for build graph creation
    Result<void> AddModuleToBuildGraph(BuildGraph& graph, const ModuleConfig& module);
    
    std::unordered_map<std::string, ModuleConfig> m_modules;
    std::vector<Path> m_searchPaths;
    std::vector<std::string> m_includePatterns;
    std::vector<std::string> m_excludePatterns;
    DiscoveryCallback m_discoveryCallback;
};

// Utility functions for module management
namespace ModuleUtils {
    // Get default source file extensions for C++
    std::vector<std::string> GetDefaultSourceExtensions();
    
    // Get default header file extensions for C++
    std::vector<std::string> GetDefaultHeaderExtensions();
    
    // Detect module type from directory structure
    ModuleType DetectModuleType(const Path& dirPath);
    
    // Generate module name from directory path
    std::string GenerateModuleName(const Path& dirPath);
    
    // Validate module configuration
    Result<void> ValidateModuleConfig(const ModuleConfig& config);
    
    // Resolve relative paths in module config
    void ResolvePaths(ModuleConfig& config, const Path& basePath);
    
    // Get output filename for a module
    std::string GetOutputFilename(const ModuleConfig& config);
    
    // Get target name for a module
    std::string GetTargetName(const ModuleConfig& config);
    
    // Merge module configurations (for inheritance)
    ModuleConfig MergeModuleConfigs(const ModuleConfig& base, const ModuleConfig& override);
    
    // Check if a module is a dependency of another
    bool IsDependency(const ModuleConfig& module, const ModuleConfig& potentialDep);
    
    // Get all transitive dependencies of a module
    std::vector<std::string> GetTransitiveDependencies(const ModuleManager& manager, 
                                                       const std::string& moduleName);
}

} // namespace IgniteBT
