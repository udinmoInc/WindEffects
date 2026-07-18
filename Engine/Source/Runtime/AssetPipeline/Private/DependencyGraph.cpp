#include "AssetPipeline/DependencyGraph.h"

#include <functional>
#include <queue>

namespace we::runtime::assetpipeline {

void DependencyGraph::Clear() {
    std::lock_guard lock(m_Mutex);
    m_Dependencies.clear();
    m_Dependents.clear();
}

void DependencyGraph::SetDependencies(
    const we::runtime::assetimporter::AssetGuid& asset,
    const std::vector<we::runtime::assetimporter::AssetGuid>& dependencies) {
    std::lock_guard lock(m_Mutex);

    // Remove old reverse edges.
    auto oldIt = m_Dependencies.find(asset);
    if (oldIt != m_Dependencies.end()) {
        for (const auto& dep : oldIt->second) {
            auto depIt = m_Dependents.find(dep);
            if (depIt != m_Dependents.end()) {
                depIt->second.erase(asset);
            }
        }
    }

    auto& deps = m_Dependencies[asset];
    deps.clear();
    for (const auto& dep : dependencies) {
        if (dep.IsNil() || dep == asset) {
            continue;
        }
        deps.insert(dep);
        m_Dependents[dep].insert(asset);
    }
}

void DependencyGraph::RemoveAsset(const we::runtime::assetimporter::AssetGuid& asset) {
    std::lock_guard lock(m_Mutex);
    auto it = m_Dependencies.find(asset);
    if (it != m_Dependencies.end()) {
        for (const auto& dep : it->second) {
            auto depIt = m_Dependents.find(dep);
            if (depIt != m_Dependents.end()) {
                depIt->second.erase(asset);
            }
        }
        m_Dependencies.erase(it);
    }
    auto rev = m_Dependents.find(asset);
    if (rev != m_Dependents.end()) {
        for (const auto& dependent : rev->second) {
            auto depIt = m_Dependencies.find(dependent);
            if (depIt != m_Dependencies.end()) {
                depIt->second.erase(asset);
            }
        }
        m_Dependents.erase(rev);
    }
}

std::vector<we::runtime::assetimporter::AssetGuid> DependencyGraph::GetDependencies(
    const we::runtime::assetimporter::AssetGuid& asset) const {
    std::lock_guard lock(m_Mutex);
    std::vector<we::runtime::assetimporter::AssetGuid> out;
    const auto it = m_Dependencies.find(asset);
    if (it == m_Dependencies.end()) {
        return out;
    }
    out.assign(it->second.begin(), it->second.end());
    return out;
}

std::vector<we::runtime::assetimporter::AssetGuid> DependencyGraph::GetDependents(
    const we::runtime::assetimporter::AssetGuid& asset) const {
    std::lock_guard lock(m_Mutex);
    std::vector<we::runtime::assetimporter::AssetGuid> out;
    const auto it = m_Dependents.find(asset);
    if (it == m_Dependents.end()) {
        return out;
    }
    out.assign(it->second.begin(), it->second.end());
    return out;
}

std::vector<we::runtime::assetimporter::AssetGuid> DependencyGraph::CollectRebuildClosure(
    const we::runtime::assetimporter::AssetGuid& changed) const {
    std::lock_guard lock(m_Mutex);
    std::vector<we::runtime::assetimporter::AssetGuid> order;
    std::unordered_set<we::runtime::assetimporter::AssetGuid, we::runtime::assetimporter::AssetGuidHash> visited;
    std::queue<we::runtime::assetimporter::AssetGuid> q;
    q.push(changed);
    visited.insert(changed);
    while (!q.empty()) {
        const auto current = q.front();
        q.pop();
        order.push_back(current);
        const auto it = m_Dependents.find(current);
        if (it == m_Dependents.end()) {
            continue;
        }
        for (const auto& dependent : it->second) {
            if (visited.insert(dependent).second) {
                q.push(dependent);
            }
        }
    }
    return order;
}

bool DependencyGraph::HasCycles() const {
    std::lock_guard lock(m_Mutex);
    std::unordered_map<
        we::runtime::assetimporter::AssetGuid,
        int,
        we::runtime::assetimporter::AssetGuidHash>
        state; // 0=unseen, 1=visiting, 2=done

    std::function<bool(const we::runtime::assetimporter::AssetGuid&)> dfs =
        [&](const we::runtime::assetimporter::AssetGuid& node) -> bool {
        state[node] = 1;
        const auto it = m_Dependencies.find(node);
        if (it != m_Dependencies.end()) {
            for (const auto& dep : it->second) {
                const int s = state[dep];
                if (s == 1) {
                    return true;
                }
                if (s == 0 && dfs(dep)) {
                    return true;
                }
            }
        }
        state[node] = 2;
        return false;
    };

    for (const auto& [guid, _] : m_Dependencies) {
        if (state[guid] == 0 && dfs(guid)) {
            return true;
        }
    }
    return false;
}

size_t DependencyGraph::AssetCount() const {
    std::lock_guard lock(m_Mutex);
    return m_Dependencies.size();
}

} // namespace we::runtime::assetpipeline
