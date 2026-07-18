#pragma once

#include "AssetCooker/CookTypes.h"
#include "AssetCooker/Export.h"
#include "AssetCooker/WepakFormat.h"

#include <memory>

namespace we::runtime::assetcooker {

struct ASSETCOOKER_API AssetCookerDependencies {
    std::string engineVersion = "0.1.0";
    std::function<void(const CookDiagnostic&)> onDiagnostic;
};

/// Platform cook + optional .wepak packaging. Consumes importer/processor outputs.
class ASSETCOOKER_API IAssetCooker {
public:
    virtual ~IAssetCooker() = default;

    [[nodiscard]] virtual CookResult CookSync(
        const CookRequest& request,
        CookProgressCallback onProgress = {}) = 0;
};

[[nodiscard]] ASSETCOOKER_API std::unique_ptr<IAssetCooker> CreateAssetCooker(
    AssetCookerDependencies deps = {});

} // namespace we::runtime::assetcooker
