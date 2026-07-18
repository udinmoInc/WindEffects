#pragma once

#include "AssetImporter/AssetGuid.h"
#include "AssetPipeline/Export.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace we::runtime::assetpipeline {

/// Directed dependency graph: edge A → B means A depends on B (B must cook first).
class ASSETPIPELINE_API DependencyGraph {
public:
    void Clear();
    void SetDependencies(
        const we::runtime::assetimporter::AssetGuid& asset,
        const std::vector<we::runtime::assetimporter::AssetGuid>& dependencies);
    void RemoveAsset(const we::runtime::assetimporter::AssetGuid& asset);

    [[nodiscard]] std::vector<we::runtime::assetimporter::AssetGuid> GetDependencies(
        const we::runtime::assetimporter::AssetGuid& asset) const;
    [[nodiscard]] std::vector<we::runtime::assetimporter::AssetGuid> GetDependents(
        const we::runtime::assetimporter::AssetGuid& asset) const;

    /// Transitive dependents including `asset` itself (rebuild order: leaf → root).
    [[nodiscard]] std::vector<we::runtime::assetimporter::AssetGuid> CollectRebuildClosure(
        const we::runtime::assetimporter::AssetGuid& changed) const;

    [[nodiscard]] bool HasCycles() const;
    [[nodiscard]] size_t AssetCount() const;

private:
    mutable std::mutex m_Mutex;
    std::unordered_map<
        we::runtime::assetimporter::AssetGuid,
        std::unordered_set<we::runtime::assetimporter::AssetGuid, we::runtime::assetimporter::AssetGuidHash>,
        we::runtime::assetimporter::AssetGuidHash>
        m_Dependencies;
    std::unordered_map<
        we::runtime::assetimporter::AssetGuid,
        std::unordered_set<we::runtime::assetimporter::AssetGuid, we::runtime::assetimporter::AssetGuidHash>,
        we::runtime::assetimporter::AssetGuidHash>
        m_Dependents;
};

} // namespace we::runtime::assetpipeline
