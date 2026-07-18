#include "AssetImporter/AssetPackage.h"

#include "AssetImporter/Types.h"
#include "Core/Logger.h"

#include <fstream>
#include <sstream>

#if WE_HAS_NLOHMANN_JSON
#include <nlohmann/json.h>
#endif

namespace we::runtime::assetimporter {
namespace {

std::string ToHex64(uint64_t value) {
    std::ostringstream oss;
    oss << std::hex << value;
    return oss.str();
}

} // namespace

std::string ComputeFileHashHex(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return {};
    }
    std::vector<uint8_t> buffer(static_cast<size_t>(1) << 16);
    uint64_t hash = 1469598103934665603ull;
    while (in) {
        in.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
        const auto got = static_cast<size_t>(in.gcount());
        if (got == 0) {
            break;
        }
        for (size_t i = 0; i < got; ++i) {
            hash ^= buffer[i];
            hash *= 1099511628211ull;
        }
    }
    return ToHex64(hash);
}

bool WriteAssetPackage(const std::filesystem::path& path, const AssetPackage& package) {
#if !WE_HAS_NLOHMANN_JSON
    (void)path;
    (void)package;
    return false;
#else
    nlohmann::json meta;
    meta["schemaVersion"] = package.metadata.schemaVersion;
    meta["guid"] = package.metadata.guid.ToString();
    meta["kind"] = std::string(AssetKindToString(package.metadata.kind));
    meta["displayName"] = package.metadata.displayName;
    meta["sourcePath"] = package.metadata.sourcePath;
    meta["sourceHash"] = package.metadata.sourceHash;
    meta["importerId"] = package.metadata.importerId;
    meta["importerVersion"] = package.metadata.importerVersion;
    meta["engineVersion"] = package.metadata.engineVersion;
    meta["createdUtc"] = package.metadata.createdUtc;
    meta["modifiedUtc"] = package.metadata.modifiedUtc;
    meta["tags"] = package.metadata.tags;
    meta["importSettings"] = package.metadata.importSettings;
    meta["custom"] = package.metadata.custom;
    meta["payloadContentType"] = package.payloadContentType;
    nlohmann::json deps = nlohmann::json::array();
    for (const auto& dep : package.metadata.dependencies) {
        deps.push_back(dep.ToString());
    }
    meta["dependencies"] = deps;

    const std::string metaJson = meta.dump();
    const uint32_t metaSize = static_cast<uint32_t>(metaJson.size());
    const uint64_t payloadSize = static_cast<uint64_t>(package.payload.size());

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        return false;
    }

    const uint32_t magic = AssetPackage::kMagic;
    const uint16_t version = AssetPackage::kVersion;
    const uint16_t flags = 0;
    out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    out.write(reinterpret_cast<const char*>(&version), sizeof(version));
    out.write(reinterpret_cast<const char*>(&flags), sizeof(flags));
    out.write(reinterpret_cast<const char*>(&metaSize), sizeof(metaSize));
    out.write(reinterpret_cast<const char*>(&payloadSize), sizeof(payloadSize));
    out.write(metaJson.data(), static_cast<std::streamsize>(metaJson.size()));
    if (!package.payload.empty()) {
        out.write(reinterpret_cast<const char*>(package.payload.data()),
            static_cast<std::streamsize>(package.payload.size()));
    }
    return static_cast<bool>(out);
#endif
}

std::optional<AssetPackage> ReadAssetPackage(const std::filesystem::path& path) {
#if !WE_HAS_NLOHMANN_JSON
    (void)path;
    return std::nullopt;
#else
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return std::nullopt;
    }

    uint32_t magic = 0;
    uint16_t version = 0;
    uint16_t flags = 0;
    uint32_t metaSize = 0;
    uint64_t payloadSize = 0;
    in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    in.read(reinterpret_cast<char*>(&flags), sizeof(flags));
    in.read(reinterpret_cast<char*>(&metaSize), sizeof(metaSize));
    in.read(reinterpret_cast<char*>(&payloadSize), sizeof(payloadSize));
    (void)flags;

    if (!in || magic != AssetPackage::kMagic || version == 0 || version > AssetPackage::kVersion) {
        return std::nullopt;
    }
    if (metaSize > (1u << 24)) {
        return std::nullopt;
    }

    std::string metaJson(metaSize, '\0');
    in.read(metaJson.data(), static_cast<std::streamsize>(metaSize));
    if (!in) {
        return std::nullopt;
    }

    AssetPackage package{};
    try {
        const nlohmann::json root = nlohmann::json::parse(metaJson);
        package.metadata.schemaVersion = root.value("schemaVersion", 1);
        if (const auto guid = AssetGuid::Parse(root.value("guid", std::string{}))) {
            package.metadata.guid = *guid;
        }
        package.metadata.kind = AssetKindFromString(root.value("kind", std::string{}));
        package.metadata.displayName = root.value("displayName", std::string{});
        package.metadata.sourcePath = root.value("sourcePath", std::string{});
        package.metadata.sourceHash = root.value("sourceHash", std::string{});
        package.metadata.importerId = root.value("importerId", std::string{});
        package.metadata.importerVersion = root.value("importerVersion", std::string{ "1.0.0" });
        package.metadata.engineVersion = root.value("engineVersion", std::string{});
        package.metadata.createdUtc = root.value("createdUtc", std::string{});
        package.metadata.modifiedUtc = root.value("modifiedUtc", std::string{});
        package.metadata.tags = root.value("tags", std::vector<std::string>{});
        package.metadata.importSettings = root.value("importSettings", nlohmann::json::object());
        package.metadata.custom = root.value("custom", nlohmann::json::object());
        package.payloadContentType = root.value("payloadContentType", std::string{});
    } catch (const std::exception& e) {
        HE_ERROR(std::string("[AssetImporter] Invalid .weasset metadata: ") + e.what());
        return std::nullopt;
    }

    package.payload.resize(static_cast<size_t>(payloadSize));
    if (payloadSize > 0) {
        in.read(reinterpret_cast<char*>(package.payload.data()), static_cast<std::streamsize>(payloadSize));
        if (!in) {
            return std::nullopt;
        }
    }
    return package;
#endif
}

} // namespace we::runtime::assetimporter
