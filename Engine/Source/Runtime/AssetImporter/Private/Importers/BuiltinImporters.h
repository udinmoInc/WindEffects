#pragma once

#include "AssetImporter/IAssetImporter.h"

#include <memory>

namespace we::runtime::assetimporter {

[[nodiscard]] std::shared_ptr<IAssetImporter> CreateTextureImporter();
[[nodiscard]] std::shared_ptr<IAssetImporter> CreateFontImporterAdapter();
[[nodiscard]] std::shared_ptr<IAssetImporter> CreateIconImporterAdapter();
[[nodiscard]] std::shared_ptr<IAssetImporter> CreateMeshImporter();
[[nodiscard]] std::shared_ptr<IAssetImporter> CreateAnimationImporter();
[[nodiscard]] std::shared_ptr<IAssetImporter> CreateAudioImporter();
[[nodiscard]] std::shared_ptr<IAssetImporter> CreateShaderImporter();
[[nodiscard]] std::shared_ptr<IAssetImporter> CreateMaterialImporter();
[[nodiscard]] std::shared_ptr<IAssetImporter> CreateSceneImporter();
[[nodiscard]] std::shared_ptr<IAssetImporter> CreateRawBinaryImporter();

void RegisterBuiltinImporters(class ImporterRegistry& registry);

} // namespace we::runtime::assetimporter
