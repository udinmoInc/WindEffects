// ==============================================================================
// WindEffects — PropertyEditor — PropertyEditorInternal
// Internal implementation for the PropertyEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "PropertyEditor/IDetailsView.h"
#include "PropertyEditor/IPropertyEditorFactory.h"
#include "PropertyEditor/IPropertyEditorRuntime.h"
#include "PropertyEditor/IPropertyHandle.h"
#include "PropertyEditor/IPropertyNode.h"
#include "PropertyEditor/IPropertyTree.h"
#include "PropertyEditor/PropertyEditorDependencies.h"

#include "Reflection/IPropertyAccessor.h"
#include "Reflection/ITypeRegistry.h"
#include "Reflection/PropertyIterator.h"
#include "Reflection/PropertyPath.h"
#include "Reflection/Reflection.h"
#include "Serialization/Delta.h"
#include "Serialization/ISerializer.h"

#include <KindUI/EditorUI.h>
#include <algorithm>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace we::editor::property {
namespace detail {

using reflection::IPropertyAccessor;
using reflection::ITypeRegistry;
using reflection::PropertyFlags;
using reflection::PropertyInfo;
using reflection::PropertyIterateOptions;
using reflection::TypeId;
using reflection::TypeInfo;
using reflection::TypeKind;
using reflection::PrimitiveKind;
using reflection::HasFlag;

inline std::string CategoryFromProperty(const PropertyInfo& property) {
    if (const auto* attr = property.attributes.Find("Category")) {
        if (!attr->stringValue.empty()) {
            return attr->stringValue;
        }
    }
    return "General";
}

inline bool PassesSearch(std::string_view name, std::string_view category, std::string_view search) {
    if (search.empty()) {
        return true;
    }
    auto contains = [](std::string_view hay, std::string_view needle) {
        if (needle.empty()) {
            return true;
        }
        std::string h(hay);
        std::string n(needle);
        std::transform(h.begin(), h.end(), h.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c)); });
        std::transform(n.begin(), n.end(), n.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c)); });
        return h.find(n) != std::string::npos;
    };
    return contains(name, search) || contains(category, search);
}

inline bool CategoryAllowed(const PropertyFilterOptions& filter, std::string_view category) {
    if (filter.categoryAllowlist.empty()) {
        return true;
    }
    for (const auto& allowed : filter.categoryAllowlist) {
        if (allowed == category) {
            return true;
        }
    }
    return false;
}

class PropertyHandleImpl;
class PropertyNodeImpl;
class PropertyTreeImpl;
class PropertyEditorFactoryImpl;

struct RuntimeServices {
    ITypeRegistry* registry = nullptr;
    IPropertyAccessor* accessor = nullptr;
    serialization::ISerializer* serializer = nullptr;
    std::shared_ptr<IPropertyTransactionHook> transactionHook;
    std::function<void(std::string_view)> onLog;
    IPropertyEditorFactory* factory = nullptr;
    IPropertyEditorRuntime* runtime = nullptr;
};

[[nodiscard]] std::shared_ptr<IPropertyTree> CreatePropertyTree(RuntimeServices services);
[[nodiscard]] std::unique_ptr<IDetailsView> CreateDetailsView(RuntimeServices services);
[[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> CreateSubOutlinerWidget(IDetailsView* details);
void PopulateDetailsPanelRegions(
    we::editor::dsl::PanelContext& p,
    const std::shared_ptr<we::runtime::kindui::Widget>& propertyList,
    IDetailsView* details);
[[nodiscard]] std::unique_ptr<IPropertyEditorFactory> CreateEditorFactory(
    ITypeRegistry* registry,
    bool registerDefaults);
void RegisterDefaultEditors(IPropertyEditorFactory& factory);

}
}
