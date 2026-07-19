#pragma once

#include "PropertyEditor/Export.h"
#include "PropertyEditor/IPropertyHandle.h"
#include "PropertyEditor/IPropertyTree.h"
#include "Reflection/TypeId.h"

#include <functional>
#include <memory>

namespace we::editor::property {

class IDetailsView;

/// Whole-type layout override for the Details Panel.
class PROPERTYEDITOR_API IDetailCustomization {
public:
    virtual ~IDetailCustomization() = default;
    virtual void CustomizeDetails(IDetailsView& detailsView) = 0;
};

using DetailCustomizationFactory =
    std::function<std::unique_ptr<IDetailCustomization>()>;

/// Single-property override (replaces the default leaf editor for a type).
class PROPERTYEDITOR_API IPropertyCustomization {
public:
    virtual ~IPropertyCustomization() = default;
    virtual void CustomizeProperty(IPropertyHandle& handle, IPropertyTree& tree) = 0;
};

using PropertyCustomizationFactory =
    std::function<std::unique_ptr<IPropertyCustomization>()>;

} // namespace we::editor::property
