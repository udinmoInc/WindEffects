#include "AssetPipeline/BuildCache.h"

#include "AssetImporter/AssetPackage.h"

#include <fstream>
#include <sstream>

#if WE_HAS_NLOHMANN_JSON
#include <nlohmann/json.h>
#endif

namespace we::runtime::assetpipeline {
namespace {

uint64_t HashCombine(std::string_view text) {
    uint64_t hash = 1469598103934665603ull;
    for (unsigned char c : text) {
        hash ^= c;
        hash *= 1099511628211ull;
    }
    return hash;
}

std::string ToHex(uint64_t value) {
    std::ostringstream oss;
    oss << std::hex << value;
    return oss.str();
}

} // namespace

BuildCache::BuildCache(std::filesystem::path cacheRoot)
    : m_Root(std::move(cacheRoot)) {}

bool BuildCache::Load() {
    std::lock_guard lock(m_Mutex);
    m_ByGuid.clear();
    m_ByKey.clear();

    const auto path = m_Root / "build_cache.json";
    if (!std::filesystem::exists(path)) {
        return true;
    }

#if WE_HAS_NLOHMANN_JSON
    try {
        std::ifstream in(path);
        if (!in) {
            return false;
        }
        nlohmann::json root;
        in >> root;
        if (!root.contains("entries") || !root["entries"].is_array()) {
            return true;
        }
        for (const auto& j : root["entries"]) {
            BuildCacheEntry e{};
            e.cacheKey = j.value("cacheKey", "");
            if (auto g = we::runtime::assetimporter::AssetGuid::Parse(j.value("guid", ""))) {
                e.guid = *g;
            }
            e.sourcePath = j.value("sourcePath", "");
            e.sourceHash = j.value("sourceHash", "");
            e.sourceTimestamp = j.value("sourceTimestamp", 0ull);
            e.importerId = j.value("importerId", "");
            e.importerVersion = j.value("importerVersion", "");
            e.processorFingerprint = j.value("processorFingerprint", "");
            e.settingsHash = j.value("settingsHash", "");
            e.cookedPath = j.value("cookedPath", "");
            e.cookedHash = j.value("cookedHash", "");
            e.engineVersion = j.value("engineVersion", "");
            e.platformTarget = j.value("platformTarget", "");
            if (!e.guid.IsNil() && !e.cacheKey.empty()) {
                m_ByGuid[e.guid] = e;
                m_ByKey[e.cacheKey] = e.guid;
            }
        }
        return true;
    } catch (...) {
        return false;
    }
#else
    (void)path;
    return true;
#endif
}

bool BuildCache::Save() const {
    std::lock_guard lock(m_Mutex);
    std::error_code ec;
    std::filesystem::create_directories(m_Root, ec);

#if WE_HAS_NLOHMANN_JSON
    nlohmann::json root;
    root["version"] = 1;
    auto& arr = root["entries"] = nlohmann::json::array();
    for (const auto& [_, e] : m_ByGuid) {
        arr.push_back({
            {"cacheKey", e.cacheKey},
            {"guid", e.guid.ToString()},
            {"sourcePath", e.sourcePath},
            {"sourceHash", e.sourceHash},
            {"sourceTimestamp", e.sourceTimestamp},
            {"importerId", e.importerId},
            {"importerVersion", e.importerVersion},
            {"processorFingerprint", e.processorFingerprint},
            {"settingsHash", e.settingsHash},
            {"cookedPath", e.cookedPath},
            {"cookedHash", e.cookedHash},
            {"engineVersion", e.engineVersion},
            {"platformTarget", e.platformTarget},
        });
    }
    const auto path = m_Root / "build_cache.json";
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        return false;
    }
    out << root.dump(2);
    return static_cast<bool>(out);
#else
    return true;
#endif
}

void BuildCache::Clear() {
    std::lock_guard lock(m_Mutex);
    m_ByGuid.clear();
    m_ByKey.clear();
}

void BuildCache::Upsert(BuildCacheEntry entry) {
    std::lock_guard lock(m_Mutex);
    if (entry.guid.IsNil() || entry.cacheKey.empty()) {
        return;
    }
    // Drop stale key mapping if guid reused with new key.
    const auto existing = m_ByGuid.find(entry.guid);
    if (existing != m_ByGuid.end() && existing->second.cacheKey != entry.cacheKey) {
        m_ByKey.erase(existing->second.cacheKey);
    }
    m_ByKey[entry.cacheKey] = entry.guid;
    m_ByGuid[entry.guid] = std::move(entry);
}

void BuildCache::Remove(const we::runtime::assetimporter::AssetGuid& guid) {
    std::lock_guard lock(m_Mutex);
    const auto it = m_ByGuid.find(guid);
    if (it == m_ByGuid.end()) {
        return;
    }
    m_ByKey.erase(it->second.cacheKey);
    m_ByGuid.erase(it);
}

std::optional<BuildCacheEntry> BuildCache::Find(
    const we::runtime::assetimporter::AssetGuid& guid) const {
    std::lock_guard lock(m_Mutex);
    const auto it = m_ByGuid.find(guid);
    if (it == m_ByGuid.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<BuildCacheEntry> BuildCache::FindByCacheKey(std::string_view cacheKey) const {
    std::lock_guard lock(m_Mutex);
    const auto keyIt = m_ByKey.find(std::string(cacheKey));
    if (keyIt == m_ByKey.end()) {
        return std::nullopt;
    }
    const auto it = m_ByGuid.find(keyIt->second);
    if (it == m_ByGuid.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::string BuildCache::MakeCacheKey(
    std::string_view sourceHash,
    uint64_t sourceTimestamp,
    std::string_view importerId,
    std::string_view importerVersion,
    std::string_view processorFingerprint,
    std::string_view settingsHash,
    std::string_view engineVersion,
    std::string_view platformTarget) {
    std::ostringstream oss;
    oss << sourceHash << '|' << sourceTimestamp << '|' << importerId << '@' << importerVersion
        << '|' << processorFingerprint << '|' << settingsHash << '|' << engineVersion
        << '|' << platformTarget;
    return ToHex(HashCombine(oss.str()));
}

size_t BuildCache::Size() const {
    std::lock_guard lock(m_Mutex);
    return m_ByGuid.size();
}

} // namespace we::runtime::assetpipeline
