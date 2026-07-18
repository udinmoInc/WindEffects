#pragma once

#include "AssetCooker/CookPlatform.h"
#include "AssetImporter/AssetGuid.h"
#include "AssetImporter/Types.h"
#include "AssetRuntime/Export.h"
#include "AssetRuntime/VirtualPath.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::assetruntime {

struct ASSETRUNTIME_API PackageAssetEntry {
    we::runtime::assetimporter::AssetGuid guid{};
    VirtualPath virtualPath;
    we::runtime::assetimporter::AssetKind kind = we::runtime::assetimporter::AssetKind::Unknown;
    std::string displayName;
    std::string contentType;
    std::vector<we::runtime::assetimporter::AssetGuid> dependencies;
    uint64_t uncompressedSize = 0;
};

struct ASSETRUNTIME_API PackageOpenResult {
    bool success = false;
    std::string errorCode;
    std::string errorMessage;
    std::string packageId;
    uint32_t packageVersion = 0;
    we::runtime::assetcooker::CookPlatform platform = we::runtime::assetcooker::CookPlatform::Windows;
    std::string platformName;
    std::vector<PackageAssetEntry> assets;
};

struct ASSETRUNTIME_API PackageReadResult {
    bool success = false;
    std::string errorCode;
    std::string errorMessage;
    std::vector<std::byte> payload;
    std::string contentType;
    we::runtime::assetimporter::AssetKind kind = we::runtime::assetimporter::AssetKind::Unknown;
    std::string displayName;
    std::vector<we::runtime::assetimporter::AssetGuid> dependencies;
};

/// Extensible package backend (loose cooked dir, .wepak, future DLC/CDN/mod providers).
class ASSETRUNTIME_API IPackageProvider {
public:
    virtual ~IPackageProvider() = default;

    [[nodiscard]] virtual std::string_view GetProviderId() const = 0;
    [[nodiscard]] virtual std::string_view GetDisplayName() const = 0;
    [[nodiscard]] virtual int GetPriority() const { return 100; }

    [[nodiscard]] virtual bool CanOpen(const std::filesystem::path& source) const = 0;

    [[nodiscard]] virtual PackageOpenResult Open(
        const std::filesystem::path& source,
        std::string_view virtualRoot) const = 0;

    /// Read a single cooked asset by GUID from an already-opened package instance.
    [[nodiscard]] virtual PackageReadResult ReadAsset(
        void* packageState,
        const we::runtime::assetimporter::AssetGuid& guid) const = 0;

    [[nodiscard]] virtual void* CreateState(const std::filesystem::path& source) const = 0;
    virtual void DestroyState(void* packageState) const = 0;
};

using PackageProviderFactory = std::function<std::shared_ptr<IPackageProvider>()>;

} // namespace we::runtime::assetruntime
