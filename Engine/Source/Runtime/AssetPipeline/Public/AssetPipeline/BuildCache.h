#pragma once

#include "AssetImporter/AssetGuid.h"
#include "AssetPipeline/Export.h"

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace we::runtime::assetpipeline {

struct ASSETPIPELINE_API BuildCacheEntry {
    std::string cacheKey;
    we::runtime::assetimporter::AssetGuid guid{};
    std::string sourcePath;
    std::string sourceHash;
    uint64_t sourceTimestamp = 0;
    std::string importerId;
    std::string importerVersion;
    std::string processorFingerprint;
    std::string settingsHash;
    std::string cookedPath;
    std::string cookedHash;
    std::string engineVersion;
    std::string platformTarget;
};

/// Persistent incremental build cache under Intermediate/AssetPipeline/Cache.
class ASSETPIPELINE_API BuildCache {
public:
    explicit BuildCache(std::filesystem::path cacheRoot);

    [[nodiscard]] const std::filesystem::path& GetRoot() const noexcept { return m_Root; }

    [[nodiscard]] bool Load();
    [[nodiscard]] bool Save() const;

    void Clear();
    void Upsert(BuildCacheEntry entry);
    void Remove(const we::runtime::assetimporter::AssetGuid& guid);

    [[nodiscard]] std::optional<BuildCacheEntry> Find(
        const we::runtime::assetimporter::AssetGuid& guid) const;
    [[nodiscard]] std::optional<BuildCacheEntry> FindByCacheKey(std::string_view cacheKey) const;

    [[nodiscard]] static std::string MakeCacheKey(
        std::string_view sourceHash,
        uint64_t sourceTimestamp,
        std::string_view importerId,
        std::string_view importerVersion,
        std::string_view processorFingerprint,
        std::string_view settingsHash,
        std::string_view engineVersion,
        std::string_view platformTarget);

    [[nodiscard]] size_t Size() const;

private:
    std::filesystem::path m_Root;
    mutable std::mutex m_Mutex;
    std::unordered_map<
        we::runtime::assetimporter::AssetGuid,
        BuildCacheEntry,
        we::runtime::assetimporter::AssetGuidHash>
        m_ByGuid;
    std::unordered_map<std::string, we::runtime::assetimporter::AssetGuid> m_ByKey;
};

} // namespace we::runtime::assetpipeline
