#include "AssetImporter/AssetMetadata.h"

#include "Core/Logger.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace we::runtime::assetimporter {
namespace {

std::string NowUtc() {
    using clock = std::chrono::system_clock;
    const auto now = clock::now();
    const std::time_t t = clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

} // namespace

bool AssetMetadata::IsValid() const noexcept {
    return !guid.IsNil() && kind != AssetKind::Unknown && !importerId.empty();
}

bool SaveMetadataJson(const std::filesystem::path& path, const AssetMetadata& metadata) {
#if !WE_HAS_NLOHMANN_JSON
    (void)path;
    (void)metadata;
    return false;
#else
    nlohmann::json root;
    root["schemaVersion"] = metadata.schemaVersion;
    root["guid"] = metadata.guid.ToString();
    root["kind"] = std::string(AssetKindToString(metadata.kind));
    root["displayName"] = metadata.displayName;
    root["sourcePath"] = metadata.sourcePath;
    root["sourceHash"] = metadata.sourceHash;
    root["importerId"] = metadata.importerId;
    root["importerVersion"] = metadata.importerVersion;
    root["engineVersion"] = metadata.engineVersion;
    root["createdUtc"] = metadata.createdUtc.empty() ? NowUtc() : metadata.createdUtc;
    root["modifiedUtc"] = NowUtc();
    root["tags"] = metadata.tags;
    root["importSettings"] = metadata.importSettings;
    root["custom"] = metadata.custom;

    nlohmann::json deps = nlohmann::json::array();
    for (const auto& dep : metadata.dependencies) {
        deps.push_back(dep.ToString());
    }
    root["dependencies"] = deps;

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out(path);
    if (!out.is_open()) {
        return false;
    }
    out << root.dump(2);
    return static_cast<bool>(out);
#endif
}

std::optional<AssetMetadata> LoadMetadataJson(const std::filesystem::path& path) {
#if !WE_HAS_NLOHMANN_JSON
    (void)path;
    return std::nullopt;
#else
    std::ifstream in(path);
    if (!in.is_open()) {
        return std::nullopt;
    }
    try {
        nlohmann::json root;
        in >> root;
        AssetMetadata metadata{};
        metadata.schemaVersion = root.value("schemaVersion", 1);
        if (const auto guid = AssetGuid::Parse(root.value("guid", std::string{}))) {
            metadata.guid = *guid;
        }
        metadata.kind = AssetKindFromString(root.value("kind", std::string{}));
        metadata.displayName = root.value("displayName", std::string{});
        metadata.sourcePath = root.value("sourcePath", std::string{});
        metadata.sourceHash = root.value("sourceHash", std::string{});
        metadata.importerId = root.value("importerId", std::string{});
        metadata.importerVersion = root.value("importerVersion", std::string{ "1.0.0" });
        metadata.engineVersion = root.value("engineVersion", std::string{});
        metadata.createdUtc = root.value("createdUtc", std::string{});
        metadata.modifiedUtc = root.value("modifiedUtc", std::string{});
        metadata.tags = root.value("tags", std::vector<std::string>{});
        metadata.importSettings = root.value("importSettings", nlohmann::json::object());
        metadata.custom = root.value("custom", nlohmann::json::object());
        if (root.contains("dependencies") && root["dependencies"].is_array()) {
            for (const auto& item : root["dependencies"]) {
                if (auto dep = AssetGuid::Parse(item.get<std::string>())) {
                    metadata.dependencies.push_back(*dep);
                }
            }
        }
        return metadata;
    } catch (const std::exception& e) {
        HE_ERROR(std::string("[AssetImporter] Failed to parse metadata: ") + e.what());
        return std::nullopt;
    }
#endif
}

} // namespace we::runtime::assetimporter
