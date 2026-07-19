#pragma once

#include "PrefabEditor/Export.h"
#include "PrefabEditor/IPrefabEditor.h"

#include <memory>

namespace we::editor::prefab {

/// Editor-session binding for tools/panels that cannot take constructor DI.
class PREFABEDITOR_API PrefabSession {
public:
    static void Install(std::shared_ptr<IPrefabEditor> editor);
    static void Clear() noexcept;

    [[nodiscard]] static IPrefabEditor* Editor() noexcept;
    [[nodiscard]] static runtime::IPrefabRuntime* Runtime() noexcept;
    [[nodiscard]] static std::shared_ptr<IPrefabEditor> EditorShared() noexcept;
    [[nodiscard]] static bool IsInstalled() noexcept;
};

} // namespace we::editor::prefab
