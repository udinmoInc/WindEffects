#pragma once

#include "PropertyEditor/Export.h"
#include "PropertyEditor/IPropertyHandle.h"
#include "Reflection/PropertyInfo.h"
#include "Reflection/TypeKinds.h"
#include "KindUI/Core/Widget.h"

#include <memory>

namespace we::editor::property {

/// Per-type leaf editor widget contract (KindUI leaf only — not the Details shell).
class PROPERTYEDITOR_API IPropertyEditor {
public:
    virtual ~IPropertyEditor() = default;

    [[nodiscard]] virtual bool CanEdit(const reflection::PropertyInfo& property) const = 0;
    virtual void Bind(const PropertyHandlePtr& handle) = 0;
    [[nodiscard]] virtual std::shared_ptr<we::runtime::kindui::Widget> CreateWidget() = 0;
};

using PropertyEditorPtr = std::shared_ptr<IPropertyEditor>;

} // namespace we::editor::property
