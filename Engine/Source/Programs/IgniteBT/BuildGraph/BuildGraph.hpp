#pragma once

#include "Core/Types.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>

namespace IgniteBT {

// Forward declarations
class BuildNode;
class BuildGraph;

// Node types in the build graph
enum class NodeType {
    Source,         // Source file (e.g., .cpp, .h)
    Object,         // Compiled object file
    Executable,     // Final executable
    Library,        // Static or shared library
    Custom,         // Custom build step
    Phony          // Virtual target (no actual file)
};

// Build node representing a file or target in the dependency graph
class BuildNode {
public:
    BuildNode(const std::string& id, NodeType type, const Path& path);
    
    const std::string& GetId() const { return m_id; }
    NodeType GetType() const { return m_type; }
    const Path& GetPath() const { return m_path; }
    
    void AddDependency(BuildNode* node);
    void RemoveDependency(BuildNode* node);
    const std::vector<BuildNode*>& GetDependencies() const { return m_dependencies; }
    
    void AddDependent(BuildNode* node);
    void RemoveDependent(BuildNode* node);
    const std::vector<BuildNode*>& GetDependents() const { return m_dependents; }
    
    bool IsDirty() const { return m_isDirty; }
    void SetDirty(bool dirty) { m_isDirty = dirty; }
    
    Timestamp GetLastBuildTime() const { return m_lastBuildTime; }
    void SetLastBuildTime(Timestamp time) { m_lastBuildTime = time; }
    
    const std::string& GetCommand() const { return m_command; }
    void SetCommand(const std::string& command) { m_command = command; }
    
    // Check if this node needs to be rebuilt
    bool NeedsRebuild() const;
    
    // Get the hash of this node's inputs (for cache invalidation)
    uint64_t GetInputHash() const { return m_inputHash; }
    void SetInputHash(uint64_t hash) { m_inputHash = hash; }
    
private:
    std::string m_id;
    NodeType m_type;
    Path m_path;
    
    std::vector<BuildNode*> m_dependencies;  // Nodes this node depends on
    std::vector<BuildNode*> m_dependents;   // Nodes that depend on this node
    
    bool m_isDirty;
    Timestamp m_lastBuildTime;
    std::string m_command;
    uint64_t m_inputHash;
};

// Build graph managing all nodes and dependencies
class BuildGraph {
public:
    BuildGraph();
    ~BuildGraph();
    
    // Disable copy, enable move
    BuildGraph(const BuildGraph&) = delete;
    BuildGraph& operator=(const BuildGraph&) = delete;
    BuildGraph(BuildGraph&&) = default;
    BuildGraph& operator=(BuildGraph&&) = default;
    
    // Node management
    BuildNode* AddNode(const std::string& id, NodeType type, const Path& path);
    bool RemoveNode(const std::string& id);
    BuildNode* GetNode(const std::string& id);
    const BuildNode* GetNode(const std::string& id) const;
    const std::unordered_map<std::string, std::unique_ptr<BuildNode>>& GetNodes() const { return m_nodes; }
    
    // Dependency management
    bool AddDependency(const std::string& nodeId, const std::string& depId);
    bool RemoveDependency(const std::string& nodeId, const std::string& depId);
    
    // Graph analysis
    std::vector<BuildNode*> GetTopologicalOrder();
    Result<void> ValidateGraph();  // Check for cycles
    
    // Cycle detection helper (public for BuildGraphUtils)
    void DFSVisit(BuildNode* node, std::unordered_set<BuildNode*>& visited, 
                   std::unordered_set<BuildNode*>& recursionStack, 
                   std::vector<BuildNode*>& order, bool& hasCycle);
    
    // Dirty propagation
    void MarkDirty(const std::string& nodeId);
    void MarkAllDirty();
    void CleanDirtyFlags();
    
    // Find all nodes that need rebuilding
    std::vector<BuildNode*> GetDirtyNodes();
    
    // Get leaf nodes (nodes with no dependents)
    std::vector<BuildNode*> GetLeafNodes();
    
    // Get root nodes (nodes with no dependencies)
    std::vector<BuildNode*> GetRootNodes();
    
    // Clear the graph
    void Clear();
    
    // Statistics
    size_t GetNodeCount() const { return m_nodes.size(); }
    size_t GetEdgeCount() const;
    
private:
    // Helper for dirty propagation
    void PropagateDirty(BuildNode* node);
    
    std::unordered_map<std::string, std::unique_ptr<BuildNode>> m_nodes;
};

// Build graph visitor for traversing the graph
class BuildGraphVisitor {
public:
    virtual ~BuildGraphVisitor() = default;
    
    virtual bool VisitNode(BuildNode* node) = 0;
    virtual bool VisitEdge(BuildNode* from, BuildNode* to) = 0;
};

// Utility functions for build graph operations
namespace BuildGraphUtils {
    // Find all source files in the graph
    std::vector<BuildNode*> GetSourceNodes(BuildGraph& graph);
    
    // Find all object files in the graph
    std::vector<BuildNode*> GetObjectNodes(BuildGraph& graph);
    
    // Find all executables in the graph
    std::vector<BuildNode*> GetExecutableNodes(BuildGraph& graph);
    
    // Calculate the critical path (longest dependency chain)
    std::vector<BuildNode*> GetCriticalPath(BuildGraph& graph);
    
    // Detect circular dependencies
    Result<std::vector<std::string>> DetectCycles(BuildGraph& graph);
    
    // Create a subgraph containing only the nodes needed to build the given targets
    Result<BuildGraph> CreateSubgraph(BuildGraph& graph, const std::vector<std::string>& targetIds);
    
    // Serialize graph to DOT format for visualization
    std::string ToDotFormat(const BuildGraph& graph);
}

} // namespace IgniteBT
