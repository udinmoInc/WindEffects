#pragma once

#include "PropertyEditor/Export.h"
#include "PropertyEditor/PropertyEditorTypes.h"
#include "Reflection/TypeId.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::reflection {
class ITypeRegistry;
}

namespace we::editor::property {

/// Multi-object edit binding — all instances share the same reflected type.
struct PROPERTYEDITOR_API MultiObjectBinding {
    reflection::TypeId typeId = reflection::kInvalidTypeId;
    std::vector<void*> instances;
};

/// Merged state for one property path across the selection.
struct PROPERTYEDITOR_API MultiObjectPropertyState {
    std::string path;
    PropertyValueState valueState = PropertyValueState::Unavailable;
    std::vector<std::uint8_t> commonBytes; // valid when Identical
    std::uint32_t propertySize = 0;
};

/// Intersect editable properties across instances; mark mixed values.
[[nodiscard]] PROPERTYEDITOR_API std::vector<MultiObjectPropertyState> MergeCommonProperties(
    const reflection::ITypeRegistry& registry,
    const MultiObjectBinding& binding,
    const PropertyFilterOptions& filter = {});

} // namespace we::editor::property
