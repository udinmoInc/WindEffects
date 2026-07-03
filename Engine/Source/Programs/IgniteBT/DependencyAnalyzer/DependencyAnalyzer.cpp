#include "DependencyAnalyzer/DependencyAnalyzer.hpp"
#include "Platform/Platform.hpp"
#include "Logging/Logger.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <queue>

namespace IgniteBT {

// DependencyAnalyzer implementation
DependencyAnalyzer::DependencyAnalyzer()
    : m_hashAlgorithm("xxhash")
    , m_enableHashChecking(true)
    , m_enableTimestampChecking(true) {
}

DependencyAnalyzer::~DependencyAnalyzer() {
    ClearAllDependencies();
}

Result<DependencyAnalysisResult> DependencyAnalyzer::AnalyzeTarget(const std::string& targetName, BuildGraph& graph) {
    DependencyAnalysisResult result;
    result.targetName = targetName;
    result.needsRebuild = false;
    
    BuildNode* targetNode = graph.GetNode(targetName);
    if (!targetNode) {
        return Result<DependencyAnalysisResult>::Failure("Target '" + targetName + "' not found in build graph");
    }
    
    Logger::Get().Info("DependencyAnalyzer: Analyzing dependencies for target '" + targetName + "'");
    
    // Get all dependencies of the target
    std::vector<BuildNode*> dependencies = targetNode->GetDependencies();
    
    // Check each dependency for changes
    for (BuildNode* depNode : dependencies) {
        Path depPath = depNode->GetPath();
        std::string depPathStr = depPath.string();
        
        // Get current file info
        auto currentInfoResult = GetFileInfo(depPath);
        if (!currentInfoResult.IsSuccess()) {
            Logger::Get().Warning("DependencyAnalyzer: Could not get info for dependency '" + depPathStr + "'");
            continue;
        }
        
        DependencyFileInfo currentInfo = currentInfoResult.GetValue();
        
        // Check if we have previous info for this dependency
        bool hasPreviousInfo = false;
        DependencyFileInfo previousInfo;
        
        auto targetIt = m_targetDependencies.find(targetName);
        if (targetIt != m_targetDependencies.end()) {
            auto depIt = targetIt->second.find(depPathStr);
            if (depIt != targetIt->second.end()) {
                hasPreviousInfo = true;
                previousInfo = depIt->second;
            }
        }
        
        // Track the current dependency info
        TrackDependency(targetName, depPath);
        
        // Check for changes
        if (hasPreviousInfo) {
            if (HasFileChanged(previousInfo, currentInfo)) {
                DependencyChange change = CreateDependencyChange(previousInfo, currentInfo);
                result.changes.push_back(change);
                result.needsRebuild = true;
                result.reason = "Dependency '" + depPathStr + "' has changed";
                
                if (m_dependencyChangeCallback) {
                    m_dependencyChangeCallback(change);
                }
            }
        } else {
            // New dependency - treat as a change
            DependencyChange change;
            change.filePath = depPathStr;
            change.changeType = DependencyChangeType::Added;
            change.newTimestamp = currentInfo.timestamp;
            change.newHash = currentInfo.hash;
            
            result.changes.push_back(change);
            result.needsRebuild = true;
            result.reason = "New dependency '" + depPathStr + "' added";
            
            if (m_dependencyChangeCallback) {
                m_dependencyChangeCallback(change);
            }
        }
    }
    
    // Check for removed dependencies
    auto targetIt = m_targetDependencies.find(targetName);
    if (targetIt != m_targetDependencies.end()) {
        for (const auto& [depPathStr, depInfo] : targetIt->second) {
            bool stillExists = false;
            for (BuildNode* depNode : dependencies) {
                if (depNode->GetPath().string() == depPathStr) {
                    stillExists = true;
                    break;
                }
            }
            
            if (!stillExists) {
                DependencyChange change;
                change.filePath = depPathStr;
                change.changeType = DependencyChangeType::Removed;
                change.oldTimestamp = depInfo.timestamp;
                change.oldHash = depInfo.hash;
                
                result.changes.push_back(change);
                result.needsRebuild = true;
                result.reason = "Dependency '" + depPathStr + "' removed";
                
                if (m_dependencyChangeCallback) {
                    m_dependencyChangeCallback(change);
                }
            }
        }
    }
    
    // Find affected targets
    if (result.needsRebuild) {
        std::unordered_set<std::string> affectedTargets;
        for (const DependencyChange& change : result.changes) {
            FindAffectedTargets(change.filePath, graph, affectedTargets);
        }
        result.affectedTargets.assign(affectedTargets.begin(), affectedTargets.end());
    }
    
    Logger::Get().Info("DependencyAnalyzer: Analysis complete for target '" + targetName + "', needs rebuild: " + 
                       (result.needsRebuild ? "yes" : "no"));
    
    return Result<DependencyAnalysisResult>::Success(result);
}

Result<std::vector<DependencyAnalysisResult>> DependencyAnalyzer::AnalyzeAllTargets(BuildGraph& graph) {
    std::vector<DependencyAnalysisResult> results;
    
    for (const auto& [id, node] : graph.GetNodes()) {
        if (node->GetType() == NodeType::Executable || node->GetType() == NodeType::Library) {
            auto result = AnalyzeTarget(id, graph);
            if (result.IsSuccess()) {
                results.push_back(result.GetValue());
            }
        }
    }
    
    return Result<std::vector<DependencyAnalysisResult>>::Success(results);
}

Result<std::vector<DependencyAnalysisResult>> DependencyAnalyzer::AnalyzeTargets(const std::vector<std::string>& targetNames, BuildGraph& graph) {
    std::vector<DependencyAnalysisResult> results;
    
    for (const std::string& targetName : targetNames) {
        auto result = AnalyzeTarget(targetName, graph);
        if (result.IsSuccess()) {
            results.push_back(result.GetValue());
        }
    }
    
    return Result<std::vector<DependencyAnalysisResult>>::Success(results);
}

void DependencyAnalyzer::TrackDependency(const std::string& targetName, const Path& filePath) {
    auto infoResult = GetFileInfo(filePath);
    if (infoResult.IsSuccess()) {
        m_targetDependencies[targetName][filePath.string()] = infoResult.GetValue();
    }
}

void DependencyAnalyzer::RemoveDependencyTracking(const std::string& targetName, const Path& filePath) {
    auto targetIt = m_targetDependencies.find(targetName);
    if (targetIt != m_targetDependencies.end()) {
        targetIt->second.erase(filePath.string());
    }
}

void DependencyAnalyzer::ClearTargetDependencies(const std::string& targetName) {
    m_targetDependencies.erase(targetName);
}

void DependencyAnalyzer::ClearAllDependencies() {
    m_targetDependencies.clear();
}

uint64_t DependencyAnalyzer::CalculateFileHash(const Path& filePath) {
    if (m_hashAlgorithm == "xxhash") {
        return CalculateXXHash(filePath);
    } else {
        return CalculateSimpleHash(filePath);
    }
}

uint64_t DependencyAnalyzer::CalculateContentHash(const std::string& content) {
    // Simple hash calculation for content
    uint64_t hash = 5381;
    for (char c : content) {
        hash = ((hash << 5) + hash) + static_cast<uint64_t>(c);
    }
    return hash;
}

Result<void> DependencyAnalyzer::SaveDependencyState(const Path& filePath) {
    // TODO: Implement JSON serialization
    Logger::Get().Warning("DependencyAnalyzer: SaveDependencyState not yet implemented");
    return Result<void>::Success();
}

Result<void> DependencyAnalyzer::LoadDependencyState(const Path& filePath) {
    // TODO: Implement JSON deserialization
    Logger::Get().Warning("DependencyAnalyzer: LoadDependencyState not yet implemented");
    return Result<void>::Success();
}

size_t DependencyAnalyzer::GetTrackedDependencyCount() const {
    size_t count = 0;
    for (const auto& [targetName, deps] : m_targetDependencies) {
        count += deps.size();
    }
    return count;
}

size_t DependencyAnalyzer::GetTargetCount() const {
    return m_targetDependencies.size();
}

Result<DependencyFileInfo> DependencyAnalyzer::GetFileInfo(const Path& filePath) {
    IPlatform* platform = PlatformManager::Get().GetPlatform();
    if (!platform) {
        return Result<DependencyFileInfo>::Failure("Platform not available");
    }
    
    if (!platform->FileExists(filePath)) {
        return Result<DependencyFileInfo>::Failure("File does not exist: " + filePath.string());
    }
    
    DependencyFileInfo info;
    info.filePath = filePath;
    info.timestamp = platform->GetFileModificationTime(filePath);
    info.fileSize = platform->GetFileSize(filePath);
    
    if (m_enableHashChecking) {
        info.hash = CalculateFileHash(filePath);
    } else {
        info.hash = 0;
    }
    
    return Result<DependencyFileInfo>::Success(info);
}

bool DependencyAnalyzer::HasFileChanged(const DependencyFileInfo& oldInfo, const DependencyFileInfo& newInfo) {
    if (m_enableTimestampChecking) {
        if (oldInfo.timestamp != newInfo.timestamp) {
            return true;
        }
    }
    
    if (m_enableHashChecking) {
        if (oldInfo.hash != newInfo.hash) {
            return true;
        }
    }
    
    if (oldInfo.fileSize != newInfo.fileSize) {
        return true;
    }
    
    return false;
}

DependencyChange DependencyAnalyzer::CreateDependencyChange(const DependencyFileInfo& oldInfo, const DependencyFileInfo& newInfo) {
    DependencyChange change;
    change.filePath = oldInfo.filePath.string();
    change.oldTimestamp = oldInfo.timestamp;
    change.newTimestamp = newInfo.timestamp;
    change.oldHash = oldInfo.hash;
    change.newHash = newInfo.hash;
    
    if (oldInfo.hash != newInfo.hash) {
        change.changeType = DependencyChangeType::Modified;
    } else if (oldInfo.timestamp != newInfo.timestamp) {
        change.changeType = DependencyChangeType::TimestampChanged;
    } else {
        change.changeType = DependencyChangeType::Modified;
    }
    
    return change;
}

void DependencyAnalyzer::FindAffectedTargets(const std::string& changedFile, BuildGraph& graph, std::unordered_set<std::string>& affectedTargets) {
    // Find all targets that depend on this文件
    for (const auto& [id, node] : graph.GetNodes()) {
        for (BuildNode* dep : node->GetDependencies()) {
            if (dep->GetPath().string() == changedFile) {
                affectedTargets.insert(id);
                // Recursively find targets that depend on this target
                FindAffectedTargets(id, graph, affectedTargets);
            }
        }
    }
}

uint64_t DependencyAnalyzer::CalculateXXHash(const Path& filePath) {
    // TODO: Implement actual XXHash calculation
    // For now, use simple hash as placeholder
    return CalculateSimpleHash(filePath);
}

uint64_t DependencyAnalyzer::CalculateSimpleHash(const Path& filePath) {
    IPlatform* platform = PlatformManager::Get().GetPlatform();
    if (!platform) {
        return 0;
    }
    
    uint64_t hash = 5381;
    hash = ((hash << 5) + hash) + static_cast<uint64_t>(platform->GetFileSize(filePath));
    
    std::string pathStr = filePath.string();
    for (char c : pathStr) {
        hash = ((hash << 5) + hash) + static_cast<uint64_t>(c);
    }
    
    return hash;
}

// DependencyUtils implementation
namespace DependencyUtils {

std::vector<Path> GetAllDependencies(BuildGraph& graph, const std::string& targetName) {
    std::vector<Path> dependencies;
    std::unordered_set<BuildNode*> visited;
    std::queue<BuildNode*> queue;
    
    BuildNode* targetNode = graph.GetNode(targetName);
    if (!targetNode) {
        return dependencies;
    }
    
    queue.push(targetNode);
    
    while (!queue.empty()) {
        BuildNode* current = queue.front();
        queue.pop();
        
        if (visited.find(current) != visited.end()) {
            continue;
        }
        
        visited.insert(current);
        
        for (BuildNode* dep : current->GetDependencies()) {
            dependencies.push_back(dep->GetPath());
            queue.push(dep);
        }
    }
    
    return dependencies;
}

std::vector<Path> GetDirectDependencies(BuildGraph& graph, const std::string& targetName) {
    std::vector<Path> dependencies;
    
    BuildNode* targetNode = graph.GetNode(targetName);
    if (!targetNode) {
        return dependencies;
    }
    
    for (BuildNode* dep : targetNode->GetDependencies()) {
        dependencies.push_back(dep->GetPath());
    }
    
    return dependencies;
}

bool IsDependencyOfFile(BuildGraph& graph, const std::string& targetName, const Path& filePath) {
    auto allDeps = GetAllDependencies(graph, targetName);
    for (const Path& dep : allDeps) {
        if (dep == filePath) {
            return true;
        }
    }
    return false;
}

std::vector<Path> GetCriticalPath(BuildGraph& graph, const std::string& targetName) {
    // TODO: Implement critical path calculation
    return GetAllDependencies(graph, targetName);
}

size_t CalculateDependencyDepth(BuildGraph& graph, const std::string& targetName) {
    std::unordered_map<BuildNode*, size_t> depths;
    std::queue<BuildNode*> queue;
    
    BuildNode* targetNode = graph.GetNode(targetName);
    if (!targetNode) {
        return 0;
    }
    
    depths[targetNode] = 0;
    queue.push(targetNode);
    
    size_t maxDepth = 0;
    
    while (!queue.empty()) {
        BuildNode* current = queue.front();
        queue.pop();
        
        size_t currentDepth = depths[current];
        maxDepth = std::max(maxDepth, currentDepth);
        
        for (BuildNode* dep : current->GetDependencies()) {
            if (depths.find(dep) == depths.end()) {
                depths[dep] = currentDepth + 1;
                queue.push(dep);
            }
        }
    }
    
    return maxDepth;
}

Result<std::vector<std::string>> DetectCircularDependencies(BuildGraph& graph) {
    return BuildGraphUtils::DetectCycles(graph);
}

Result<DependencyUtils::DependencyStatistics> DependencyUtils::CalculateDependencyStatistics(BuildGraph& graph, const std::string& targetName) {
    DependencyStatistics stats;
    
    auto directDeps = GetDirectDependencies(graph, targetName);
    auto allDeps = GetAllDependencies(graph, targetName);
    
    stats.directDependencies = directDeps.size();
    stats.transitiveDependencies = allDeps.size() - directDeps.size();
    stats.totalDependencies = allDeps.size();
    stats.maxDepth = CalculateDependencyDepth(graph, targetName);
    
    // Calculate average depth
    std::unordered_map<BuildNode*, size_t> depths;
    std::queue<BuildNode*> queue;
    
    BuildNode* targetNode = graph.GetNode(targetName);
    if (!targetNode) {
        return Result<DependencyStatistics>::Failure("Target not found");
    }
    
    depths[targetNode] = 0;
    queue.push(targetNode);
    
    size_t totalDepth = 0;
    size_t nodeCount = 0;
    
    while (!queue.empty()) {
        BuildNode* current = queue.front();
        queue.pop();
        
        size_t currentDepth = depths[current];
        totalDepth += currentDepth;
        nodeCount++;
        
        for (BuildNode* dep : current->GetDependencies()) {
            if (depths.find(dep) == depths.end()) {
                depths[dep] = currentDepth + 1;
                queue.push(dep);
            }
        }
    }
    
    stats.averageDepth = nodeCount > 0 ? static_cast<double>(totalDepth) / nodeCount : 0.0;
    
    return Result<DependencyStatistics>::Success(stats);
}

std::string DependencyUtils::SerializeDependencies(const std::unordered_map<std::string, DependencyFileInfo>& dependencies) {
    // TODO: Implement JSON serialization
    return "{}";
}

Result<std::unordered_map<std::string, DependencyFileInfo>> DependencyUtils::DeserializeDependencies(const std::string& json) {
    // TODO: Implement JSON deserialization
    return Result<std::unordered_map<std::string, DependencyFileInfo>>::Success({});
}

std::vector<DependencyChange> DependencyUtils::CompareDependencySets(
    const std::unordered_map<std::string, DependencyFileInfo>& oldDeps,
    const std::unordered_map<std::string, DependencyFileInfo>& newDeps) {
    
    std::vector<DependencyChange> changes;
    
    // Check for added and modified dependencies
    for (const auto& [path, newInfo] : newDeps) {
        auto oldIt = oldDeps.find(path);
        if (oldIt == oldDeps.end()) {
            // Added
            DependencyChange change;
            change.filePath = path;
            change.changeType = DependencyChangeType::Added;
            change.newTimestamp = newInfo.timestamp;
            change.newHash = newInfo.hash;
            changes.push_back(change);
        } else {
            // Check for modification
            const DependencyFileInfo& oldInfo = oldIt->second;
            if (oldInfo.timestamp != newInfo.timestamp || oldInfo.hash != newInfo.hash) {
                DependencyChange change;
                change.filePath = path;
                change.changeType = DependencyChangeType::Modified;
                change.oldTimestamp = oldInfo.timestamp;
                change.newTimestamp = newInfo.timestamp;
                change.oldHash = oldInfo.hash;
                change.newHash = newInfo.hash;
                changes.push_back(change);
            }
        }
    }
    
    // Check for removed dependencies
    for (const auto& [path, oldInfo] : oldDeps) {
        if (newDeps.find(path) == newDeps.end()) {
            // Removed
            DependencyChange change;
            change.filePath = path;
            change.changeType = DependencyChangeType::Removed;
            change.oldTimestamp = oldInfo.timestamp;
            change.oldHash = oldInfo.hash;
            changes.push_back(change);
        }
    }
    
    return changes;
}

} // namespace DependencyUtils

} // namespace IgniteBT
