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
