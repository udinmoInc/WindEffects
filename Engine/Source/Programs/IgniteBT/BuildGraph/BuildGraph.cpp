#include "BuildGraph/BuildGraph.hpp"
#include "Platform/Platform.hpp"
#include "Logging/Logger.hpp"
#include <algorithm>
#include <queue>
#include <sstream>
#include <fstream>

namespace IgniteBT {

// BuildNode implementation
BuildNode::BuildNode(const std::string& id, NodeType type, const Path& path)
    : m_id(id)
    , m_type(type)
    , m_path(path)
    , m_isDirty(false)
    , m_lastBuildTime()
    , m_inputHash(0) {
}

void BuildNode::AddDependency(BuildNode* node) {
    if (node && std::find(m_dependencies.begin(), m_dependencies.end(), node) == m_dependencies.end()) {
        m_dependencies.push_back(node);
        node->AddDependent(this);
    }
}

void BuildNode::RemoveDependency(BuildNode* node) {
    auto it = std::find(m_dependencies.begin(), m_dependencies.end(), node);
    if (it != m_dependencies.end()) {
        m_dependencies.erase(it);
        node->RemoveDependent(this);
    }
}

void BuildNode::AddDependent(BuildNode* node) {
    if (node && std::find(m_dependents.begin(), m_dependents.end(), node) == m_dependents.end()) {
        m_dependents.push_back(node);
    }
}

void BuildNode::RemoveDependent(BuildNode* node) {
    auto it = std::find(m_dependents.begin(), m_dependents.end(), node);
    if (it != m_dependents.end()) {
        m_dependents.erase(it);
    }
}

bool BuildNode::NeedsRebuild() const {
    if (m_isDirty) {
        return true;
    }
    
    // Check if any dependency is newer than this node
    for (BuildNode* dep : m_dependencies) {
        if (dep->GetLastBuildTime() > m_lastBuildTime) {
            return true;
        }
    }
    
    // For source files, check if the file has been modified
    if (m_type == NodeType::Source && !m_path.empty()) {
        IPlatform* platform = PlatformManager::Get().GetPlatform();
        if (platform && platform->FileExists(m_path)) {
            Timestamp fileTime = platform->GetFileModificationTime(m_path);
            if (fileTime > m_lastBuildTime) {
                return true;
            }
        }
    }
    
    return false;
}

// BuildGraph implementation
BuildGraph::BuildGraph() {
}

BuildGraph::~BuildGraph() {
    Clear();
}

BuildNode* BuildGraph::AddNode(const std::string& id, NodeType type, const Path& path) {
    if (m_nodes.find(id) != m_nodes.end()) {
        Logger::Get().Warning("BuildGraph: Node with id '{}' already exists", id);
        return m_nodes[id].get();
    }
    
    auto node = std::make_unique<BuildNode>(id, type, path);
    BuildNode* nodePtr = node.get();
    m_nodes[id] = std::move(node);
    
    Logger::Get().Debug("BuildGraph: Added node '{}' of type " + std::to_string(static_cast<int>(type)), id);
    return nodePtr;
}

bool BuildGraph::RemoveNode(const std::string& id) {
    auto it = m_nodes.find(id);
    if (it == m_nodes.end()) {
        return false;
    }
    
    BuildNode* node = it->second.get();
    
    // Remove this node from all dependencies
    for (BuildNode* dep : node->GetDependencies()) {
        dep->RemoveDependent(node);
    }
    
    // Remove this node from all dependents
    for (BuildNode* dependent : node->GetDependents()) {
        dependent->RemoveDependency(node);
    }
    
    m_nodes.erase(it);
    Logger::Get().Debug("BuildGraph: Removed node '{}'", id);
    return true;
}

BuildNode* BuildGraph::GetNode(const std::string& id) {
    auto it = m_nodes.find(id);
    return it != m_nodes.end() ? it->second.get() : nullptr;
}

const BuildNode* BuildGraph::GetNode(const std::string& id) const {
    auto it = m_nodes.find(id);
    return it != m_nodes.end() ? it->second.get() : nullptr;
}

bool BuildGraph::AddDependency(const std::string& nodeId, const std::string& depId) {
    BuildNode* node = GetNode(nodeId);
    BuildNode* dep = GetNode(depId);
    
    if (!node || !dep) {
        Logger::Get().Error("BuildGraph: Cannot add dependency - node '" + nodeId + "' or '" + depId + "' not found");
        return false;
    }
    
    node->AddDependency(dep);
    Logger::Get().Debug("BuildGraph: Added dependency '" + nodeId + "' -> '" + depId + "'");
    return true;
}

bool BuildGraph::RemoveDependency(const std::string& nodeId, const std::string& depId) {
    BuildNode* node = GetNode(nodeId);
    BuildNode* dep = GetNode(depId);
    
    if (!node || !dep) {
        return false;
    }
    
    node->RemoveDependency(dep);
        Logger::Get().Debug("BuildGraph: Removed dependency '" + nodeId + "' -> '" + depId + "'");
    return true;
}

std::vector<BuildNode*> BuildGraph::GetTopologicalOrder() {
    std::vector<BuildNode*> order;
    std::unordered_set<BuildNode*> visited;
    std::unordered_set<BuildNode*> recursionStack;
    bool hasCycle = false;
    
    // Perform DFS on all nodes
    for (auto& [id, node] : m_nodes) {
        if (visited.find(node.get()) == visited.end()) {
            DFSVisit(node.get(), visited, recursionStack, order, hasCycle);
        }
    }
    
    if (hasCycle) {
        Logger::Get().Error("BuildGraph: Cycle detected in dependency graph");
        order.clear();
    } else {
        // Reverse to get topological order (dependencies before dependents)
        std::reverse(order.begin(), order.end());
    }
    
    return order;
}

void BuildGraph::DFSVisit(BuildNode* node, std::unordered_set<BuildNode*>& visited,
                          std::unordered_set<BuildNode*>& recursionStack,
                          std::vector<BuildNode*>& order, bool& hasCycle) {
    visited.insert(node);
    recursionStack.insert(node);
    
    for (BuildNode* dep : node->GetDependencies()) {
        if (visited.find(dep) == visited.end()) {
            DFSVisit(dep, visited, recursionStack, order, hasCycle);
        } else if (recursionStack.find(dep) != recursionStack.end()) {
            hasCycle = true;
            Logger::Get().Error("BuildGraph: Cycle detected involving nodes '" + node->GetId() + "' and '" + dep->GetId() + "'");
        }
    }
    
    recursionStack.erase(node);
    order.push_back(node);
}

Result<void> BuildGraph::ValidateGraph() {
    std::unordered_set<BuildNode*> visited;
    std::unordered_set<BuildNode*> recursionStack;
    std::vector<BuildNode*> order;
    bool hasCycle = false;
    
    for (auto& [id, node] : m_nodes) {
        if (visited.find(node.get()) == visited.end()) {
            DFSVisit(node.get(), visited, recursionStack, order, hasCycle);
        }
    }
    
    if (hasCycle) {
        return Result<void>::Failure("Circular dependency detected in build graph");
    }
    
    return Result<void>::Success();
}

void BuildGraph::MarkDirty(const std::string& nodeId) {
    BuildNode* node = GetNode(nodeId);
    if (node) {
        node->SetDirty(true);
        PropagateDirty(node);
    }
}

void BuildGraph::MarkAllDirty() {
    for (auto& [id, node] : m_nodes) {
        node->SetDirty(true);
    }
}

void BuildGraph::CleanDirtyFlags() {
    for (auto& [id, node] : m_nodes) {
        node->SetDirty(false);
    }
}

void BuildGraph::PropagateDirty(BuildNode* node) {
    // Mark all dependents as dirty
    for (BuildNode* dependent : node->GetDependents()) {
        if (!dependent->IsDirty()) {
            dependent->SetDirty(true);
            PropagateDirty(dependent);
        }
    }
}

std::vector<BuildNode*> BuildGraph::GetDirtyNodes() {
    std::vector<BuildNode*> dirtyNodes;
    
    for (auto& [id, node] : m_nodes) {
        if (node->NeedsRebuild()) {
            dirtyNodes.push_back(node.get());
        }
    }
    
    return dirtyNodes;
}

std::vector<BuildNode*> BuildGraph::GetLeafNodes() {
    std::vector<BuildNode*> leafNodes;
    
    for (auto& [id, node] : m_nodes) {
        if (node->GetDependents().empty()) {
            leafNodes.push_back(node.get());
        }
    }
    
    return leafNodes;
}

std::vector<BuildNode*> BuildGraph::GetRootNodes() {
    std::vector<BuildNode*> rootNodes;
    
    for (auto& [id, node] : m_nodes) {
        if (node->GetDependencies().empty()) {
            rootNodes.push_back(node.get());
        }
    }
    
    return rootNodes;
}

void BuildGraph::Clear() {
    m_nodes.clear();
}

size_t BuildGraph::GetEdgeCount() const {
    size_t count = 0;
    for (const auto& [id, node] : m_nodes) {
        count += node->GetDependencies().size();
    }
    return count;
}

// BuildGraphUtils implementation
namespace BuildGraphUtils {

std::vector<BuildNode*> GetSourceNodes(BuildGraph& graph) {
    std::vector<BuildNode*> sourceNodes;
    
    for (auto& [id, node] : graph.GetNodes()) {
        if (node->GetType() == NodeType::Source) {
            sourceNodes.push_back(node.get());
        }
    }
    
    return sourceNodes;
}

std::vector<BuildNode*> GetObjectNodes(BuildGraph& graph) {
    std::vector<BuildNode*> objectNodes;
    
    for (auto& [id, node] : graph.GetNodes()) {
        if (node->GetType() == NodeType::Object) {
            objectNodes.push_back(node.get());
        }
    }
    
    return objectNodes;
}

std::vector<BuildNode*> GetExecutableNodes(BuildGraph& graph) {
    std::vector<BuildNode*> executableNodes;
    
    for (auto& [id, node] : graph.GetNodes()) {
        if (node->GetType() == NodeType::Executable) {
            executableNodes.push_back(node.get());
        }
    }
    
    return executableNodes;
}

std::vector<BuildNode*> GetCriticalPath(BuildGraph& graph) {
    std::vector<BuildNode*> criticalPath;
    
    // Find the longest path from any source to any leaf
    std::unordered_map<BuildNode*, size_t> depths;
    std::queue<BuildNode*> queue;
    
    // Initialize depths for root nodes
    auto rootNodes = graph.GetRootNodes();
    for (BuildNode* root : rootNodes) {
        depths[root] = 0;
        queue.push(root);
    }
    
    // BFS to calculate depths
    while (!queue.empty()) {
        BuildNode* current = queue.front();
        queue.pop();
        
        for (BuildNode* dependent : current->GetDependents()) {
            size_t newDepth = depths[current] + 1;
            if (depths.find(dependent) == depths.end() || newDepth > depths[dependent]) {
                depths[dependent] = newDepth;
                queue.push(dependent);
            }
        }
    }
    
    // Find the node with maximum depth
    BuildNode* maxDepthNode = nullptr;
    size_t maxDepth = 0;
    for (const auto& [node, depth] : depths) {
        if (depth > maxDepth) {
            maxDepth = depth;
            maxDepthNode = node;
        }
    }
    
    // Backtrack to find the critical path
    if (maxDepthNode) {
        criticalPath.push_back(maxDepthNode);
        BuildNode* current = maxDepthNode;
        
        while (!current->GetDependencies().empty()) {
            BuildNode* next = nullptr;
            size_t maxDepDepth = 0;
            
            for (BuildNode* dep : current->GetDependencies()) {
                if (depths.find(dep) != depths.end() && depths[dep] > maxDepDepth) {
                    maxDepDepth = depths[dep];
                    next = dep;
                }
            }
            
            if (next) {
                criticalPath.push_back(next);
                current = next;
            } else {
                break;
            }
        }
        
        std::reverse(criticalPath.begin(), criticalPath.end());
    }
    
    return criticalPath;
}

Result<std::vector<std::string>> DetectCycles(BuildGraph& graph) {
    std::vector<std::string> cycleNodes;
    
    std::unordered_set<BuildNode*> visited;
    std::unordered_set<BuildNode*> recursionStack;
    std::vector<BuildNode*> order;
    bool hasCycle = false;
    
    for (auto& [id, node] : graph.GetNodes()) {
        if (visited.find(node.get()) == visited.end()) {
            graph.DFSVisit(node.get(), visited, recursionStack, order, hasCycle);
        }
    }
    
    if (hasCycle) {
        // Collect nodes in the recursion stack (part of the cycle)
        for (BuildNode* node : recursionStack) {
            cycleNodes.push_back(node->GetId());
        }
        return Result<std::vector<std::string>>::Success(cycleNodes);
    }
    
    return Result<std::vector<std::string>>::Failure("No cycles detected");
}

Result<BuildGraph> CreateSubgraph(BuildGraph& graph, const std::vector<std::string>& targetIds) {
    BuildGraph subgraph;
    std::unordered_set<std::string> targetSet(targetIds.begin(), targetIds.end());
    std::unordered_set<BuildNode*> visited;
    std::queue<BuildNode*> queue;
    
    // Add target nodes to the queue
    for (const std::string& targetId : targetIds) {
        BuildNode* node = graph.GetNode(targetId);
        if (node) {
            queue.push(node);
        } else {
            return Result<BuildGraph>::Failure("Target node '" + targetId + "' not found in graph");
        }
    }
    
    // BFS to collect all dependencies
    while (!queue.empty()) {
        BuildNode* current = queue.front();
        queue.pop();
        
        if (visited.find(current) != visited.end()) {
            continue;
        }
        
        visited.insert(current);
        
        // Add node to subgraph
        subgraph.AddNode(current->GetId(), current->GetType(), current->GetPath());
        subgraph.GetNode(current->GetId())->SetCommand(current->GetCommand());
        subgraph.GetNode(current->GetId())->SetLastBuildTime(current->GetLastBuildTime());
        subgraph.GetNode(current->GetId())->SetInputHash(current->GetInputHash());
        
        // Add dependencies to queue
        for (BuildNode* dep : current->GetDependencies()) {
            queue.push(dep);
        }
    }
    
    // Reconstruct dependencies in subgraph
    for (BuildNode* node : visited) {
        BuildNode* subNode = subgraph.GetNode(node->GetId());
        if (subNode) {
            for (BuildNode* dep : node->GetDependencies()) {
                BuildNode* subDep = subgraph.GetNode(dep->GetId());
                if (subDep) {
                    subgraph.AddDependency(node->GetId(), dep->GetId());
                }
            }
        }
    }
    
    return Result<BuildGraph>::Success(std::move(subgraph));
}

std::string ToDotFormat(const BuildGraph& graph) {
    std::stringstream ss;
    
    ss << "digraph BuildGraph {\n";
    ss << "  rankdir=LR;\n";
    ss << "  node [shape=box];\n\n";
    
    // Define nodes with colors based on type
    for (const auto& [id, node] : graph.GetNodes()) {
        std::string color;
        switch (node->GetType()) {
            case NodeType::Source:
                color = "lightblue";
                break;
            case NodeType::Object:
                color = "lightgreen";
                break;
            case NodeType::Executable:
                color = "lightcoral";
                break;
            case NodeType::Library:
                color = "lightyellow";
                break;
            case NodeType::Custom:
                color = "lightgray";
                break;
            case NodeType::Phony:
                color = "white";
                break;
        }
        
        ss << "  \"" << id << "\" [label=\"" << id << "\\n" 
           << node->GetPath().string() << "\", fillcolor=" << color << ", style=filled];\n";
    }
    
    ss << "\n";
    
    // Define edges
    for (const auto& [id, node] : graph.GetNodes()) {
        for (BuildNode* dep : node->GetDependencies()) {
            ss << "  \"" << dep->GetId() << "\" -> \"" << id << "\";\n";
        }
    }
    
    ss << "}\n";
    
    return ss.str();
}

} // namespace BuildGraphUtils

} // namespace IgniteBT
