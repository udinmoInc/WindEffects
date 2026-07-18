#pragma once

#include "AssetImporter/Export.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace we::runtime::assetimporter {

/// Canonical asset kind for the import pipeline (source and cooked).
enum class AssetKind : uint32_t {
    Unknown = 0,
    Texture,
    Font,
    Icon,
    IconAtlas,
    StaticMesh,
    SkeletalMesh,
    Animation,
    Audio,
    Shader,
    Material,
    MaterialInstance,
    Scene,
    Blueprint,
    Prefab,
    Script,
    Video,
    RawBinary,
    Count
};

enum class ImportSeverity : uint8_t {
    Info = 0,
    Warning,
    Error,
    Fatal
};

enum class ImportJobState : uint8_t {
    Queued = 0,
    Running,
    Succeeded,
    Failed,
    Cancelled
};

[[nodiscard]] ASSETIMPORTER_API std::string_view AssetKindToString(AssetKind kind);
[[nodiscard]] ASSETIMPORTER_API AssetKind AssetKindFromString(std::string_view name);
[[nodiscard]] ASSETIMPORTER_API std::string_view AssetKindNativeExtension(AssetKind kind);
[[nodiscard]] ASSETIMPORTER_API AssetKind AssetKindFromSourceExtension(std::string_view extension);

} // namespace we::runtime::assetimporter
