#pragma once

#include "PropertyEditor/Export.h"
#include "PropertyEditor/IPropertyEditor.h"
#include "Reflection/PropertyInfo.h"
#include "Reflection/TypeId.h"
#include "Reflection/TypeKinds.h"

#include <functional>
#include <memory>

namespace we::editor::property {

using PropertyEditorFactoryFn = std::function<PropertyEditorPtr()>;

/// Creates leaf editors by PrimitiveKind / TypeKind / TypeId override.
class PROPERTYEDITOR_API IPropertyEditorFactory {
public:
    virtual ~IPropertyEditorFactory() = default;

    [[nodiscard]] virtual PropertyEditorPtr CreateEditor(
        const reflection::PropertyInfo& property,
        const PropertyHandlePtr& handle) = 0;

    virtual void RegisterByPrimitive(reflection::PrimitiveKind kind, PropertyEditorFactoryFn factory) = 0;
    virtual void RegisterByTypeKind(reflection::TypeKind kind, PropertyEditorFactoryFn factory) = 0;
    virtual void RegisterByTypeId(reflection::TypeId typeId, PropertyEditorFactoryFn factory) = 0;
};

} // namespace we::editor::property
