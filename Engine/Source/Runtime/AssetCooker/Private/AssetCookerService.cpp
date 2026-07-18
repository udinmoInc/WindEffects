#include "AssetCooker/IAssetCooker.h"

#include "AssetCooker/WepakFormat.h"
#include "AssetImporter/AssetMetadata.h"
#include "AssetImporter/AssetPackage.h"
#include "Core/Logger.h"
#include "Core/Paths.h"

#include <algorithm>
#include <fstream>
#include <unordered_set>

namespace we::runtime::assetcooker {
namespace {

bool IsCookableFile(const std::filesystem::path& path) {
    const auto ext = path.extension().string();
    return ext == ".weasset" || ext == ".wefont" || ext == ".weiconatlas"
        || ext == ".wemesh" || ext == ".weaudio" || ext == ".weshader";
}

std::vector<std::byte> ReadAllBytes(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return {};
    }
    in.seekg(0, std::ios::end);
    const auto size = static_cast<size_t>(in.tellg());
    in.seekg(0, std::ios::beg);
    std::vector<std::byte> data(size);
    if (size > 0) {
        in.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));
    }
    return data;
}

class AssetCookerService final : public IAssetCooker {
public:
    explicit AssetCookerService(AssetCookerDependencies deps)
        : m_Deps(std::move(deps)) {}

    [[nodiscard]] CookResult CookSync(
        const CookRequest& request,
        CookProgressCallback onProgress) override {
        CookResult result{};
        result.platform = request.platform;
        result.success = false;

        if (request.contentRoot.empty() || !std::filesystem::exists(request.contentRoot)) {
            CookDiagnostic d{};
            d.severity = we::runtime::assetimporter::ImportSeverity::Fatal;
            d.code = "cook.content_missing";
            d.message = "Content root missing for cook.";
            d.path = request.contentRoot.string();
            result.diagnostics.push_back(d);
            if (m_Deps.onDiagnostic) {
                m_Deps.onDiagnostic(d);
            }
            return result;
        }

        const std::string platformName = request.platform == CookPlatform::Custom
            ? (request.customPlatformName.empty() ? "Custom" : request.customPlatformName)
            : std::string(CookPlatformToString(request.platform));

        result.cookedRoot = request.outputDirectory.empty()
            ? we::core::PathService::Get().CookedRootForPlatform(platformName)
            : request.outputDirectory;

        std::error_code ec;
        std::filesystem::create_directories(result.cookedRoot, ec);

        std::vector<std::filesystem::path> files;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(request.contentRoot)) {
            if (entry.is_regular_file() && IsCookableFile(entry.path())) {
                files.push_back(entry.path());
            }
        }
        if (request.deterministic) {
            std::sort(files.begin(), files.end());
        }

        std::unordered_set<
            we::runtime::assetimporter::AssetGuid,
            we::runtime::assetimporter::AssetGuidHash>
            filter;
        for (const auto& g : request.onlyGuids) {
            if (!g.IsNil()) {
                filter.insert(g);
            }
        }

        AssetPackageArchive archive{};
        archive.platform = request.platform;
        archive.platformName = platformName;

        const uint32_t total = static_cast<uint32_t>(files.size());
        uint32_t index = 0;
        for (const auto& file : files) {
            ++index;
            if (onProgress) {
                CookProgress p{};
                p.fraction = total == 0 ? 1.0f : static_cast<float>(index) / static_cast<float>(total);
                p.stage = "Cook";
                p.cookedAssets = index;
                p.totalAssets = total;
                onProgress(p);
            }

            auto bytes = ReadAllBytes(file);
            if (bytes.empty()) {
                CookDiagnostic d{};
                d.severity = we::runtime::assetimporter::ImportSeverity::Warning;
                d.code = "cook.empty_asset";
                d.message = "Skipping empty cookable file.";
                d.path = file.string();
                result.diagnostics.push_back(d);
                continue;
            }

            CookedAssetRecord rec{};
            rec.relativePath = std::filesystem::relative(file, request.contentRoot, ec).generic_string();
            if (ec) {
                rec.relativePath = file.filename().string();
            }
            rec.contentType = "application/vnd.windeffects.asset";
            rec.uncompressedSize = bytes.size();
            rec.storedSize = bytes.size();
            rec.offset = static_cast<uint32_t>(archive.payloadBlob.size());

            // Prefer guid from .wemeta or embedded package.
            const auto metaPath = file.parent_path() / (file.stem().string() + ".wemeta");
            if (auto meta = we::runtime::assetimporter::LoadMetadataJson(metaPath)) {
                rec.guid = meta->guid;
            } else if (auto pkg = we::runtime::assetimporter::ReadAssetPackage(file)) {
                rec.guid = pkg->metadata.guid;
            }

            if (!filter.empty() && (rec.guid.IsNil() || filter.find(rec.guid) == filter.end())) {
                continue;
            }

            // Platform-specific copy into Cooked/<Platform>/ preserving relative path.
            const auto dest = result.cookedRoot / rec.relativePath;
            std::filesystem::create_directories(dest.parent_path(), ec);
            std::filesystem::copy_file(
                file,
                dest,
                std::filesystem::copy_options::overwrite_existing,
                ec);

            if (request.includeThumbnails) {
                // Thumbnails remain optional sidecars; no hard failure.
            }

            archive.payloadBlob.insert(archive.payloadBlob.end(), bytes.begin(), bytes.end());
            result.totalBytes += bytes.size();
            archive.toc.push_back(rec);
            result.assets.push_back(std::move(rec));
        }

        const auto pakPath = request.packagePath.empty()
            ? (result.cookedRoot / (platformName + ".wepak"))
            : request.packagePath;
        if (!WriteWepak(pakPath, archive)) {
            CookDiagnostic d{};
            d.severity = we::runtime::assetimporter::ImportSeverity::Error;
            d.code = "cook.wepak_write_failed";
            d.message = "Failed to write .wepak package.";
            d.path = pakPath.string();
            result.diagnostics.push_back(d);
            if (m_Deps.onDiagnostic) {
                m_Deps.onDiagnostic(d);
            }
            return result;
        }
        result.packagePath = pakPath;

        result.success = true;
        HE_INFO("[AssetCooker] Cooked " + std::to_string(result.assets.size())
            + " assets for " + platformName);
        return result;
    }

private:
    AssetCookerDependencies m_Deps{};
};

} // namespace

std::unique_ptr<IAssetCooker> CreateAssetCooker(AssetCookerDependencies deps) {
    return std::make_unique<AssetCookerService>(std::move(deps));
}

} // namespace we::runtime::assetcooker
