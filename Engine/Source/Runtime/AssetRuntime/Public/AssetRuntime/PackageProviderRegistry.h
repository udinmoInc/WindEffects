#pragma once

#include "AssetRuntime/Export.h"
#include "AssetRuntime/IPackageProvider.h"

#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace we::runtime::assetruntime {

/// Thread-safe registry of package providers (DLC, mods, patches, CDN, …).
class ASSETRUNTIME_API PackageProviderRegistry {
public:
    void Register(std::shared_ptr<IPackageProvider> provider);
    void Unregister(std::string_view providerId);
    void Clear();

    [[nodiscard]] std::shared_ptr<IPackageProvider> FindById(std::string_view providerId) const;
    [[nodiscard]] std::shared_ptr<IPackageProvider> ResolveForPath(
        const std::filesystem::path& source) const;
    [[nodiscard]] std::vector<std::shared_ptr<IPackageProvider>> GetAll() const;

private:
    mutable std::mutex m_Mutex;
    std::unordered_map<std::string, std::shared_ptr<IPackageProvider>> m_ById;
};

} // namespace we::runtime::assetruntime
