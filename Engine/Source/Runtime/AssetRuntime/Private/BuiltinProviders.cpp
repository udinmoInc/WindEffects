#include "BuiltinProviders.h"

#include "AssetCooker/WepakFormat.h"
#include "AssetImporter/AssetMetadata.h"
#include "AssetImporter/AssetPackage.h"
#include "AssetImporter/Types.h"
#include "AssetRuntime/AssetLoaderRegistry.h"
#include "AssetRuntime/PackageProviderRegistry.h"
#include "AssetRuntime/RuntimeTypes.h"

#include <cstring>
#include <fstream>
#include <unordered_map>

#if WE_HAS_NLOHMANN_JSON
#include <nlohmann/json.h>
#endif

namespace we::runtime::assetruntime {
namespace {

VirtualPath MakeVirtualPath(std::string_view virtualRoot, const std::filesystem::path& relative) {
    std::string combined(virtualRoot);
    if (!combined.empty() && combined.back() == '/') {
        combined.pop_back();
    }
    combined.push_back('/');
    std::string rel = relative.generic_string();
    for (char& c : rel) {
        if (c == '\\') {
            c = '/';
        }
    }
    // Strip cooked extension for cleaner virtual paths.
    const auto dot = rel.find_last_of('.');
    if (dot != std::string::npos) {
        const std::string ext = rel.substr(dot);
        if (IsCookedAssetExtension(ext) && ext != ".wemeta") {
            rel = rel.substr(0, dot);
        }
    }
    combined += rel;
    return VirtualPath::Normalize(combined);
}

we::runtime::assetimporter::AssetKind KindFromExtension(const std::filesystem::path& path) {
    const auto ext = path.extension().string();
    if (ext == ".wefont") return we::runtime::assetimporter::AssetKind::Font;
    if (ext == ".weiconatlas") return we::runtime::assetimporter::AssetKind::IconAtlas;
    if (ext == ".wetex") return we::runtime::assetimporter::AssetKind::Texture;
    if (ext == ".wemesh") return we::runtime::assetimporter::AssetKind::StaticMesh;
    if (ext == ".weanim") return we::runtime::assetimporter::AssetKind::Animation;
    if (ext == ".weaudio") return we::runtime::assetimporter::AssetKind::Audio;
    if (ext == ".weshader") return we::runtime::assetimporter::AssetKind::Shader;
    if (ext == ".wemat") return we::runtime::assetimporter::AssetKind::Material;
    if (ext == ".wescene") return we::runtime::assetimporter::AssetKind::Scene;
    return we::runtime::assetimporter::AssetKind::RawBinary;
}

struct LoosePackageState {
    std::filesystem::path root;
    std::unordered_map<
        we::runtime::assetimporter::AssetGuid,
        std::filesystem::path,
        we::runtime::assetimporter::AssetGuidHash>
        paths;
};

class LooseCookedPackageProvider final : public IPackageProvider {
public:
    [[nodiscard]] std::string_view GetProviderId() const override { return "provider.loose_cooked"; }
    [[nodiscard]] std::string_view GetDisplayName() const override {
        return "Loose Cooked Content";
    }
    [[nodiscard]] int GetPriority() const override { return 50; }

    [[nodiscard]] bool CanOpen(const std::filesystem::path& source) const override {
        std::error_code ec;
        return std::filesystem::is_directory(source, ec);
    }

    [[nodiscard]] PackageOpenResult Open(
        const std::filesystem::path& source,
        std::string_view virtualRoot) const override {
        PackageOpenResult result{};
        if (!CanOpen(source)) {
            result.errorCode = "package.not_directory";
            result.errorMessage = "Loose cooked provider expects a directory of cooked assets.";
            return result;
        }

        result.packageId = source.filename().string();
        result.packageVersion = 1;
        result.platformName = "Any";

        std::error_code ec;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(source, ec)) {
            if (!entry.is_regular_file(ec)) {
                continue;
            }
            const auto& path = entry.path();
            const auto ext = path.extension().string();
            if (IsRawSourceExtension(ext)) {
                continue; // never catalog raw sources
            }
            if (!IsCookedAssetExtension(ext) || ext == ".wemeta") {
                continue;
            }

            PackageAssetEntry asset{};
            const auto metaPath = path.parent_path() / (path.stem().string() + ".wemeta");
            if (auto meta = we::runtime::assetimporter::LoadMetadataJson(metaPath)) {
                asset.guid = meta->guid;
                asset.kind = meta->kind;
                asset.displayName = meta->displayName;
                asset.dependencies = meta->dependencies;
            } else if (auto pkg = we::runtime::assetimporter::ReadAssetPackage(path)) {
                asset.guid = pkg->metadata.guid;
                asset.kind = pkg->metadata.kind;
                asset.displayName = pkg->metadata.displayName;
                asset.dependencies = pkg->metadata.dependencies;
                asset.contentType = pkg->payloadContentType;
            }

            if (asset.guid.IsNil()) {
                continue;
            }
            if (asset.kind == we::runtime::assetimporter::AssetKind::Unknown) {
                asset.kind = KindFromExtension(path);
            }
            if (asset.displayName.empty()) {
                asset.displayName = path.stem().string();
            }
            if (asset.contentType.empty()) {
                asset.contentType = "application/vnd.windeffects.asset";
            }

            const auto relative = std::filesystem::relative(path, source, ec);
            asset.virtualPath = MakeVirtualPath(virtualRoot, relative);
            asset.uncompressedSize = entry.file_size(ec);
            result.assets.push_back(std::move(asset));
        }

        result.success = true;
        return result;
    }

    [[nodiscard]] void* CreateState(const std::filesystem::path& source) const override {
        auto* state = new LoosePackageState();
        state->root = source;
        std::error_code ec;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(source, ec)) {
            if (!entry.is_regular_file(ec)) {
                continue;
            }
            const auto& path = entry.path();
            const auto ext = path.extension().string();
            if (!IsCookedAssetExtension(ext) || ext == ".wemeta" || IsRawSourceExtension(ext)) {
                continue;
            }
            we::runtime::assetimporter::AssetGuid guid{};
            const auto metaPath = path.parent_path() / (path.stem().string() + ".wemeta");
            if (auto meta = we::runtime::assetimporter::LoadMetadataJson(metaPath)) {
                guid = meta->guid;
            } else if (auto pkg = we::runtime::assetimporter::ReadAssetPackage(path)) {
                guid = pkg->metadata.guid;
            }
            if (!guid.IsNil()) {
                state->paths[guid] = path;
            }
        }
        return state;
    }

    void DestroyState(void* packageState) const override {
        delete static_cast<LoosePackageState*>(packageState);
    }

    [[nodiscard]] PackageReadResult ReadAsset(
        void* packageState,
        const we::runtime::assetimporter::AssetGuid& guid) const override {
        PackageReadResult result{};
        auto* state = static_cast<LoosePackageState*>(packageState);
        if (!state) {
            result.errorCode = "package.state_invalid";
            result.errorMessage = "Loose package state is null.";
            return result;
        }
        const auto it = state->paths.find(guid);
        if (it == state->paths.end()) {
            result.errorCode = "asset.missing";
            result.errorMessage = "Asset GUID not found in loose package.";
            return result;
        }

        if (auto pkg = we::runtime::assetimporter::ReadAssetPackage(it->second)) {
            result.payload = std::move(pkg->payload);
            result.contentType = pkg->payloadContentType;
            result.kind = pkg->metadata.kind;
            result.displayName = pkg->metadata.displayName;
            result.dependencies = pkg->metadata.dependencies;
            result.success = true;
            return result;
        }

        // Non-WEAS cooked native: load raw file bytes.
        std::ifstream in(it->second, std::ios::binary);
        if (!in) {
            result.errorCode = "asset.read_failed";
            result.errorMessage = "Failed to read cooked asset file.";
            return result;
        }
        in.seekg(0, std::ios::end);
        const auto size = static_cast<size_t>(in.tellg());
        in.seekg(0, std::ios::beg);
        result.payload.resize(size);
        if (size > 0) {
            in.read(reinterpret_cast<char*>(result.payload.data()),
                static_cast<std::streamsize>(size));
        }
        result.kind = KindFromExtension(it->second);
        result.displayName = it->second.stem().string();
        result.contentType = "application/vnd.windeffects.asset";
        result.success = true;
        return result;
    }
};

struct WepakPackageState {
    we::runtime::assetcooker::AssetPackageArchive archive{};
    std::unordered_map<
        we::runtime::assetimporter::AssetGuid,
        size_t,
        we::runtime::assetimporter::AssetGuidHash>
        tocIndex;
};

class WepakPackageProvider final : public IPackageProvider {
public:
    [[nodiscard]] std::string_view GetProviderId() const override { return "provider.wepak"; }
    [[nodiscard]] std::string_view GetDisplayName() const override { return "WEPAK Package"; }
    [[nodiscard]] int GetPriority() const override { return 100; }

    [[nodiscard]] bool CanOpen(const std::filesystem::path& source) const override {
        std::error_code ec;
        return std::filesystem::is_regular_file(source, ec)
            && source.extension() == ".wepak";
    }

    [[nodiscard]] PackageOpenResult Open(
        const std::filesystem::path& source,
        std::string_view virtualRoot) const override {
        PackageOpenResult result{};
        auto archive = we::runtime::assetcooker::ReadWepak(source);
        if (!archive) {
            result.errorCode = "package.invalid";
            result.errorMessage = "Failed to read or validate .wepak package.";
            return result;
        }

        result.packageId = source.stem().string();
        result.packageVersion = we::runtime::assetcooker::AssetPackageArchive::kVersion;
        result.platform = archive->platform;
        result.platformName = archive->platformName;

#if WE_HAS_NLOHMANN_JSON
        // Optional extended header fields written by future cookers / patches.
        // ReadWepak already consumed the file; re-parse is expensive — use defaults unless
        // TOC relative paths encode identity. Version already validated against kVersion.
#endif

        for (const auto& rec : archive->toc) {
            if (rec.guid.IsNil()) {
                continue;
            }
            PackageAssetEntry asset{};
            asset.guid = rec.guid;
            asset.contentType = rec.contentType;
            asset.displayName = rec.relativePath;
            asset.uncompressedSize = rec.uncompressedSize;
            asset.virtualPath = MakeVirtualPath(virtualRoot, rec.relativePath);
            asset.kind = KindFromExtension(rec.relativePath);
            result.assets.push_back(std::move(asset));
        }

        result.success = true;
        return result;
    }

    [[nodiscard]] void* CreateState(const std::filesystem::path& source) const override {
        auto archive = we::runtime::assetcooker::ReadWepak(source);
        if (!archive) {
            return nullptr;
        }
        auto* state = new WepakPackageState();
        state->archive = std::move(*archive);
        for (size_t i = 0; i < state->archive.toc.size(); ++i) {
            const auto& rec = state->archive.toc[i];
            if (!rec.guid.IsNil()) {
                state->tocIndex[rec.guid] = i;
            }
        }
        return state;
    }

    void DestroyState(void* packageState) const override {
        delete static_cast<WepakPackageState*>(packageState);
    }

    [[nodiscard]] PackageReadResult ReadAsset(
        void* packageState,
        const we::runtime::assetimporter::AssetGuid& guid) const override {
        PackageReadResult result{};
        auto* state = static_cast<WepakPackageState*>(packageState);
        if (!state) {
            result.errorCode = "package.state_invalid";
            result.errorMessage = "WEPAK package state is null.";
            return result;
        }
        const auto it = state->tocIndex.find(guid);
        if (it == state->tocIndex.end()) {
            result.errorCode = "asset.missing";
            result.errorMessage = "Asset GUID not found in package TOC.";
            return result;
        }
        const auto& rec = state->archive.toc[it->second];
        const uint64_t end = static_cast<uint64_t>(rec.offset) + rec.storedSize;
        if (end > state->archive.payloadBlob.size()) {
            result.errorCode = "asset.corrupted";
            result.errorMessage = "Package TOC offset/size exceeds payload blob.";
            return result;
        }
        result.payload.assign(
            state->archive.payloadBlob.begin() + static_cast<std::ptrdiff_t>(rec.offset),
            state->archive.payloadBlob.begin() + static_cast<std::ptrdiff_t>(end));

        // Prefer decoded WEAS payload when the blob is a WEAS package.
        if (result.payload.size() >= 4) {
            uint32_t magic = 0;
            std::memcpy(&magic, result.payload.data(), sizeof(magic));
            if (magic == we::runtime::assetimporter::AssetPackage::kMagic) {
                // Write to a temp decode via in-memory path is unavailable; parse inline.
                // Fall through with raw WEAS bytes — AssetManager will try ReadAssetPackage
                // only for loose files. Decode WEAS here:
                // Use a lightweight parse by writing isn't needed — keep raw and let
                // passthrough loader accept bytes. For WEAS, extract payload below.
            }
        }

        // Attempt in-place WEAS unwrap without disk I/O.
        if (TryUnwrapWeas(result.payload, result)) {
            result.success = true;
            return result;
        }

        result.contentType = rec.contentType;
        result.displayName = rec.relativePath;
        result.kind = KindFromExtension(rec.relativePath);
        result.success = true;
        return result;
    }

private:
    static bool TryUnwrapWeas(std::vector<std::byte>& bytes, PackageReadResult& out) {
        if (bytes.size() < 16) {
            return false;
        }
        uint32_t magic = 0;
        std::memcpy(&magic, bytes.data(), sizeof(magic));
        if (magic != we::runtime::assetimporter::AssetPackage::kMagic) {
            return false;
        }
        uint16_t version = 0;
        uint16_t flags = 0;
        uint32_t metaSize = 0;
        uint64_t payloadSize = 0;
        std::memcpy(&version, bytes.data() + 4, sizeof(version));
        std::memcpy(&flags, bytes.data() + 6, sizeof(flags));
        std::memcpy(&metaSize, bytes.data() + 8, sizeof(metaSize));
        std::memcpy(&payloadSize, bytes.data() + 12, sizeof(payloadSize));
        (void)flags;
        if (version != we::runtime::assetimporter::AssetPackage::kVersion) {
            return false;
        }
        const size_t headerBytes = 16;
        if (bytes.size() < headerBytes + metaSize + payloadSize) {
            return false;
        }
#if WE_HAS_NLOHMANN_JSON
        try {
            const std::string metaJson(
                reinterpret_cast<const char*>(bytes.data() + headerBytes), metaSize);
            const auto meta = nlohmann::json::parse(metaJson);
            out.contentType = meta.value("payloadContentType", "");
            out.displayName = meta.value("displayName", "");
            out.kind = we::runtime::assetimporter::AssetKindFromString(meta.value("kind", ""));
            if (meta.contains("dependencies") && meta["dependencies"].is_array()) {
                for (const auto& d : meta["dependencies"]) {
                    if (auto g = we::runtime::assetimporter::AssetGuid::Parse(d.get<std::string>())) {
                        out.dependencies.push_back(*g);
                    }
                }
            }
        } catch (...) {
            return false;
        }
#else
        (void)metaSize;
#endif
        const auto payloadBegin = bytes.begin()
            + static_cast<std::ptrdiff_t>(headerBytes + metaSize);
        out.payload.assign(payloadBegin, payloadBegin + static_cast<std::ptrdiff_t>(payloadSize));
        return true;
    }
};

class PassthroughAssetLoader final : public IAssetLoader {
public:
    [[nodiscard]] std::string_view GetLoaderId() const override { return "loader.passthrough"; }
    [[nodiscard]] std::string_view GetDisplayName() const override {
        return "Passthrough Cooked Payload";
    }
    [[nodiscard]] we::runtime::assetimporter::AssetKind GetAssetKind() const override {
        return we::runtime::assetimporter::AssetKind::RawBinary;
    }
    [[nodiscard]] std::vector<std::string> GetSupportedContentTypes() const override {
        return { "*" };
    }
    [[nodiscard]] int GetPriority() const override { return 1; }

    [[nodiscard]] bool CanLoad(
        we::runtime::assetimporter::AssetKind /*kind*/,
        std::string_view /*contentType*/) const override {
        return true;
    }

    [[nodiscard]] AssetLoadDecodeResult Decode(
        const RuntimeAsset& asset,
        std::span<const std::byte> cookedPayload) const override {
        AssetLoadDecodeResult result{};
        auto bytes = std::make_shared<std::vector<std::byte>>(
            cookedPayload.begin(), cookedPayload.end());
        result.object = bytes;
        result.typeId = "runtime.bytes.v1";
        result.success = true;
        (void)asset;
        return result;
    }
};

} // namespace

std::shared_ptr<IPackageProvider> CreateLooseCookedPackageProvider() {
    return std::make_shared<LooseCookedPackageProvider>();
}

std::shared_ptr<IPackageProvider> CreateWepakPackageProvider() {
    return std::make_shared<WepakPackageProvider>();
}

std::shared_ptr<IAssetLoader> CreatePassthroughAssetLoader() {
    return std::make_shared<PassthroughAssetLoader>();
}

void RegisterBuiltinPackageProviders(PackageProviderRegistry& registry) {
    registry.Register(CreateLooseCookedPackageProvider());
    registry.Register(CreateWepakPackageProvider());
}

void RegisterBuiltinAssetLoaders(AssetLoaderRegistry& registry) {
    registry.Register(CreatePassthroughAssetLoader());
}

} // namespace we::runtime::assetruntime
