#pragma once

#include "PropertyEditor/Export.h"
#include "PropertyEditor/IDetailsView.h"
#include "PropertyEditor/IPropertyEditorRuntime.h"

#include <memory>

namespace we::editor::property {

/// Editor-session binding for panel factories that cannot take constructor DI.
/// Installed by the Editor host before shell build; cleared on workspace unload.
class PROPERTYEDITOR_API PropertyEditorSession {
public:
    static void Install(
        std::shared_ptr<IPropertyEditorRuntime> runtime,
        std::shared_ptr<IDetailsView> detailsView);

    static void Clear() noexcept;

    [[nodiscard]] static IPropertyEditorRuntime* Runtime() noexcept;
    [[nodiscard]] static IDetailsView* Details() noexcept;
    [[nodiscard]] static std::shared_ptr<IDetailsView> DetailsShared() noexcept;
    [[nodiscard]] static bool IsInstalled() noexcept;
};

} // namespace we::editor::property
