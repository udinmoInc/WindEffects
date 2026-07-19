#pragma once

#include "Prefab/Export.h"
#include "Prefab/PrefabTypes.h"
#include "Prefab/IPrefabAsset.h"

#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::prefab {

enum class PrefabCommandId : std::uint16_t {
    CreateFromSelection = 1,
    SpawnAtOrigin,
    ApplyInstance,
    RevertInstance,
    UpdateAllInstances,
    BreakLink,
    RestoreLink,
    DuplicateAsset,
    ConvertToActors,
    SelectSourceAsset,
    ValidateAsset,
};

struct PREFAB_API PrefabCommandContext {
    PrefabGuid asset{};
    PrefabInstanceId instance{};
    std::uint64_t rootEntityId = 0;
    std::string path;
    std::string name;
    PrefabSpawnParams spawn{};
};

class PREFAB_API IPrefabCommandRouter {
public:
    virtual ~IPrefabCommandRouter() = default;

    [[nodiscard]] virtual bool Execute(PrefabCommandId id, const PrefabCommandContext& ctx) = 0;
    [[nodiscard]] virtual bool CanExecute(PrefabCommandId id, const PrefabCommandContext& ctx) const = 0;
};

struct PREFAB_API PrefabThumbnailRequest {
    PrefabGuid asset{};
    std::uint32_t width = 128;
    std::uint32_t height = 128;
};

struct PREFAB_API PrefabThumbnailResult {
    bool success = false;
    std::vector<std::uint8_t> rgba;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

class PREFAB_API IPrefabThumbnailProvider {
public:
    virtual ~IPrefabThumbnailProvider() = default;
    [[nodiscard]] virtual PrefabThumbnailResult Generate(const PrefabThumbnailRequest& request) = 0;
    [[nodiscard]] virtual bool SupportsAsync() const noexcept { return true; }
};

struct PREFAB_API PrefabPreviewRequest {
    PrefabGuid asset{};
    bool includeChildren = true;
};

struct PREFAB_API PrefabPreviewResult {
    bool success = false;
    PrefabNodeTemplate root{};
    std::vector<PrefabValidationIssue> issues;
};

class PREFAB_API IPrefabPreviewProvider {
public:
    virtual ~IPrefabPreviewProvider() = default;
    [[nodiscard]] virtual PrefabPreviewResult BuildPreview(const PrefabPreviewRequest& request) = 0;
};

} // namespace we::runtime::prefab
