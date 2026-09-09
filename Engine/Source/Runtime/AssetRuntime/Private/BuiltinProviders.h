// ==============================================================================
// WindEffects — AssetRuntime — BuiltinProviders
// Internal implementation for the AssetRuntime module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "AssetRuntime/AssetLoaderRegistry.h"
#include "AssetRuntime/IAssetLoader.h"
#include "AssetRuntime/IPackageProvider.h"
#include "AssetRuntime/PackageProviderRegistry.h"

#include <memory>

namespace we::runtime::assetruntime {

[[nodiscard]] std::shared_ptr<IPackageProvider> CreateLooseCookedPackageProvider();
[[nodiscard]] std::shared_ptr<IPackageProvider> CreateWepakPackageProvider();
[[nodiscard]] std::shared_ptr<IAssetLoader> CreatePassthroughAssetLoader();

void RegisterBuiltinPackageProviders(PackageProviderRegistry& registry);
void RegisterBuiltinAssetLoaders(AssetLoaderRegistry& registry);

} // namespace we::runtime::assetruntime
