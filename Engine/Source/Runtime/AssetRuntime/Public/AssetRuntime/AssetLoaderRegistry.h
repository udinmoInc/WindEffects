#pragma once

#include "AssetImporter/Types.h"
#include "AssetRuntime/Export.h"
#include "AssetRuntime/IAssetLoader.h"

#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace we::runtime::assetruntime {

/// Thread-safe registry of runtime asset loaders. Plugins register without modifying engine code.
class ASSETRUNTIME_API AssetLoaderRegistry {
public:
    void Register(std::shared_ptr<IAssetLoader> loader);
    void Unregister(std::string_view loaderId);
    void Clear();

    [[nodiscard]] std::shared_ptr<IAssetLoader> FindById(std::string_view loaderId) const;
    [[nodiscard]] std::shared_ptr<IAssetLoader> Resolve(
        we::runtime::assetimporter::AssetKind kind,
        std::string_view contentType) const;
    [[nodiscard]] std::vector<std::shared_ptr<IAssetLoader>> GetAll() const;

private:
    mutable std::mutex m_Mutex;
    std::unordered_map<std::string, std::shared_ptr<IAssetLoader>> m_ById;
};

} // namespace we::runtime::assetruntime
