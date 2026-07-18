#include "AssetCooker/WepakFormat.h"

#include "AssetImporter/AssetGuid.h"

#include <cstring>
#include <fstream>

#if WE_HAS_NLOHMANN_JSON
#include <nlohmann/json.h>
#endif

namespace we::runtime::assetcooker {
namespace {

void WriteU32(std::ostream& out, uint32_t v) {
    out.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

void WriteU16(std::ostream& out, uint16_t v) {
    out.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

bool ReadU32(std::istream& in, uint32_t& v) {
    return static_cast<bool>(in.read(reinterpret_cast<char*>(&v), sizeof(v)));
}

bool ReadU16(std::istream& in, uint16_t& v) {
    return static_cast<bool>(in.read(reinterpret_cast<char*>(&v), sizeof(v)));
}

} // namespace

bool WriteWepak(const std::filesystem::path& path, const AssetPackageArchive& archive) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

#if WE_HAS_NLOHMANN_JSON
    nlohmann::json toc = nlohmann::json::array();
    for (const auto& rec : archive.toc) {
        toc.push_back({
            {"guid", rec.guid.ToString()},
            {"path", rec.relativePath},
            {"contentType", rec.contentType},
            {"uncompressedSize", rec.uncompressedSize},
            {"storedSize", rec.storedSize},
            {"offset", rec.offset},
        });
    }
    nlohmann::json header = {
        {"platform", archive.platformName.empty()
            ? std::string(CookPlatformToString(archive.platform))
            : archive.platformName},
        {"platformId", static_cast<uint32_t>(archive.platform)},
        {"toc", toc},
    };
    const std::string headerJson = header.dump();
#else
    const std::string headerJson = "{}";
#endif

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }

    WriteU32(out, AssetPackageArchive::kMagic);
    WriteU16(out, AssetPackageArchive::kVersion);
    WriteU16(out, 0); // flags
    WriteU32(out, static_cast<uint32_t>(headerJson.size()));
    WriteU32(out, static_cast<uint32_t>(archive.payloadBlob.size()));
    out.write(headerJson.data(), static_cast<std::streamsize>(headerJson.size()));
    if (!archive.payloadBlob.empty()) {
        out.write(
            reinterpret_cast<const char*>(archive.payloadBlob.data()),
            static_cast<std::streamsize>(archive.payloadBlob.size()));
    }
    return static_cast<bool>(out);
}

std::optional<AssetPackageArchive> ReadWepak(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return std::nullopt;
    }

    uint32_t magic = 0;
    uint16_t version = 0;
    uint16_t flags = 0;
    uint32_t headerSize = 0;
    uint32_t payloadSize = 0;
    if (!ReadU32(in, magic) || magic != AssetPackageArchive::kMagic) {
        return std::nullopt;
    }
    if (!ReadU16(in, version) || !ReadU16(in, flags) || !ReadU32(in, headerSize)
        || !ReadU32(in, payloadSize)) {
        return std::nullopt;
    }
    (void)flags;
    if (version != AssetPackageArchive::kVersion) {
        return std::nullopt;
    }

    std::string headerJson(headerSize, '\0');
    if (!in.read(headerJson.data(), static_cast<std::streamsize>(headerSize))) {
        return std::nullopt;
    }

    AssetPackageArchive archive{};
    archive.payloadBlob.resize(payloadSize);
    if (payloadSize > 0
        && !in.read(
            reinterpret_cast<char*>(archive.payloadBlob.data()),
            static_cast<std::streamsize>(payloadSize))) {
        return std::nullopt;
    }

#if WE_HAS_NLOHMANN_JSON
    try {
        const auto header = nlohmann::json::parse(headerJson);
        archive.platformName = header.value("platform", "Windows");
        archive.platform = static_cast<CookPlatform>(header.value("platformId", 0u));
        if (header.contains("toc") && header["toc"].is_array()) {
            for (const auto& j : header["toc"]) {
                CookedAssetRecord rec{};
                if (auto g = we::runtime::assetimporter::AssetGuid::Parse(j.value("guid", ""))) {
                    rec.guid = *g;
                }
                rec.relativePath = j.value("path", "");
                rec.contentType = j.value("contentType", "");
                rec.uncompressedSize = j.value("uncompressedSize", 0ull);
                rec.storedSize = j.value("storedSize", 0ull);
                rec.offset = j.value("offset", 0u);
                archive.toc.push_back(std::move(rec));
            }
        }
    } catch (...) {
        return std::nullopt;
    }
#else
    (void)headerJson;
#endif
    return archive;
}

} // namespace we::runtime::assetcooker
