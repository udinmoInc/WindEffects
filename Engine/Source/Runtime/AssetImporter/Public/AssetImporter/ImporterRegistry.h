#pragma once

#include "AssetImporter/Export.h"
#include "AssetImporter/IAssetImporter.h"
#include "AssetImporter/Types.h"

#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::runtime::assetimporter {

/// Thread-safe registry of importer backends. Owned by IAssetImportService.
class ASSETIMPORTER_API ImporterRegistry {
public:
    void Register(std::shared_ptr<IAssetImporter> importer);
    void Unregister(std::string_view importerId);
    void Clear();

    [[nodiscard]] std::shared_ptr<IAssetImporter> FindById(std::string_view importerId) const;
    [[nodiscard]] std::shared_ptr<IAssetImporter> ResolveForPath(const std::filesystem::path& sourcePath) const;
    [[nodiscard]] std::shared_ptr<IAssetImporter> ResolveForKind(AssetKind kind) const;
    [[nodiscard]] std::vector<std::shared_ptr<IAssetImporter>> GetAll() const;

private:
    mutable std::mutex m_Mutex;
    std::unordered_map<std::string, std::shared_ptr<IAssetImporter>> m_ById;
};

} // namespace we::runtime::assetimporter
