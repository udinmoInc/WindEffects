// ==============================================================================
// WindEffects — PropertyEditor — IPropertyField
// Public API surface for the PropertyEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "PropertyEditor/Export.h"
#include "PropertyEditor/IPropertyHandle.h"
#include "PropertyEditor/PropertyFieldTypes.h"
#include "KindUI/Core/Widget.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace we::editor::property {

/// Central abstract interface for reusable editor fields across all editor tools.
class PROPERTYEDITOR_API IPropertyField {
public:
    virtual ~IPropertyField() = default;

    // Field Identity & Metadata
    [[nodiscard]] virtual FieldType GetFieldType() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetLabel() const noexcept = 0;
    virtual void SetLabel(std::string_view label) = 0;

    [[nodiscard]] virtual std::string_view GetTooltip() const noexcept = 0;
    virtual void SetTooltip(std::string_view tooltip) = 0;

    // State Controls
    [[nodiscard]] virtual bool IsReadOnly() const noexcept = 0;
    virtual void SetReadOnly(bool readOnly) = 0;

    [[nodiscard]] virtual bool IsEnabled() const noexcept = 0;
    virtual void SetEnabled(bool enabled) = 0;

    [[nodiscard]] virtual bool IsModifiedFromDefault() const noexcept = 0;
    virtual void ResetToDefault() = 0;

    // Validation
    [[nodiscard]] virtual ValidationResult Validate() const = 0;
    virtual void SetCustomValidator(std::function<ValidationResult(const void* data)> validator) = 0;

    // Data Binding
    virtual void BindHandle(const PropertyHandlePtr& handle) = 0;
    virtual void BindRaw(void* dataPtr, std::size_t size, std::function<void()> onExternalChange = nullptr) = 0;

    // Presentation Widgets
    /// Creates a standalone editor control (leaf widget) suitable for custom layouts, toolbars, or forms.
    [[nodiscard]] virtual std::shared_ptr<we::runtime::kindui::Widget> CreateWidget() = 0;

    /// Creates a 2-column row widget with label, splitter, value control, and modified reset indicator.
    [[nodiscard]] virtual std::shared_ptr<we::runtime::kindui::Widget> CreateRowWidget(float labelColumnRatio = 0.4f) =
        0;

    // Callbacks & Transaction Hooks
    virtual void SetOnValueChanged(std::function<void()> callback) = 0;
    virtual void SetOnValueCommitted(std::function<void()> callback) = 0;
    virtual void SetTransactionHooks(
        std::function<void(const char* name)> beginTx,
        std::function<void()> endTx) = 0;

    // Drag & Drop / Context Actions
    virtual void SetDragDropTarget(
        std::string_view typeTag,
        std::function<bool(const std::string& payload)> onDrop) = 0;

    virtual void AddContextAction(std::string_view label, std::function<void()> action) = 0;
    [[nodiscard]] virtual const std::vector<ContextAction>& GetContextActions() const noexcept = 0;
};

using PropertyFieldPtr = std::shared_ptr<IPropertyField>;

} // namespace we::editor::property
