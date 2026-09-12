// ==============================================================================
// WindEffects — PropertyEditor — PropertyFieldImpl
// Internal implementation for the PropertyEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "PropertyEditor/IPropertyField.h"
#include "PropertyEditor/PropertyFieldFactory.h"
#include "PropertyEditor/PropertyFieldTypes.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/PropertyPanelChrome.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace we::editor::property::detail {

class PropertyFieldBase : public IPropertyField, public std::enable_shared_from_this<PropertyFieldBase> {
public:
    explicit PropertyFieldBase(FieldType type, std::string_view label = "");
    ~PropertyFieldBase() override = default;

    // Field Identity & Metadata
    [[nodiscard]] FieldType GetFieldType() const noexcept override { return m_Type; }
    [[nodiscard]] std::string_view GetLabel() const noexcept override { return m_Label; }
    void SetLabel(std::string_view label) override { m_Label = label; }

    [[nodiscard]] std::string_view GetTooltip() const noexcept override { return m_Tooltip; }
    void SetTooltip(std::string_view tooltip) override { m_Tooltip = tooltip; }

    // State Controls
    [[nodiscard]] bool IsReadOnly() const noexcept override { return m_ReadOnly || (m_Handle &&
        m_Handle->IsReadOnly()); }
    void SetReadOnly(bool readOnly) override { m_ReadOnly = readOnly; }

    [[nodiscard]] bool IsEnabled() const noexcept override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }

    [[nodiscard]] bool IsModifiedFromDefault() const noexcept override { return m_IsModified; }
    void ResetToDefault() override;

    // Validation
    [[nodiscard]] ValidationResult Validate() const override;
    void SetCustomValidator(std::function<ValidationResult(const void* data)> validator) override { m_Validator =
        std::move(validator); }

    // Data Binding
    void BindHandle(const PropertyHandlePtr& handle) override;
    void BindRaw(void* dataPtr, std::size_t size, std::function<void()> onExternalChange = nullptr) override;

    // Row presentation
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> CreateRowWidget(float labelColumnRatio = 0.4f) override;

    // Callbacks & Transactions
    void SetOnValueChanged(std::function<void()> callback) override { m_OnValueChanged = std::move(callback); }
    void SetOnValueCommitted(std::function<void()> callback) override { m_OnValueCommitted = std::move(callback); }
    void SetTransactionHooks(std::function<void(const char* name)> beginTx, std::function<void()> endTx) override {
        m_BeginTx = std::move(beginTx);
        m_EndTx = std::move(endTx);
    }

    // Drag & Drop / Context
    void SetDragDropTarget(std::string_view typeTag, std::function<bool(const std::string& payload)> onDrop) override {
        m_DropTypeTag = typeTag;
        m_OnDropPayload = std::move(onDrop);
    }

    void AddContextAction(std::string_view label, std::function<void()> action) override {
        m_ContextActions.push_back({ std::string(label), std::string(label), std::move(action) });
    }

    [[nodiscard]] const std::vector<ContextAction>& GetContextActions() const noexcept override {
        return m_ContextActions; }

protected:
    void NotifyValueChanged();
    void NotifyValueCommitted();
    void BeginTransaction(const char* name);
    void EndTransaction();

    FieldType m_Type;
    std::string m_Label;
    std::string m_Tooltip;
    bool m_ReadOnly = false;
    bool m_Enabled = true;
    bool m_IsModified = false;

    PropertyHandlePtr m_Handle;
    void* m_RawDataPtr = nullptr;
    std::size_t m_RawDataSize = 0;
    std::function<void()> m_RawOnExternalChange;

    std::function<ValidationResult(const void* data)> m_Validator;
    std::function<void()> m_OnValueChanged;
    std::function<void()> m_OnValueCommitted;
    std::function<void(const char* name)> m_BeginTx;
    std::function<void()> m_EndTx;

    std::string m_DropTypeTag;
    std::function<bool(const std::string& payload)> m_OnDropPayload;
    std::vector<ContextAction> m_ContextActions;
};

// --- Specialized Concrete Field Classes ---

class BoolPropertyField final : public PropertyFieldBase {
public:
    explicit BoolPropertyField(std::string_view label = "");
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> CreateWidget() override;
    [[nodiscard]] bool GetValue() const;
    void SetValue(bool val);
private:
    bool m_Value = false;
    bool m_DefaultValue = false;
};

class NumericPropertyField final : public PropertyFieldBase {
public:
    NumericPropertyField(FieldType numType, std::string_view label = "");
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> CreateWidget() override;

    void SetBounds(double minVal, double maxVal, double step = 0.1);
    [[nodiscard]] double GetDoubleValue() const;
    void SetDoubleValue(double val);
private:
    double m_Value = 0.0;
    double m_DefaultValue = 0.0;
    double m_Min = -1e9;
    double m_Max = 1e9;
    double m_Step = 0.1;
    bool m_HasBounds = false;
};

class StringPropertyField final : public PropertyFieldBase {
public:
    StringPropertyField(std::string_view label = "", bool multiline = false);
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> CreateWidget() override;
    [[nodiscard]] std::string GetValue() const;
    void SetValue(std::string val);
private:
    std::string m_Value;
    std::string m_DefaultValue;
    bool m_Multiline = false;
};

class EnumPropertyField final : public PropertyFieldBase {
public:
    EnumPropertyField(std::vector<std::string> options, std::string_view label = "");
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> CreateWidget() override;
    [[nodiscard]] int GetSelectedIndex() const { return m_SelectedIndex; }
    void SetSelectedIndex(int index);
private:
    std::vector<std::string> m_Options;
    int m_SelectedIndex = 0;
    int m_DefaultIndex = 0;
};

class FlagsPropertyField final : public PropertyFieldBase {
public:
    FlagsPropertyField(std::vector<std::pair<std::uint32_t, std::string>> flagBits, std::string_view label = "");
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> CreateWidget() override;
    [[nodiscard]] std::uint32_t GetMask() const { return m_Mask; }
    void SetMask(std::uint32_t mask);
private:
    std::vector<std::pair<std::uint32_t, std::string>> m_FlagBits;
    std::uint32_t m_Mask = 0;
    std::uint32_t m_DefaultMask = 0;
};

class VectorPropertyField final : public PropertyFieldBase {
public:
    VectorPropertyField(int components, std::string_view label = "");
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> CreateWidget() override;
    [[nodiscard]] double GetComponent(int index) const;
    void SetComponent(int index, double val);
    [[nodiscard]] int GetComponentCount() const { return m_ComponentCount; }
private:
    int m_ComponentCount = 3;
    double m_Values[4] = { 0.0, 0.0, 0.0, 0.0 };
    double m_DefaultValues[4] = { 0.0, 0.0, 0.0, 0.0 };
};

class RotationPropertyField final : public PropertyFieldBase {
public:
    explicit RotationPropertyField(std::string_view label = "");
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> CreateWidget() override;
private:
    double m_Pitch = 0.0, m_Yaw = 0.0, m_Roll = 0.0;
};

class TransformPropertyField final : public PropertyFieldBase {
public:
    explicit TransformPropertyField(std::string_view label = "");
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> CreateWidget() override;
};

class ColorPropertyField final : public PropertyFieldBase {
public:
    ColorPropertyField(bool includeAlpha, std::string_view label = "");
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> CreateWidget() override;
private:
    float m_RGBA[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool m_IncludeAlpha = true;
};

class GradientPropertyField final : public PropertyFieldBase {
public:
    explicit GradientPropertyField(std::string_view label = "");
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> CreateWidget() override;
};

class AssetRefPropertyField final : public PropertyFieldBase {
public:
    AssetRefPropertyField(std::string_view allowedExtension = "", std::string_view label = "");
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> CreateWidget() override;
private:
    std::string m_AssetPath;
    std::string m_AllowedExtension;
};

class ObjectRefPropertyField final : public PropertyFieldBase {
public:
    ObjectRefPropertyField(reflection::TypeId typeId, std::string_view label = "");
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> CreateWidget() override;
private:
    reflection::TypeId m_TypeId = reflection::kInvalidTypeId;
    std::string m_ObjectName;
};

class ClassRefPropertyField final : public PropertyFieldBase {
public:
    ClassRefPropertyField(reflection::TypeId baseClassId, std::string_view label = "");
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> CreateWidget() override;
private:
    reflection::TypeId m_BaseClassId = reflection::kInvalidTypeId;
    std::string m_ClassName;
};

class ContainerPropertyField final : public PropertyFieldBase {
public:
    ContainerPropertyField(FieldType containerType, PropertyFieldPtr elementTemplate, std::string_view label = "");
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> CreateWidget() override;
private:
    PropertyFieldPtr m_ElementTemplate;
    std::vector<PropertyFieldPtr> m_Elements;
};

class CurvePropertyField final : public PropertyFieldBase {
public:
    explicit CurvePropertyField(std::string_view label = "");
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> CreateWidget() override;
};

class TagLayerPropertyField final : public PropertyFieldBase {
public:
    TagLayerPropertyField(bool isLayerMode, std::string_view label = "");
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> CreateWidget() override;
private:
    bool m_IsLayerMode = false;
    std::vector<std::string> m_Tags;
};

} // namespace we::editor::property::detail
