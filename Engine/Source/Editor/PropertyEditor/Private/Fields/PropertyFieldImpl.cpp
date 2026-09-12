// ==============================================================================
// WindEffects — PropertyEditor — PropertyFieldImpl
// Internal implementation for the PropertyEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Fields/PropertyFieldImpl.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/PropertyPanelChrome.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Widgets/TextBox.h"
#include "KindUI/Widgets/Label.h"
#include "KindUI/Widgets/ColorPicker.h"
#include "KindUI/Theming/ThemeAccess.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace we::editor::property {

// Factory Implementation
PropertyFieldPtr PropertyFieldFactory::CreateBoolField(std::string_view label) {
    return std::make_shared<detail::BoolPropertyField>(label);
}

PropertyFieldPtr PropertyFieldFactory::CreateNumericField(FieldType numericType, std::string_view label) {
    return std::make_shared<detail::NumericPropertyField>(numericType, label);
}

PropertyFieldPtr PropertyFieldFactory::CreateRangeSliderField(double minVal, double maxVal, double step,
    std::string_view label) {
    auto field = std::make_shared<detail::NumericPropertyField>(FieldType::RangeSlider, label);
    field->SetBounds(minVal, maxVal, step);
    return field;
}

PropertyFieldPtr PropertyFieldFactory::CreateStringField(std::string_view label, bool multiline) {
    return std::make_shared<detail::StringPropertyField>(label, multiline);
}

PropertyFieldPtr PropertyFieldFactory::CreateEnumField(std::vector<std::string> options, std::string_view label) {
    return std::make_shared<detail::EnumPropertyField>(std::move(options), label);
}

PropertyFieldPtr PropertyFieldFactory::CreateFlagsField(std::vector<std::pair<std::uint32_t, std::string>> flagBits,
    std::string_view label) {
    return std::make_shared<detail::FlagsPropertyField>(std::move(flagBits), label);
}

PropertyFieldPtr PropertyFieldFactory::CreateVectorField(int components, std::string_view label) {
    return std::make_shared<detail::VectorPropertyField>(components, label);
}

PropertyFieldPtr PropertyFieldFactory::CreateRotationField(std::string_view label) {
    return std::make_shared<detail::RotationPropertyField>(label);
}

PropertyFieldPtr PropertyFieldFactory::CreateTransformField(std::string_view label) {
    return std::make_shared<detail::TransformPropertyField>(label);
}

PropertyFieldPtr PropertyFieldFactory::CreateColorField(bool includeAlpha, std::string_view label) {
    return std::make_shared<detail::ColorPropertyField>(includeAlpha, label);
}

PropertyFieldPtr PropertyFieldFactory::CreateGradientField(std::string_view label) {
    return std::make_shared<detail::GradientPropertyField>(label);
}

PropertyFieldPtr PropertyFieldFactory::CreateAssetRefField(std::string_view allowedExtension, std::string_view label) {
    return std::make_shared<detail::AssetRefPropertyField>(allowedExtension, label);
}

PropertyFieldPtr PropertyFieldFactory::CreateObjectRefField(reflection::TypeId typeId, std::string_view label) {
    return std::make_shared<detail::ObjectRefPropertyField>(typeId, label);
}

PropertyFieldPtr PropertyFieldFactory::CreateClassRefField(reflection::TypeId baseClassId, std::string_view label) {
    return std::make_shared<detail::ClassRefPropertyField>(baseClassId, label);
}

PropertyFieldPtr PropertyFieldFactory::CreateContainerField(FieldType containerType, PropertyFieldPtr elementTemplate,
    std::string_view label) {
    return std::make_shared<detail::ContainerPropertyField>(containerType, std::move(elementTemplate), label);
}

PropertyFieldPtr PropertyFieldFactory::CreateCurveField(std::string_view label) {
    return std::make_shared<detail::CurvePropertyField>(label);
}

PropertyFieldPtr PropertyFieldFactory::CreateTagLayerField(bool isLayerMode, std::string_view label) {
    return std::make_shared<detail::TagLayerPropertyField>(isLayerMode, label);
}

PropertyFieldPtr PropertyFieldFactory::CreateFromHandle(const PropertyHandlePtr& handle) {
    if (!handle || !handle->GetPropertyInfo()) {
        return nullptr;
    }
    const auto* info = handle->GetPropertyInfo();
    std::string_view labelStr(info->name);
    PropertyFieldPtr field;

    if (info->primitive != reflection::PrimitiveKind::None) {
        switch (info->primitive) {
        case reflection::PrimitiveKind::Bool:
            field = CreateBoolField(labelStr);
            break;
        case reflection::PrimitiveKind::Int32:
        case reflection::PrimitiveKind::UInt32:
        case reflection::PrimitiveKind::Int16:
        case reflection::PrimitiveKind::UInt16:
        case reflection::PrimitiveKind::Int8:
        case reflection::PrimitiveKind::UInt8:
            field = CreateNumericField(FieldType::Int32, labelStr);
            break;
        case reflection::PrimitiveKind::Int64:
        case reflection::PrimitiveKind::UInt64:
            field = CreateNumericField(FieldType::Int64, labelStr);
            break;
        case reflection::PrimitiveKind::Float:
            field = CreateNumericField(FieldType::Float, labelStr);
            break;
        case reflection::PrimitiveKind::Double:
            field = CreateNumericField(FieldType::Double, labelStr);
            break;
        case reflection::PrimitiveKind::String:
            field = CreateStringField(labelStr, false);
            break;
        case reflection::PrimitiveKind::Vec2:
            field = CreateVectorField(2, labelStr);
            break;
        case reflection::PrimitiveKind::Vec3:
            if (labelStr == "Rotation") {
                field = CreateRotationField(labelStr);
            } else if (labelStr.find("Color") != std::string_view::npos || labelStr.find("color") !=
                std::string_view::npos) {
                field = CreateColorField(false, labelStr);
            } else {
                field = CreateVectorField(3, labelStr);
            }
            break;
        case reflection::PrimitiveKind::Vec4:
        case reflection::PrimitiveKind::Quat:
            if (labelStr.find("Color") != std::string_view::npos || labelStr.find("color") != std::string_view::npos) {
                field = CreateColorField(true, labelStr);
            } else {
                field = CreateVectorField(4, labelStr);
            }
            break;
        default:
            field = CreateStringField(labelStr, false);
            break;
        }
    } else {
        field = CreateStringField(labelStr, false);
    }

    if (field) {
        field->BindHandle(handle);
    }
    return field;
}

namespace detail {

PropertyFieldBase::PropertyFieldBase(FieldType type, std::string_view label)
    : m_Type(type), m_Label(std::string(label)) {}

void PropertyFieldBase::BindHandle(const PropertyHandlePtr& handle) {
    m_Handle = handle;
    if (m_Handle && m_Label.empty() && m_Handle->GetPropertyInfo()) {
        m_Label = std::string(m_Handle->GetPropertyInfo()->name);
    }
}

void PropertyFieldBase::BindRaw(void* dataPtr, std::size_t size, std::function<void()> onExternalChange) {
    m_RawDataPtr = dataPtr;
    m_RawDataSize = size;
    m_RawOnExternalChange = std::move(onExternalChange);
}

void PropertyFieldBase::ResetToDefault() {
    m_IsModified = false;
    NotifyValueChanged();
}

ValidationResult PropertyFieldBase::Validate() const {
    if (m_Validator) {
        return m_Validator(m_RawDataPtr);
    }
    return ValidationResult::Success();
}

void PropertyFieldBase::NotifyValueChanged() {
    m_IsModified = true;
    if (m_OnValueChanged) {
        m_OnValueChanged();
    }
}

void PropertyFieldBase::NotifyValueCommitted() {
    if (m_OnValueCommitted) {
        m_OnValueCommitted();
    }
}

void PropertyFieldBase::BeginTransaction(const char* name) {
    if (m_BeginTx) {
        m_BeginTx(name);
    }
}

void PropertyFieldBase::EndTransaction() {
    if (m_EndTx) {
        m_EndTx();
    }
}

// Composite 2-column row widget wrapping leaf widget
std::shared_ptr<we::runtime::kindui::Widget> PropertyFieldBase::CreateRowWidget(float labelColumnRatio) {
    class PropertyRowWidget final : public we::runtime::kindui::Widget {
    public:
        PropertyRowWidget(PropertyFieldBase* owner, std::shared_ptr<we::runtime::kindui::Widget> leaf, float ratio)
            : m_Owner(owner), m_LeafWidget(std::move(leaf)), m_Ratio(ratio) {}

        we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& available) override {
            const float h = we::runtime::kindui::LayoutMetrics::PropertyControlHeight();
            return we::runtime::kindui::Size{ available.width, h };
        }

        void Arrange(const we::runtime::kindui::Rect& r) override {
            m_Geometry = r;
            const auto layout = we::runtime::kindui::PropertyPanelChrome::LayoutPropertyRow(r, 0, {}, false, m_Ratio);
            if (m_LeafWidget) {
                const auto controlRect = we::runtime::kindui::LayoutMetrics::LayoutPropertyControlInRow(layout.value);
                m_LeafWidget->Arrange(controlRect);
            }
        }

        void Paint(we::runtime::kindui::PaintContext& context) override {
            const auto layout = we::runtime::kindui::PropertyPanelChrome::LayoutPropertyRow(m_Geometry, 0, {}, false,
                m_Ratio);
            we::runtime::kindui::PropertyPanelChrome::PaintPropertyRowBackground(context, m_Geometry, m_IsHovered,
                false);
            we::runtime::kindui::PropertyPanelChrome::PaintPropertyRowLabel(
                context, layout.label, m_Owner->GetLabel(), false);
            if (m_LeafWidget) {
                m_LeafWidget->Paint(context);
            }
        }

        void OnMouseMove(const we::runtime::kindui::MouseEvent& e) override {
            (void)e;
            m_IsHovered = true;
        }

        void OnHoverLost() override {
            m_IsHovered = false;
        }

    private:
        PropertyFieldBase* m_Owner = nullptr;
        std::shared_ptr<we::runtime::kindui::Widget> m_LeafWidget;
        float m_Ratio = 0.4f;
        bool m_IsHovered = false;
    };

    return std::make_shared<PropertyRowWidget>(this, CreateWidget(), labelColumnRatio);
}

// --- Bool ---
BoolPropertyField::BoolPropertyField(std::string_view label)
    : PropertyFieldBase(FieldType::Bool, label) {}

std::shared_ptr<we::runtime::kindui::Widget> BoolPropertyField::CreateWidget() {
    class BoolWidget final : public we::runtime::kindui::Widget {
    public:
        explicit BoolWidget(BoolPropertyField* owner) : m_Owner(owner) {}
        we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& available) override {
            return we::runtime::kindui::Size{ available.width,
                we::runtime::kindui::LayoutMetrics::PropertyControlHeight() };
        }
        void Arrange(const we::runtime::kindui::Rect& r) override { m_Geometry = r; }
        void Paint(we::runtime::kindui::PaintContext& context) override {
            const auto frame = we::runtime::kindui::LayoutMetrics::LayoutPropertyControlInRow(m_Geometry);
            we::runtime::kindui::ControlChrome::InteractionState state;
            state.hoverAnim = m_IsHovered ? 1.0f : 0.0f;
            state.pressAnim = m_IsPressed ? 1.0f : 0.0f;
            we::runtime::kindui::ControlChrome::PaintCheckbox(context, frame, m_Owner->GetValue(), state);
        }
        void OnMouseDown(const we::runtime::kindui::MouseEvent& e) override {
            if (e.button == we::runtime::kindui::MouseButton::Left && !m_Owner->IsReadOnly()) {
                m_IsPressed = true;
            }
        }
        void OnMouseUp(const we::runtime::kindui::MouseEvent& e) override {
            if (e.button == we::runtime::kindui::MouseButton::Left && m_IsPressed) {
                m_IsPressed = false;
                m_Owner->SetValue(!m_Owner->GetValue());
            }
        }
    private:
        BoolPropertyField* m_Owner = nullptr;
        bool m_IsHovered = false;
        bool m_IsPressed = false;
    };
    return std::make_shared<BoolWidget>(this);
}

bool BoolPropertyField::GetValue() const {
    if (m_Handle) {
        bool val = false;
        if (m_Handle->GetBool(val)) return val;
    }
    return m_Value;
}

void BoolPropertyField::SetValue(bool val) {
    if (m_Handle) {
        (void)m_Handle->SetBool(val);
    }
    m_Value = val;
    NotifyValueChanged();
    NotifyValueCommitted();
}

// --- Numeric ---
NumericPropertyField::NumericPropertyField(FieldType numType, std::string_view label)
    : PropertyFieldBase(numType, label) {}

void NumericPropertyField::SetBounds(double minVal, double maxVal, double step) {
    m_Min = minVal;
    m_Max = maxVal;
    m_Step = step;
    m_HasBounds = true;
}

double NumericPropertyField::GetDoubleValue() const {
    if (m_Handle) {
        double dVal = 0.0;
        if (m_Handle->GetDouble(dVal)) return dVal;
        float fVal = 0.0f;
        if (m_Handle->GetFloat(fVal)) return static_cast<double>(fVal);
        std::int32_t iVal = 0;
        if (m_Handle->GetInt32(iVal)) return static_cast<double>(iVal);
    }
    return m_Value;
}

void NumericPropertyField::SetDoubleValue(double val) {
    if (m_HasBounds) {
        val = std::clamp(val, m_Min, m_Max);
    }
    if (m_Handle) {
        (void)m_Handle->SetDouble(val);
        (void)m_Handle->SetFloat(static_cast<float>(val));
        (void)m_Handle->SetInt32(static_cast<std::int32_t>(val));
    }
    m_Value = val;
    NotifyValueChanged();
}

std::shared_ptr<we::runtime::kindui::Widget> NumericPropertyField::CreateWidget() {
    class NumericWidget final : public we::runtime::kindui::Widget {
    public:
        explicit NumericWidget(NumericPropertyField* owner) : m_Owner(owner) {}
        we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& available) override {
            return we::runtime::kindui::Size{ available.width,
                we::runtime::kindui::LayoutMetrics::PropertyControlHeight() };
        }
        void Arrange(const we::runtime::kindui::Rect& r) override { m_Geometry = r; }
        void Paint(we::runtime::kindui::PaintContext& context) override {
            const auto frame = we::runtime::kindui::LayoutMetrics::LayoutPropertyControlInRow(m_Geometry);
            we::runtime::kindui::ControlChrome::InteractionState state;
            state.hoverAnim = m_IsHovered ? 1.0f : 0.0f;
            state.focused = m_IsFocused;
            we::runtime::kindui::ControlChrome::PaintInputFrame(context, frame, state);
        }
    private:
        NumericPropertyField* m_Owner = nullptr;
        bool m_IsHovered = false;
        bool m_IsFocused = false;
    };
    return std::make_shared<NumericWidget>(this);
}

// --- String ---
StringPropertyField::StringPropertyField(std::string_view label, bool multiline)
    : PropertyFieldBase(multiline ? FieldType::Text : FieldType::String, label), m_Multiline(multiline) {}

std::string StringPropertyField::GetValue() const {
    if (m_Handle) {
        std::string out;
        if (m_Handle->GetString(out)) return out;
    }
    return m_Value;
}

void StringPropertyField::SetValue(std::string val) {
    if (m_Handle) {
        (void)m_Handle->SetString(val);
    }
    m_Value = val;
    NotifyValueChanged();
    NotifyValueCommitted();
}

std::shared_ptr<we::runtime::kindui::Widget> StringPropertyField::CreateWidget() {
    class StringWidget final : public we::runtime::kindui::Widget {
    public:
        explicit StringWidget(StringPropertyField* owner) : m_Owner(owner) {}
        we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& available) override {
            return we::runtime::kindui::Size{ available.width,
                we::runtime::kindui::LayoutMetrics::PropertyControlHeight() };
        }
        void Arrange(const we::runtime::kindui::Rect& r) override { m_Geometry = r; }
        void Paint(we::runtime::kindui::PaintContext& context) override {
            const auto frame = we::runtime::kindui::LayoutMetrics::LayoutPropertyControlInRow(m_Geometry);
            we::runtime::kindui::ControlChrome::InteractionState state;
            state.hoverAnim = m_IsHovered ? 1.0f : 0.0f;
            state.focused = m_IsFocused;
            we::runtime::kindui::ControlChrome::PaintInputFrame(context, frame, state);
        }
    private:
        StringPropertyField* m_Owner = nullptr;
        bool m_IsHovered = false;
        bool m_IsFocused = false;
    };
    return std::make_shared<StringWidget>(this);
}

// --- Enum ---
EnumPropertyField::EnumPropertyField(std::vector<std::string> options, std::string_view label)
    : PropertyFieldBase(FieldType::Enum, label), m_Options(std::move(options)) {}

void EnumPropertyField::SetSelectedIndex(int index) {
    m_SelectedIndex = index;
    if (m_Handle) {
        (void)m_Handle->SetInt32(static_cast<std::int32_t>(index));
    }
    NotifyValueChanged();
    NotifyValueCommitted();
}

std::shared_ptr<we::runtime::kindui::Widget> EnumPropertyField::CreateWidget() {
    class EnumWidget final : public we::runtime::kindui::Widget {
    public:
        explicit EnumWidget(EnumPropertyField* owner) : m_Owner(owner) {}
        we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& available) override {
            return we::runtime::kindui::Size{ available.width,
                we::runtime::kindui::LayoutMetrics::PropertyControlHeight() };
        }
        void Arrange(const we::runtime::kindui::Rect& r) override { m_Geometry = r; }
        void Paint(we::runtime::kindui::PaintContext& context) override {
            const auto frame = we::runtime::kindui::LayoutMetrics::LayoutPropertyControlInRow(m_Geometry);
            we::runtime::kindui::ControlChrome::InteractionState state;
            state.hoverAnim = m_IsHovered ? 1.0f : 0.0f;
            we::runtime::kindui::ControlChrome::PaintInputFrame(context, frame, state);
        }
    private:
        EnumPropertyField* m_Owner = nullptr;
        bool m_IsHovered = false;
    };
    return std::make_shared<EnumWidget>(this);
}

// --- Flags ---
FlagsPropertyField::FlagsPropertyField(std::vector<std::pair<std::uint32_t, std::string>> flagBits,
    std::string_view label)
    : PropertyFieldBase(FieldType::Flags, label), m_FlagBits(std::move(flagBits)) {}

void FlagsPropertyField::SetMask(std::uint32_t mask) {
    m_Mask = mask;
    if (m_Handle) {
        (void)m_Handle->SetUInt32(mask);
    }
    NotifyValueChanged();
    NotifyValueCommitted();
}

std::shared_ptr<we::runtime::kindui::Widget> FlagsPropertyField::CreateWidget() {
    class FlagsWidget final : public we::runtime::kindui::Widget {
    public:
        explicit FlagsWidget(FlagsPropertyField* owner) : m_Owner(owner) {}
        we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& available) override {
            return we::runtime::kindui::Size{ available.width,
                we::runtime::kindui::LayoutMetrics::PropertyControlHeight() };
        }
        void Arrange(const we::runtime::kindui::Rect& r) override { m_Geometry = r; }
        void Paint(we::runtime::kindui::PaintContext& context) override {
            const auto frame = we::runtime::kindui::LayoutMetrics::LayoutPropertyControlInRow(m_Geometry);
            we::runtime::kindui::ControlChrome::InteractionState state;
            state.hoverAnim = m_IsHovered ? 1.0f : 0.0f;
            we::runtime::kindui::ControlChrome::PaintInputFrame(context, frame, state);
        }
    private:
        FlagsPropertyField* m_Owner = nullptr;
        bool m_IsHovered = false;
    };
    return std::make_shared<FlagsWidget>(this);
}

// --- Vector ---
VectorPropertyField::VectorPropertyField(int components, std::string_view label)
    : PropertyFieldBase(FieldType::Vector3, label), m_ComponentCount(components) {}

double VectorPropertyField::GetComponent(int index) const {
    if (index >= 0 && index < 4) return m_Values[index];
    return 0.0;
}

void VectorPropertyField::SetComponent(int index, double val) {
    if (index >= 0 && index < 4) {
        m_Values[index] = val;
        NotifyValueChanged();
    }
}

std::shared_ptr<we::runtime::kindui::Widget> VectorPropertyField::CreateWidget() {
    class VectorWidget final : public we::runtime::kindui::Widget {
    public:
        explicit VectorWidget(VectorPropertyField* owner) : m_Owner(owner) {}
        we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& available) override {
            return we::runtime::kindui::Size{ available.width,
                we::runtime::kindui::LayoutMetrics::PropertyControlHeight() };
        }
        void Arrange(const we::runtime::kindui::Rect& r) override { m_Geometry = r; }
        void Paint(we::runtime::kindui::PaintContext& context) override {
            const auto frame = we::runtime::kindui::LayoutMetrics::LayoutPropertyControlInRow(m_Geometry);
            we::runtime::kindui::ControlChrome::InteractionState state;
            state.hoverAnim = m_IsHovered ? 1.0f : 0.0f;
            we::runtime::kindui::ControlChrome::PaintInputFrame(context, frame, state);
        }
    private:
        VectorPropertyField* m_Owner = nullptr;
        bool m_IsHovered = false;
    };
    return std::make_shared<VectorWidget>(this);
}

// --- Rotation ---
RotationPropertyField::RotationPropertyField(std::string_view label)
    : PropertyFieldBase(FieldType::Rotation, label) {}

std::shared_ptr<we::runtime::kindui::Widget> RotationPropertyField::CreateWidget() {
    return std::make_shared<VectorPropertyField>(3, GetLabel())->CreateWidget();
}

// --- Transform ---
TransformPropertyField::TransformPropertyField(std::string_view label)
    : PropertyFieldBase(FieldType::Transform, label) {}

std::shared_ptr<we::runtime::kindui::Widget> TransformPropertyField::CreateWidget() {
    return std::make_shared<VectorPropertyField>(3, GetLabel())->CreateWidget();
}

// --- Color ---
ColorPropertyField::ColorPropertyField(bool includeAlpha, std::string_view label)
    : PropertyFieldBase(FieldType::Color, label), m_IncludeAlpha(includeAlpha) {}

std::shared_ptr<we::runtime::kindui::Widget> ColorPropertyField::CreateWidget() {
    class ColorWidget final : public we::runtime::kindui::Widget {
    public:
        explicit ColorWidget(ColorPropertyField* owner) : m_Owner(owner) {}
        we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& available) override {
            return we::runtime::kindui::Size{ available.width,
                we::runtime::kindui::LayoutMetrics::PropertyControlHeight() };
        }
        void Arrange(const we::runtime::kindui::Rect& r) override { m_Geometry = r; }
        void Paint(we::runtime::kindui::PaintContext& context) override {
            const auto frame = we::runtime::kindui::LayoutMetrics::LayoutPropertyControlInRow(m_Geometry);
            we::runtime::kindui::ControlChrome::InteractionState state;
            state.hoverAnim = m_IsHovered ? 1.0f : 0.0f;
            we::runtime::kindui::ControlChrome::PaintInputFrame(context, frame, state);
        }
    private:
        ColorPropertyField* m_Owner = nullptr;
        bool m_IsHovered = false;
    };
    return std::make_shared<ColorWidget>(this);
}

// --- Gradient ---
GradientPropertyField::GradientPropertyField(std::string_view label)
    : PropertyFieldBase(FieldType::Gradient, label) {}

std::shared_ptr<we::runtime::kindui::Widget> GradientPropertyField::CreateWidget() {
    return std::make_shared<ColorPropertyField>(true, GetLabel())->CreateWidget();
}

// --- AssetRef ---
AssetRefPropertyField::AssetRefPropertyField(std::string_view allowedExtension, std::string_view label)
    : PropertyFieldBase(FieldType::AssetRef, label), m_AllowedExtension(std::string(allowedExtension)) {}

std::shared_ptr<we::runtime::kindui::Widget> AssetRefPropertyField::CreateWidget() {
    class AssetRefWidget final : public we::runtime::kindui::Widget {
    public:
        explicit AssetRefWidget(AssetRefPropertyField* owner) : m_Owner(owner) {}
        we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& available) override {
            return we::runtime::kindui::Size{ available.width,
                we::runtime::kindui::LayoutMetrics::PropertyControlHeight() };
        }
        void Arrange(const we::runtime::kindui::Rect& r) override { m_Geometry = r; }
        void Paint(we::runtime::kindui::PaintContext& context) override {
            const auto frame = we::runtime::kindui::LayoutMetrics::LayoutPropertyControlInRow(m_Geometry);
            we::runtime::kindui::ControlChrome::InteractionState state;
            state.hoverAnim = m_IsHovered ? 1.0f : 0.0f;
            we::runtime::kindui::ControlChrome::PaintInputFrame(context, frame, state);
        }
    private:
        AssetRefPropertyField* m_Owner = nullptr;
        bool m_IsHovered = false;
    };
    return std::make_shared<AssetRefWidget>(this);
}

// --- ObjectRef ---
ObjectRefPropertyField::ObjectRefPropertyField(reflection::TypeId typeId, std::string_view label)
    : PropertyFieldBase(FieldType::ObjectRef, label), m_TypeId(typeId) {}

std::shared_ptr<we::runtime::kindui::Widget> ObjectRefPropertyField::CreateWidget() {
    return std::make_shared<AssetRefPropertyField>("", GetLabel())->CreateWidget();
}

// --- ClassRef ---
ClassRefPropertyField::ClassRefPropertyField(reflection::TypeId baseClassId, std::string_view label)
    : PropertyFieldBase(FieldType::ClassRef, label), m_BaseClassId(baseClassId) {}

std::shared_ptr<we::runtime::kindui::Widget> ClassRefPropertyField::CreateWidget() {
    return std::make_shared<AssetRefPropertyField>("", GetLabel())->CreateWidget();
}

// --- Container ---
ContainerPropertyField::ContainerPropertyField(FieldType containerType, PropertyFieldPtr elementTemplate,
    std::string_view label)
    : PropertyFieldBase(containerType, label), m_ElementTemplate(std::move(elementTemplate)) {}

std::shared_ptr<we::runtime::kindui::Widget> ContainerPropertyField::CreateWidget() {
    class ContainerWidget final : public we::runtime::kindui::Widget {
    public:
        explicit ContainerWidget(ContainerPropertyField* owner) : m_Owner(owner) {}
        we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& available) override {
            return we::runtime::kindui::Size{ available.width,
                we::runtime::kindui::LayoutMetrics::PropertyControlHeight() };
        }
        void Arrange(const we::runtime::kindui::Rect& r) override { m_Geometry = r; }
        void Paint(we::runtime::kindui::PaintContext& context) override {
            const auto frame = we::runtime::kindui::LayoutMetrics::LayoutPropertyControlInRow(m_Geometry);
            we::runtime::kindui::ControlChrome::InteractionState state;
            state.hoverAnim = m_IsHovered ? 1.0f : 0.0f;
            we::runtime::kindui::ControlChrome::PaintInputFrame(context, frame, state);
        }
    private:
        ContainerPropertyField* m_Owner = nullptr;
        bool m_IsHovered = false;
    };
    return std::make_shared<ContainerWidget>(this);
}

// --- Curve ---
CurvePropertyField::CurvePropertyField(std::string_view label)
    : PropertyFieldBase(FieldType::Curve, label) {}

std::shared_ptr<we::runtime::kindui::Widget> CurvePropertyField::CreateWidget() {
    class CurveWidget final : public we::runtime::kindui::Widget {
    public:
        explicit CurveWidget(CurvePropertyField* owner) : m_Owner(owner) {}
        we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& available) override {
            return we::runtime::kindui::Size{ available.width,
                we::runtime::kindui::LayoutMetrics::PropertyControlHeight() };
        }
        void Arrange(const we::runtime::kindui::Rect& r) override { m_Geometry = r; }
        void Paint(we::runtime::kindui::PaintContext& context) override {
            const auto frame = we::runtime::kindui::LayoutMetrics::LayoutPropertyControlInRow(m_Geometry);
            we::runtime::kindui::ControlChrome::InteractionState state;
            state.hoverAnim = m_IsHovered ? 1.0f : 0.0f;
            we::runtime::kindui::ControlChrome::PaintInputFrame(context, frame, state);
        }
    private:
        CurvePropertyField* m_Owner = nullptr;
        bool m_IsHovered = false;
    };
    return std::make_shared<CurveWidget>(this);
}

// --- Tag / Layer ---
TagLayerPropertyField::TagLayerPropertyField(bool isLayerMode, std::string_view label)
    : PropertyFieldBase(isLayerMode ? FieldType::Layer : FieldType::Tag, label), m_IsLayerMode(isLayerMode) {}

std::shared_ptr<we::runtime::kindui::Widget> TagLayerPropertyField::CreateWidget() {
    class TagLayerWidget final : public we::runtime::kindui::Widget {
    public:
        explicit TagLayerWidget(TagLayerPropertyField* owner) : m_Owner(owner) {}
        we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& available) override {
            return we::runtime::kindui::Size{ available.width,
                we::runtime::kindui::LayoutMetrics::PropertyControlHeight() };
        }
        void Arrange(const we::runtime::kindui::Rect& r) override { m_Geometry = r; }
        void Paint(we::runtime::kindui::PaintContext& context) override {
            const auto frame = we::runtime::kindui::LayoutMetrics::LayoutPropertyControlInRow(m_Geometry);
            we::runtime::kindui::ControlChrome::InteractionState state;
            state.hoverAnim = m_IsHovered ? 1.0f : 0.0f;
            we::runtime::kindui::ControlChrome::PaintInputFrame(context, frame, state);
        }
    private:
        TagLayerPropertyField* m_Owner = nullptr;
        bool m_IsHovered = false;
    };
    return std::make_shared<TagLayerWidget>(this);
}

} // namespace detail
} // namespace we::editor::property
