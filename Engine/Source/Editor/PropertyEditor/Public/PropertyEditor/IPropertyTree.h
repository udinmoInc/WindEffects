#pragma once

#include "PropertyEditor/Export.h"
#include "PropertyEditor/IPropertyNode.h"
#include "PropertyEditor/PropertyEditorTypes.h"
#include "Reflection/TypeId.h"

#include <memory>
#include <string_view>
#include <vector>

namespace we::editor::property {

/// Reflection-backed property tree model for one or more instances.
class PROPERTYEDITOR_API IPropertyTree {
public:
    virtual ~IPropertyTree() = default;

    virtual void Build(reflection::TypeId typeId, void* instance) = 0;
    virtual void Build(reflection::TypeId typeId, const std::vector<void*>& instances) = 0;
    virtual void BuildBindings(const std::vector<ObjectBinding>& bindings) = 0;
    virtual void Rebuild() = 0;
    virtual void Clear() = 0;

    virtual void ApplyFilter(const PropertyFilterOptions& filter) = 0;
    [[nodiscard]] virtual const PropertyFilterOptions& GetFilter() const noexcept = 0;

    [[nodiscard]] virtual const std::vector<PropertyNodePtr>& GetRootNodes() const noexcept = 0;
    [[nodiscard]] virtual const std::vector<PropertyNodePtr>& GetFilteredRootNodes() const noexcept = 0;
    [[nodiscard]] virtual PropertyNodePtr FindByPath(std::string_view path) const = 0;

    [[nodiscard]] virtual reflection::TypeId GetTypeId() const noexcept = 0;
    [[nodiscard]] virtual const std::vector<void*>& GetInstances() const noexcept = 0;
};

using PropertyTreePtr = std::shared_ptr<IPropertyTree>;

} // namespace we::editor::property
