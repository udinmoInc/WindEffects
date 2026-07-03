#pragma once

#include "Core/Types.hpp"
#include "BuildGraph/BuildGraph.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace IgniteBT {

// Dependency change type
enum class DependencyChangeType {
    Added,
    Removed,
    Modified,
    TimestampChanged
};

// Information about a dependency change
struct DependencyChange {
    std::string filePath;
    DependencyChangeType changeType;
    Timestamp oldTimestamp;
    Timestamp newTimestamp;
    uint64_t oldHash;
    uint64_t newHash;
};

// Dependency analysis result
struct DependencyAnalysisResult {
    std::string targetName;
    bool needsRebuild;
    std::vector<DependencyChange> changes;
    std::vector<std::string> affectedTargets;
    std::string reason;
};

// Dependency file information
struct DependencyFileInfo {
    Path filePath;
    Timestamp timestamp;
    uint64_t hash;
    size_t fileSize;
};

// Dependency analyzer for detecting rebuild requirements
class DependencyAnalyzer {
public:
    DependencyAnalyzer();
    ~DependencyAnalyzer();
    
    // Analysis operations
    Result<DependencyAnalysisResult> AnalyzeTarget(const std::string& targetName, BuildGraph& graph);
    Result<std::vector<DependencyAnalysisResult>> AnalyzeAllTargets(BuildGraph& graph);
    Result<std::vector<DependencyAnalysisResult>> AnalyzeTargets(const std::vector<std::string>& targetNames, BuildGraph& graph);
    
    // Dependency tracking
    void TrackDependency(const std::string& targetName, const Path& filePath);
    void RemoveDependencyTracking(const std::string& targetName, const Path& filePath);
    void ClearTargetDependencies(const std::string& targetName);
    void ClearAllDependencies();
    
    // Hash calculation
    uint64_t CalculateFileHash(const Path& filePath);
    uint64_t CalculateContentHash(const std::string& content);
    
    // Dependency state persistence
    Result<void> SaveDependencyState(const Path& filePath);
    Result<void> LoadDependencyState(const Path& filePath);
    
    // Configuration
    void SetHashAlgorithm(const std::string& algorithm) { m_hashAlgorithm = algorithm; }
    const std::string& GetHashAlgorithm() const { return m_hashAlgorithm; }
    
    void EnableHashChecking(bool enable) { m_enableHashChecking = enable; }
    bool IsHashCheckingEnabled() const { return m_enableHashChecking; }
    
    void EnableTimestampChecking(bool enable) { m_enableTimestampChecking = enable; }
    bool IsTimestampCheckingEnabled() const { return m_enableTimestampChecking; }
    
    // Statistics
    size_t GetTrackedDependencyCount() const;
    size_t GetTargetCount() const;
    
    // Callback for dependency changes
    using DependencyChangeCallback = std::function<void(const DependencyChange&)>;
    void SetDependencyChangeCallback(DependencyChangeCallback callback) { m_dependencyChangeCallback = callback; }
    
private:
    // Helper methods
    Result<DependencyFileInfo> GetFileInfo(const Path& filePath);
    bool HasFileChanged(const DependencyFileInfo& oldInfo, const DependencyFileInfo& newInfo);
    DependencyChange CreateDependencyChange(const DependencyFileInfo& oldInfo, const DependencyFileInfo& newInfo);
    
    // Transitive dependency analysis
    void FindAffectedTargets(const std::string& changedFile, BuildGraph& graph, std::unordered_set<std::string>& affectedTargets);
    
    // Hash calculation helpers
    uint64_t CalculateXXHash(const Path& filePath);
    uint64_t CalculateSimpleHash(const Path& filePath);
    
    std::unordered_map<std::string, std::unordered_map<std::string, DependencyFileInfo>> m_targetDependencies;
    std::string m_hashAlgorithm;
    bool m_enableHashChecking;
    bool m_enableTimestampChecking;
    DependencyChangeCallback m_dependencyChangeCallback;
};

// Utility functions for dependency analysis
namespace DependencyUtils {
    // Get all dependencies of a target (transitive)
    std::vector<Path> GetAllDependencies(BuildGraph& graph, const std::string& targetName);
    
    // Get direct dependencies of a target
    std::vector<Path> GetDirectDependencies(BuildGraph& graph, const std::string& targetName);
    
    // Check if a file is a dependency of a target
    bool IsDependencyOfFile(BuildGraph& graph, const std::string& targetName, const Path& filePath);
    
    // Get the critical path of dependencies for a target
    std::vector<Path> GetCriticalPath(BuildGraph& graph, const std::string& targetName);
    
    // Calculate dependency depth for a target
    size_t CalculateDependencyDepth(BuildGraph& graph, const std::string& targetName);
    
    // Detect circular dependencies
    Result<std::vector<std::string>> DetectCircularDependencies(BuildGraph& graph);
    
    // Get dependency statistics
    struct DependencyStatistics {
        size_t totalDependencies;
        size_t directDependencies;
        size_t transitiveDependencies;
        size_t maxDepth;
        double averageDepth;
    };
    Result<DependencyStatistics> CalculateDependencyStatistics(BuildGraph& graph, const std::string& targetName);
    
    // Serialize dependency information to JSON
    std::string SerializeDependencies(const std::unordered_map<std::string, DependencyFileInfo>& dependencies);
    
    // Deserialize dependency information from JSON
    Result<std::unordered_map<std::string, DependencyFileInfo>> DeserializeDependencies(const std::string& json);
    
    // Compare two dependency sets
    std::vector<DependencyChange> CompareDependencySets(
        const std::unordered_map<std::string, DependencyFileInfo>& oldDeps,
        const std::unordered_map<std::string, DependencyFileInfo>& newDeps);
}

} // namespace IgniteBT
