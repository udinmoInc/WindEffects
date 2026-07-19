#pragma once

#include "Undo/Export.h"
#include "Undo/ITransactionManager.h"
#include "Undo/UndoTypes.h"

#include <functional>
#include <memory>
#include <string_view>

namespace we::runtime::reflection {
class ITypeRegistry;
}

namespace we::runtime::serialization {
class ISerializer;
}

namespace we::editor::property {
class IPropertyTransactionHook;
}

namespace we::runtime::world {
class IEditorWorldHook;
}

namespace we::editor::undo {

struct UNDO_API UndoDependencies {
    reflection::ITypeRegistry* typeRegistry = nullptr;     // not owned; uses global if null
    serialization::ISerializer* serializer = nullptr;      // not owned; optional but recommended
    HistoryConfig history{};
    std::function<void(std::string_view)> onLog;
    bool installDefaultMergePolicy = true;
};

/// Top-level Undo/Redo runtime — owns the transaction manager and integration adapters.
class UNDO_API IUndoRuntime {
public:
    virtual ~IUndoRuntime() = default;

    [[nodiscard]] virtual ITransactionManager& Manager() noexcept = 0;
    [[nodiscard]] virtual const ITransactionManager& Manager() const noexcept = 0;

    /// Shared adapters for PropertyEditor / World Runtime DI.
    [[nodiscard]] virtual std::shared_ptr<property::IPropertyTransactionHook>
    MakePropertyTransactionHook() = 0;

    [[nodiscard]] virtual std::shared_ptr<world::IEditorWorldHook>
    MakeEditorWorldHook() = 0;

    virtual void Shutdown() = 0;
};

[[nodiscard]] UNDO_API std::unique_ptr<IUndoRuntime> CreateUndoRuntime(UndoDependencies deps = {});

} // namespace we::editor::undo
