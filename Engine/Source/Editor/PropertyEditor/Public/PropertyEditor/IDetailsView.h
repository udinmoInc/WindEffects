#pragma once

#include "PropertyEditor/Export.h"
#include "PropertyEditor/IDetailCustomization.h"
#include "PropertyEditor/IPropertyTree.h"
#include "PropertyEditor/PropertyChangeEvent.h"
#include "PropertyEditor/PropertyEditorTypes.h"
#include "Reflection/TypeId.h"
#include "KindUI/Core/Widget.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace we::editor::property {

/// Details Panel model + host widget for reflected object editing.
class PROPERTYEDITOR_API IDetailsView {
public:
    virtual ~IDetailsView() = default;

    virtual void SetObject(reflection::TypeId typeId, void* instance) = 0;
    virtual void SetObjects(reflection::TypeId typeId, const std::vector<void*>& instances) = 0;
    virtual void Clear() = 0;

    virtual void SetFilter(const PropertyFilterOptions& filter) = 0;
    virtual void SetSearchText(std::string_view text) = 0;
    virtual void SetShowAdvanced(bool show) = 0;

    virtual void ExpandAll() = 0;
    virtual void CollapseAll() = 0;
    virtual void SetCategoryExpanded(std::string_view category, bool expanded) = 0;

    [[nodiscard]] virtual IPropertyTree& GetTree() noexcept = 0;
    [[nodiscard]] virtual const IPropertyTree& GetTree() const noexcept = 0;
    [[nodiscard]] virtual std::shared_ptr<we::runtime::kindui::Widget> GetWidget() = 0;

    virtual void AddChangeListener(PropertyChangeListener listener) = 0;
};

using DetailsViewPtr = std::shared_ptr<IDetailsView>;

} // namespace we::editor::property
