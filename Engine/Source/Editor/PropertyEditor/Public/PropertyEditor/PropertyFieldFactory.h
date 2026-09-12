// ==============================================================================
// WindEffects — PropertyEditor — PropertyFieldFactory
// Public API surface for the PropertyEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "PropertyEditor/Export.h"
#include "PropertyEditor/IPropertyField.h"
#include "PropertyEditor/IPropertyHandle.h"
#include "PropertyEditor/PropertyFieldTypes.h"
#include "Reflection/TypeId.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace we::editor::property {

/// Factory for instantiating reusable PropertyField models across all editor tools.
class PROPERTYEDITOR_API PropertyFieldFactory {
public:
    static PropertyFieldPtr CreateBoolField(std::string_view label = "");
    static PropertyFieldPtr CreateNumericField(FieldType numericType, std::string_view label = "");
    static PropertyFieldPtr CreateRangeSliderField(double minVal, double maxVal, double step = 0.1,
        std::string_view label = "");
    static PropertyFieldPtr CreateStringField(std::string_view label = "", bool multiline = false);
    static PropertyFieldPtr CreateEnumField(std::vector<std::string> options, std::string_view label = "");
    static PropertyFieldPtr CreateFlagsField(std::vector<std::pair<std::uint32_t, std::string>> flagBits,
        std::string_view label = "");
    static PropertyFieldPtr CreateVectorField(int components, std::string_view label = "");
    static PropertyFieldPtr CreateRotationField(std::string_view label = "");
    static PropertyFieldPtr CreateTransformField(std::string_view label = "");
    static PropertyFieldPtr CreateColorField(bool includeAlpha = true, std::string_view label = "");
    static PropertyFieldPtr CreateGradientField(std::string_view label = "");
    static PropertyFieldPtr CreateAssetRefField(std::string_view allowedExtension = "", std::string_view label = "");
    static PropertyFieldPtr CreateObjectRefField(reflection::TypeId typeId = reflection::kInvalidTypeId,
        std::string_view label = "");
    static PropertyFieldPtr CreateClassRefField(reflection::TypeId baseClassId = reflection::kInvalidTypeId,
        std::string_view label = "");
    static PropertyFieldPtr CreateContainerField(FieldType containerType, PropertyFieldPtr elementTemplate = nullptr,
        std::string_view label = "");
    static PropertyFieldPtr CreateCurveField(std::string_view label = "");
    static PropertyFieldPtr CreateTagLayerField(bool isLayerMode = false, std::string_view label = "");

    /// Auto-detects field type and creates a bound PropertyField from an IPropertyHandle.
    static PropertyFieldPtr CreateFromHandle(const PropertyHandlePtr& handle);
};

} // namespace we::editor::property
