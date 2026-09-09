// ==============================================================================
// WindEffects — AssetCooker — IAssetCooker
// Public API surface for the AssetCooker module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
