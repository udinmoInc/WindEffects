// ==============================================================================
// WindEffects — PropertyEditor — PropertyFieldTypes
// Public API surface for the PropertyEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "PropertyEditor/Export.h"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace we::editor::property {

/// Supported field categories in the central reusable PropertyField API.
enum class FieldType : std::uint8_t {
    Bool = 0,
    Int32,
    Int64,
    Float,
    Double,
    String,
    Text,
    Enum,
    Flags,
    Vector2,
    Vector3,
    Vector4,
    Rotation,
    Transform,
    Color,
    Gradient,
    AssetRef,
    ObjectRef,
    ClassRef,
    Array,
    List,
    Set,
    Map,
    Curve,
    RangeSlider,
    Layer,
    Tag,
};

/// Severity levels for property validation.
enum class ValidationSeverity : std::uint8_t {
    Ok = 0,
    Warning,
    Error,
};

/// Result of property field validation check.
struct PROPERTYEDITOR_API ValidationResult {
    bool isValid = true;
    ValidationSeverity severity = ValidationSeverity::Ok;
    std::string message;

    static ValidationResult Success() { return ValidationResult{ true, ValidationSeverity::Ok, "" }; }
    static ValidationResult Warn(std::string msg) { return ValidationResult{ true, ValidationSeverity::Warning,
        std::move(msg) }; }
    static ValidationResult Fail(std::string msg) { return ValidationResult{ false, ValidationSeverity::Error,
        std::move(msg) }; }
};

/// Custom context action for right-click context menu or toolbar action.
struct PROPERTYEDITOR_API ContextAction {
    std::string id;
    std::string label;
    std::function<void()> action;
};

} // namespace we::editor::property
