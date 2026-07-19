#pragma once

#include "PrefabEditor/Export.h"
#include "Prefab/Prefab.h"
#include "Core/Math/Types.h"

#include <functional>
#include <memory>
#include <string_view>

namespace we::runtime::scene {
class Scene;
}

namespace we::editor::viewportedit {
class IViewportEditor;
}

namespace we::editor::outliner {
class IWorldOutlinerRuntime;
}

namespace we::editor::contentbrowser {
class IContentBrowserRuntime;
}

namespace we::editor::prefab {

/// Host-injected reversible transaction (Editor wires Undo::RecordCustom).
using PrefabTransactionFn = std::function<bool(
    std::string_view label,
    std::function<bool()> undoFn,
    std::function<bool()> redoFn)>;

class PREFABEDITOR_API IPrefabEditor {
public:
    virtual ~IPrefabEditor() = default;

    [[nodiscard]] virtual runtime::IPrefabRuntime& Runtime() noexcept = 0;
    [[nodiscard]] virtual const runtime::IPrefabRuntime& Runtime() const noexcept = 0;

    /// Undo-aware command execution (wraps IPrefabCommandRouter).
    [[nodiscard]] virtual bool ExecuteCommand(
        runtime::PrefabCommandId id,
        const runtime::PrefabCommandContext& ctx) = 0;

    /// Spawn from Content Browser / Viewport drag payload (asset path or guid text).
    [[nodiscard]] virtual runtime::PrefabInstanceId SpawnFromPayload(
        std::string_view payloadType,
        std::string_view payloadData,
        const we::math::Vec3& worldPosition,
        std::uint64_t parentEntityId = 0) = 0;

    /// Convert selected Scene entities into a new .weprefab and register it.
    [[nodiscard]] virtual bool ConvertSelectionToPrefab(
        std::uint64_t rootEntityId,
        std::string_view name,
        std::string_view assetPath) = 0;

    virtual void Tick(float deltaSeconds) = 0;
    virtual void Shutdown() = 0;
};

struct PREFABEDITOR_API PrefabEditorDependencies {
    runtime::IPrefabRuntime* prefabRuntime = nullptr; // required; not owned unless ownRuntime
    bool ownRuntime = false;
    we::runtime::scene::Scene* scene = nullptr;
    viewportedit::IViewportEditor* viewportEdit = nullptr;
    outliner::IWorldOutlinerRuntime* worldOutliner = nullptr;
    contentbrowser::IContentBrowserRuntime* contentBrowser = nullptr;
    PrefabTransactionFn recordTransaction;
    runtime::PrefabConfig config{};
    std::function<void(std::string_view)> onLog;
};

[[nodiscard]] PREFABEDITOR_API std::unique_ptr<IPrefabEditor> CreatePrefabEditor(
    const PrefabEditorDependencies& deps);

} // namespace we::editor::prefab
