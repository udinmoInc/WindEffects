#pragma once

#include "PropertyEditor/Export.h"
#include "Reflection/TypeId.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::editor::property {

/// How a property value compares across multi-object selection.
enum class PropertyValueState : std::uint8_t {
    Identical = 0,
    Mixed,
    Unavailable,
};

/// Options controlling which properties appear in the tree / details filter.
struct PROPERTYEDITOR_API PropertyFilterOptions {
    bool editableOnly = true;
    bool skipHidden = true;
    bool skipTransient = false;
    bool showAdvanced = false;
    std::string searchText;
    std::vector<std::string> categoryAllowlist; // empty = all categories
};

/// Category display metadata for details UI.
struct PROPERTYEDITOR_API PropertyCategoryInfo {
    std::string name;
    bool expanded = true;
};

/// Optional edit-condition: enable the editor when another bool property matches.
struct PROPERTYEDITOR_API EditCondition {
    std::string propertyPath;
    bool expectedValue = true;
};

/// Single live object binding (instance not owned by PropertyEditor).
struct PROPERTYEDITOR_API ObjectBinding {
    reflection::TypeId typeId = reflection::kInvalidTypeId;
    void* instance = nullptr;
};

} // namespace we::editor::property
