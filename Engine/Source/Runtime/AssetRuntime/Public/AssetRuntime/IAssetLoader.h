#pragma once

#include "AssetImporter/Types.h"
#include "AssetRuntime/Export.h"
#include "AssetRuntime/RuntimeTypes.h"

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::assetruntime {

/// Result of a plugin/runtime type loader decoding cooked bytes into a domain object.
struct ASSETRUNTIME_API AssetLoadDecodeResult {
    bool success = false;
    std::string errorCode;
    std::string errorMessage;
    /// Type-erased domain object (texture, mesh, font, …). Owned by the loader consumer.
    std::shared_ptr<void> object;
    std::string typeId; // e.g. "runtime.texture.v1"
};

/// Plugin-extensible runtime decoder for cooked payloads.
/// Runtime never loads raw sources (PNG/FBX/…); loaders only see cooked bytes.
class ASSETRUNTIME_API IAssetLoader {
public:
    virtual ~IAssetLoader() = default;

    [[nodiscard]] virtual std::string_view GetLoaderId() const = 0;
    [[nodiscard]] virtual std::string_view GetDisplayName() const = 0;
    [[nodiscard]] virtual we::runtime::assetimporter::AssetKind GetAssetKind() const = 0;
    [[nodiscard]] virtual std::vector<std::string> GetSupportedContentTypes() const = 0;
    [[nodiscard]] virtual int GetPriority() const { return 100; }

    [[nodiscard]] virtual bool CanLoad(
        we::runtime::assetimporter::AssetKind kind,
        std::string_view contentType) const = 0;

    /// Decode cooked payload. Must be thread-safe for concurrent calls on distinct assets.
    [[nodiscard]] virtual AssetLoadDecodeResult Decode(
        const RuntimeAsset& asset,
        std::span<const std::byte> cookedPayload) const = 0;
};

} // namespace we::runtime::assetruntime
