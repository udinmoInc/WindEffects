#pragma once

#include "AssetImporter/Export.h"
#include "AssetImporter/ImportTypes.h"
#include "AssetImporter/Types.h"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::assetimporter {

/// Per-domain importer. Implementations live in Private/; plugins may register their own.
class ASSETIMPORTER_API IAssetImporter {
public:
    virtual ~IAssetImporter() = default;

    [[nodiscard]] virtual std::string_view GetImporterId() const = 0;
    [[nodiscard]] virtual std::string_view GetDisplayName() const = 0;
    [[nodiscard]] virtual AssetKind GetAssetKind() const = 0;
    [[nodiscard]] virtual std::vector<std::string> GetSupportedExtensions() const = 0;
    [[nodiscard]] virtual int GetPriority() const { return 100; }

    [[nodiscard]] virtual bool CanImport(const std::filesystem::path& sourcePath) const = 0;

    /// Synchronous import. Must be thread-safe for concurrent calls on distinct requests.
    [[nodiscard]] virtual ImportResult Import(
        const ImportRequest& request,
        const ImportProgressCallback& progress) const = 0;
};

} // namespace we::runtime::assetimporter
