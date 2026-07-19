#pragma once

#include "PropertyEditor/Export.h"
#include "PropertyEditor/IPropertyHandle.h"
#include "PropertyEditor/PropertyEditorTypes.h"
#include "Reflection/PropertyInfo.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace we::editor::property {

class IPropertyNode;

using PropertyNodePtr = std::shared_ptr<IPropertyNode>;

/// One node in the property tree (category, property, or array element).
class PROPERTYEDITOR_API IPropertyNode {
public:
    virtual ~IPropertyNode() = default;

    [[nodiscard]] virtual std::string_view GetPath() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetDisplayName() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetCategory() const noexcept = 0;
    [[nodiscard]] virtual const reflection::PropertyInfo* GetPropertyInfo() const noexcept = 0;
    [[nodiscard]] virtual int GetDepth() const noexcept = 0;
    [[nodiscard]] virtual int GetArrayIndex() const noexcept = 0; // -1 if not an array element
    [[nodiscard]] virtual PropertyValueState GetValueState() const noexcept = 0;
    [[nodiscard]] virtual bool IsCategoryNode() const noexcept = 0;
    [[nodiscard]] virtual bool IsExpanded() const noexcept = 0;
    virtual void SetExpanded(bool expanded) = 0;

    [[nodiscard]] virtual const std::vector<PropertyNodePtr>& GetChildren() const noexcept = 0;
    [[nodiscard]] virtual PropertyHandlePtr GetHandle() const = 0;
};

} // namespace we::editor::property
