#pragma once

#include "PropertyEditor/Export.h"
#include "Reflection/PropertyInfo.h"
#include "Reflection/TypeId.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace we::editor::property {

/// Fired around property edits (pre/post) for listeners and transaction hooks.
struct PROPERTYEDITOR_API PropertyChangeEvent {
    std::string path;
    reflection::TypeId typeId = reflection::kInvalidTypeId;
    std::vector<void*> instances;
    const reflection::PropertyInfo* property = nullptr;
    std::vector<std::uint8_t> beforeBytes;
    std::vector<std::uint8_t> afterBytes;
    bool isMultiObject = false;
    bool cancelled = false;
};

using PropertyChangeListener = std::function<void(const PropertyChangeEvent&)>;

} // namespace we::editor::property
