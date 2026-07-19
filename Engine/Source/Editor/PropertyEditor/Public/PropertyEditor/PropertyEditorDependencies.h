#pragma once

#include "PropertyEditor/Export.h"
#include "PropertyEditor/IPropertyTransactionHook.h"

#include <functional>
#include <memory>
#include <string_view>

namespace we::runtime::reflection {
class ITypeRegistry;
class IPropertyAccessor;
}

namespace we::runtime::serialization {
class ISerializer;
}

namespace we::editor::property {

/// Explicit dependency injection for PropertyEditor Runtime construction.
struct PROPERTYEDITOR_API PropertyEditorDependencies {
    reflection::ITypeRegistry* typeRegistry = nullptr;           // not owned; uses global if null
    reflection::IPropertyAccessor* propertyAccessor = nullptr;   // not owned; runtime creates if null
    serialization::ISerializer* serializer = nullptr;            // optional; not owned
    std::shared_ptr<IPropertyTransactionHook> transactionHook;
    std::function<void(std::string_view)> onLog;
    bool registerDefaultEditors = true;
};

} // namespace we::editor::property
