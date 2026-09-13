// ==============================================================================
// WindEffects — PropertyEditor — DefaultEditors
// Internal implementation for the PropertyEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "PropertyEditorInternal.h"
#include "PropertyEditor/IPropertyEditor.h"
#include "PropertyEditor/IPropertyEditorFactory.h"
#include "PropertyEditor/IPropertyField.h"
#include "PropertyEditor/PropertyFieldFactory.h"
#include "PropertyEditor/PropertyFieldTypes.h"
#include "Core/Logger.h"

#include <KindUI/EditorUI.h>
// Forward declaration for the popup host function
namespace we::programs::editor {
    ::we::runtime::kindui::IPopupHost* GetEditorPopupHost();
}

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unordered_map>

namespace we::editor::property {
namespace LayoutMetrics = ::we::runtime::kindui::LayoutMetrics;
namespace detail {
namespace {

using we::runtime::kindui::Color;
using we::runtime::kindui::ColorToken;
using we::runtime::kindui::DPIContext;
using we::runtime::kindui::MetricToken;
using we::runtime::kindui::MouseButton;
using we::runtime::kindui::MouseEvent;
using we::runtime::kindui::PaintContext;
using we::runtime::kindui::Point;
using we::runtime::kindui::Rect;
using we::runtime::kindui::Size;
using we::runtime::kindui::TextBox;
using we::runtime::kindui::Widget;
using we::runtime::kindui::IconPainter;
using we::runtime::kindui::WindIconRef;
namespace WindIcons = we::runtime::kindui::WindIcons;
namespace Layout = we::runtime::kindui::LayoutMetrics;
namespace Chrome = we::runtime::kindui::ControlChrome;

float EditorControlHeight() {
    return Layout::PropertyControlHeight();
}

Rect EditorControlRect(const Rect& valueRect) {
    return Layout::LayoutPropertyControlInRow(valueRect);
}

/// internal rules + colored left accents (no per-axis outer boxes).
struct VectorComponentGrid {
    Rect frame;
    int componentCount = 0;
    float fieldWidth = 0.0f;
    float fieldPad = 0.0f;
    float accentWidth = 0.0f;
    float gap = 0.0f;

    [[nodiscard]] Rect FieldAt(int index) const {
        const float offset = static_cast<float>(index) * (fieldWidth + gap);
        return Rect{
            frame.x + offset,
            frame.y,
            fieldWidth,
            frame.height };
    }

    [[nodiscard]] float DividerXAfter(int index) const {
        const Rect field = FieldAt(index);
        return field.x + field.width;
    }
};

[[nodiscard]] VectorComponentGrid LayoutVectorComponentGrid(const Rect& frame, int componentCount) {
    const float scale = DPIContext::GetScale();
    const float sidePadding = 0.0f;
    const float pad = 4.0f * scale;
    const float gap = 4.0f * scale;
    const float accentW = std::max(2.0f, ResolveMetric(MetricToken::BorderWidth) * 2.0f);
    const Rect paddedFrame{
        frame.x + sidePadding,
        frame.y,
        frame.width - sidePadding * 2.0f,
        frame.height
    };
    const float totalGap = gap * static_cast<float>(componentCount - 1);
    const float availableWidth = paddedFrame.width - totalGap;
    VectorComponentGrid grid;
    grid.frame = paddedFrame;
    grid.componentCount = componentCount;
    grid.fieldWidth = componentCount > 0 ? availableWidth / static_cast<float>(componentCount) : 0.0f;
    grid.fieldPad = pad;
    grid.accentWidth = accentW;
    grid.gap = gap;
    return grid;
}

class BoundEditorBase : public IPropertyEditor {
public:
    void Bind(const PropertyHandlePtr& handle) override { m_Handle = handle; }

protected:
    PropertyHandlePtr m_Handle;
};

class PropertyFieldAdapterEditor final : public BoundEditorBase {
public:
    explicit PropertyFieldAdapterEditor(PropertyFieldPtr field) : m_Field(std::move(field)) {}

    bool CanEdit(const PropertyInfo& property) const override { return m_Field != nullptr; }

    void Bind(const PropertyHandlePtr& handle) override {
        BoundEditorBase::Bind(handle);
        if (m_Field) {
            m_Field->BindHandle(handle);
        }
    }

    std::shared_ptr<Widget> CreateWidget() override {
        if (m_Field) {
            return m_Field->CreateWidget();
        }
        return nullptr;
    }

    PropertyFieldPtr GetField() const { return m_Field; }
private:
    PropertyFieldPtr m_Field;
};

// --- Bool ---
class BoolEditor final : public BoundEditorBase {
public:
    [[nodiscard]] bool CanEdit(const PropertyInfo& property) const override {
        return property.primitive == PrimitiveKind::Bool;
    }

    [[nodiscard]] std::shared_ptr<Widget> CreateWidget() override {
        class BoolWidget final : public Widget {
        public:
            explicit BoolWidget(PropertyHandlePtr handle) : m_Handle(std::move(handle)) {
                if (m_Handle) {
                    (void)m_Handle->GetBool(m_Value);
                }
            }
            Size Measure(const Size& available) override {
                return Size{ available.width, EditorControlHeight() };
            }
            void Arrange(const Rect& r) override { m_Geometry = r; }
            void Paint(PaintContext& context) override {
                const float checkSize = ResolveMetric(MetricToken::CheckboxGlyphSize);
                Rect checkRect = EditorControlRect(m_Geometry);
                checkRect.width = checkSize;
                checkRect.height = checkSize;
                // A boolean is a value control, not a row decoration: anchor
                // it to the same fixed value-column edge as text and vectors.
                checkRect.x = m_Geometry.x;
                Chrome::InteractionState state;
                state.focused = m_Focused;
                Chrome::PaintCheckbox(context, checkRect, m_Value, state);
                if (m_Handle && m_Handle->GetValueState() == PropertyValueState::Mixed) {
                    const float fontSize = ResolveMetric(MetricToken::TextSizeProperty);
                    context.DrawText(
                        "—",
                        Point{ m_Geometry.x + ResolveMetric(MetricToken::Space1),
                            LayoutMetrics::AlignTextTopY(m_Geometry, fontSize) },
                        ThemeColor(ColorToken::TextSecondary),
                        fontSize);
                }
            }
            void OnMouseDown(const MouseEvent& event) override {
                if (event.button != MouseButton::Left || !m_Handle || m_Handle->IsReadOnly()) {
                    return;
                }
                m_Value = !m_Value;
                (void)m_Handle->SetBool(m_Value);
            }

        private:
            PropertyHandlePtr m_Handle;
            bool m_Value = false;
        };
        return std::make_shared<BoolWidget>(m_Handle);
    }
};

// --- Numeric / string via TextBox ---
class TextValueEditor final : public BoundEditorBase {
public:
    enum class Mode { Int32, UInt32, Int64, Float, Double, String };

    explicit TextValueEditor(Mode mode) : m_Mode(mode) {}

    [[nodiscard]] bool CanEdit(const PropertyInfo& property) const override {
        switch (m_Mode) {
        case Mode::Int32:
            return property.primitive == PrimitiveKind::Int32 || property.primitive == PrimitiveKind::Int16 ||
                   property.primitive == PrimitiveKind::Int8;
        case Mode::UInt32:
            return property.primitive == PrimitiveKind::UInt32 || property.primitive == PrimitiveKind::UInt16 ||
                   property.primitive == PrimitiveKind::UInt8;
        case Mode::Int64:
            return property.primitive == PrimitiveKind::Int64 || property.primitive == PrimitiveKind::UInt64;
        case Mode::Float:
            return property.primitive == PrimitiveKind::Float;
        case Mode::Double:
            return property.primitive == PrimitiveKind::Double;
        case Mode::String:
            return property.primitive == PrimitiveKind::String || property.primitive == PrimitiveKind::Char;
        }
        return false;
    }

    [[nodiscard]] std::shared_ptr<Widget> CreateWidget() override {
        std::string initial = FormatValue();
        auto handle = m_Handle;
        Mode mode = m_Mode;
        return std::make_shared<TextBox>(initial, [handle, mode](const std::string& text) {
            if (!handle || handle->IsReadOnly()) {
                return;
            }
            switch (mode) {
            case Mode::Int32: {
                const std::int32_t v = static_cast<std::int32_t>(std::strtol(text.c_str(), nullptr, 10));
                (void)handle->SetInt32(v);
                break;
            }
            case Mode::UInt32: {
                const std::uint32_t v = static_cast<std::uint32_t>(std::strtoul(text.c_str(), nullptr, 10));
                (void)handle->SetUInt32(v);
                break;
            }
            case Mode::Int64: {
                const std::int64_t v = static_cast<std::int64_t>(std::strtoll(text.c_str(), nullptr, 10));
                (void)handle->SetInt64(v);
                break;
            }
            case Mode::Float: {
                const float v = std::strtof(text.c_str(), nullptr);
                (void)handle->SetFloat(v);
                break;
            }
            case Mode::Double: {
                const double v = std::strtod(text.c_str(), nullptr);
                (void)handle->SetDouble(v);
                break;
            }
            case Mode::String:
                (void)handle->SetString(text);
                break;
            }
        });
    }

private:
    [[nodiscard]] std::string FormatValue() const {
        if (!m_Handle || m_Handle->GetValueState() == PropertyValueState::Mixed) {
            return "—";
        }
        char buf[64]{};
        switch (m_Mode) {
        case Mode::Int32: {
            std::int32_t v = 0;
            (void)m_Handle->GetInt32(v);
            std::snprintf(buf, sizeof(buf), "%d", v);
            return buf;
        }
        case Mode::UInt32: {
            std::uint32_t v = 0;
            (void)m_Handle->GetUInt32(v);
            std::snprintf(buf, sizeof(buf), "%u", v);
            return buf;
        }
        case Mode::Int64: {
            std::int64_t v = 0;
            (void)m_Handle->GetInt64(v);
            std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(v));
            return buf;
        }
        case Mode::Float: {
            float v = 0.f;
            (void)m_Handle->GetFloat(v);
            std::snprintf(buf, sizeof(buf), "%.4g", v);
            return buf;
        }
        case Mode::Double: {
            double v = 0.0;
            (void)m_Handle->GetDouble(v);
            std::snprintf(buf, sizeof(buf), "%.6g", v);
            return buf;
        }
        case Mode::String: {
            std::string s;
            (void)m_Handle->GetString(s);
            return s;
        }
        }
        return {};
    }

    Mode m_Mode;
};

// --- Enum ---
class EnumEditor final : public BoundEditorBase {
public:
    explicit EnumEditor(ITypeRegistry* registry) : m_Registry(registry) {}

    [[nodiscard]] bool CanEdit(const PropertyInfo& property) const override {
        if (!m_Registry) {
            return false;
        }
        const TypeInfo* info = m_Registry->Find(property.typeId);
        return info && info->IsEnum();
    }

    [[nodiscard]] std::shared_ptr<Widget> CreateWidget() override {
        class EnumWidget final : public Widget {
        public:
            EnumWidget(PropertyHandlePtr handle, ITypeRegistry* registry)
                : m_Handle(std::move(handle))
                , m_Registry(registry)
            {
                Refresh();
            }
            Size Measure(const Size& available) override {
                return Size{ available.width, EditorControlHeight() };
            }
            void Arrange(const Rect& r) override { m_Geometry = r; }
            void Paint(PaintContext& context) override {
                const Rect frame = EditorControlRect(m_Geometry);
                Chrome::InteractionState state;
                state.focused = m_Focused;
                Chrome::PaintInputFrame(context, frame, state);
                const std::string label =
                    m_Handle && m_Handle->GetValueState() == PropertyValueState::Mixed ? "—" : m_CurrentName;
                const float fontSize = ResolveMetric(MetricToken::TextSizeProperty);
                const float pad = ResolveMetric(MetricToken::Space2);
                context.DrawText(
                    label,
                    Point{ frame.x + pad, LayoutMetrics::AlignTextTopY(frame, fontSize) },
                    ThemeColor(ColorToken::TextPrimary),
                    fontSize);
            }
            void OnMouseDown(const MouseEvent& event) override {
                if (event.button != MouseButton::Left || !m_Handle || m_Handle->IsReadOnly() || m_Values.empty()) {
                    return;
                }
                m_Index = (m_Index + 1) % static_cast<int>(m_Values.size());
                const std::int64_t value = m_Values[static_cast<std::size_t>(m_Index)].value;
                // Write underlying integer size from property
                if (const auto* prop = m_Handle->GetPropertyInfo()) {
                    if (prop->size == 1) {
                        const std::int8_t v = static_cast<std::int8_t>(value);
                        (void)m_Handle->SetRaw(&v, sizeof(v));
                    } else if (prop->size == 2) {
                        const std::int16_t v = static_cast<std::int16_t>(value);
                        (void)m_Handle->SetRaw(&v, sizeof(v));
                    } else if (prop->size == 4) {
                        const std::int32_t v = static_cast<std::int32_t>(value);
                        (void)m_Handle->SetRaw(&v, sizeof(v));
                    } else if (prop->size == 8) {
                        (void)m_Handle->SetRaw(&value, sizeof(value));
                    }
                }
                m_CurrentName = m_Values[static_cast<std::size_t>(m_Index)].name;
            }

        private:
            void Refresh() {
                m_Values.clear();
                m_CurrentName = "—";
                m_Index = 0;
                if (!m_Handle || !m_Registry || !m_Handle->GetPropertyInfo()) {
                    return;
                }
                const TypeInfo* info = m_Registry->Find(m_Handle->GetPropertyInfo()->typeId);
                if (!info || !info->IsEnum()) {
                    return;
                }
                m_Values = info->enumInfo.values;
                std::int64_t current = 0;
                if (const auto* prop = m_Handle->GetPropertyInfo()) {
                    if (prop->size <= 8) {
                        std::uint8_t buf[8]{};
                        if (m_Handle->GetRaw(buf, prop->size)) {
                            std::memcpy(&current, buf, prop->size);
                        }
                    }
                }
                for (std::size_t i = 0; i < m_Values.size(); ++i) {
                    if (m_Values[i].value == current) {
                        m_Index = static_cast<int>(i);
                        m_CurrentName = m_Values[i].name;
                        return;
                    }
                }
                if (!m_Values.empty()) {
                    m_CurrentName = m_Values.front().name;
                }
            }

            PropertyHandlePtr m_Handle;
            ITypeRegistry* m_Registry = nullptr;
            std::vector<reflection::EnumValueInfo> m_Values;
            std::string m_CurrentName;
            int m_Index = 0;
        };
        return std::make_shared<EnumWidget>(m_Handle, m_Registry);
    }

private:
    ITypeRegistry* m_Registry = nullptr;
};

// --- Vector / Color ---
class VectorEditor final : public BoundEditorBase {
public:
    enum class Mode { Vec2, Vec3, Vec4 };

    explicit VectorEditor(Mode mode) : m_Mode(mode) {}

    [[nodiscard]] bool CanEdit(const PropertyInfo& property) const override {
        switch (m_Mode) {
        case Mode::Vec2:
            return property.primitive == PrimitiveKind::Vec2;
        case Mode::Vec3:
            return property.primitive == PrimitiveKind::Vec3;
        case Mode::Vec4:
            return property.primitive == PrimitiveKind::Vec4 || property.primitive == PrimitiveKind::Quat;
        }
        return false;
    }

    [[nodiscard]] std::shared_ptr<Widget> CreateWidget() override {
        class VecWidget final : public Widget {
        public:
            VecWidget(PropertyHandlePtr handle, Mode mode) : m_Handle(std::move(handle)), m_Mode(mode) {
                Read();
            }
            Size Measure(const Size& available) override { return Size{ available.width, EditorControlHeight() }; }
            void Arrange(const Rect& r) override { m_Geometry = r; }

        private:
            [[nodiscard]] bool IsTransformProperty() const {
                if (!m_Handle) {
                    return false;
                }
                const std::string path(m_Handle->GetPath());
                std::string s = path;
                for (char& c : s) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                return (s.find("position") != std::string::npos ||
                        s.find("rotation") != std::string::npos ||
                        s.find("scale") != std::string::npos ||
                        s.find("transform") != std::string::npos ||
                        s.find("location") != std::string::npos);
            }

            [[nodiscard]] bool IsScaleProperty() const {
                if (!m_Handle) {
                    return false;
                }
                const std::string path(m_Handle->GetPath());
                std::string s = path;
                for (char& c : s) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                return (s.find("scale") != std::string::npos);
            }

            [[nodiscard]] we::runtime::kindui::Color AxisTint(int index) const {
                switch (index) {
                case 0: return ThemeColor(we::runtime::kindui::ColorToken::AxisX);
                case 1: return ThemeColor(we::runtime::kindui::ColorToken::AxisY);
                case 2: return ThemeColor(we::runtime::kindui::ColorToken::AxisZ);
                default: return ThemeColor(we::runtime::kindui::ColorToken::TextSecondary);
                }
            }

            void ApplyProportionalScale(int changedComponent, float newValue) {
                const int componentCount = m_Mode == Mode::Vec2 ? 2 : (m_Mode == Mode::Vec3 ? 3 : 4);
                const double oldVal = static_cast<double>(m_StartValues[changedComponent]);
                const double newVal = static_cast<double>(newValue);

                if (std::abs(oldVal) > 1e-7) {
                    const double factor = newVal / oldVal;
                    for (int i = 0; i < componentCount; ++i) {
                        if (i == changedComponent) {
                            m_Values[i] = newValue;
                        } else {
                            m_Values[i] = static_cast<float>(static_cast<double>(m_StartValues[i]) * factor);
                        }
                    }
                } else {
                    const double delta = newVal - oldVal;
                    for (int i = 0; i < componentCount; ++i) {
                        if (i == changedComponent) {
                            m_Values[i] = newValue;
                        } else {
                            m_Values[i] = static_cast<float>(static_cast<double>(m_StartValues[i]) + delta);
                        }
                    }
                }
            }

        public:
            void OnMouseDown(const MouseEvent& event) override {
                if (event.button != MouseButton::Left || !m_Handle || m_Handle->IsReadOnly()) {
                    return;
                }

                const Rect frame = EditorControlRect(m_Geometry);
                const int componentCount = m_Mode == Mode::Vec2 ? 2 : (m_Mode == Mode::Vec3 ? 3 : 4);
                const VectorComponentGrid grid = LayoutVectorComponentGrid(frame, componentCount);

                for (int i = 0; i < componentCount; ++i) {
                    if (grid.FieldAt(i).Contains(event.position)) {
                        m_ActiveComponent = i;
                        m_IsDragging = true;
                        m_DragStartX = event.position.x;
                        for (int k = 0; k < 4; ++k) {
                            m_StartValues[k] = m_Values[k];
                        }
                        return;
                    }
                }
            }

            void OnMouseMove(const MouseEvent& event) override {
                const float scale = DPIContext::GetScale();
                if (m_IsDragging && m_ActiveComponent >= 0 && m_ActiveComponent < 4) {
                    const float deltaX = event.position.x - m_DragStartX;
                    const float sensitivity = 0.01f * scale;
                    const float newVal = m_StartValues[m_ActiveComponent] + deltaX * sensitivity;

                    const bool isScale = IsScaleProperty();
                    const bool isLocked = isScale &&
                        we::runtime::kindui::PropertyAspectLockRegistry::IsLocked(m_Handle->GetPath(), true);

                    if (isLocked) {
                        ApplyProportionalScale(m_ActiveComponent, newVal);
                    } else {
                        m_Values[m_ActiveComponent] = newVal;
                    }

                    Write();
                    InvalidatePaint();
                }
            }

            void OnMouseUp(const MouseEvent& /*event*/) override {
                m_IsDragging = false;
                m_ActiveComponent = -1;
            }

            void Paint(PaintContext& context) override {
                Chrome::InteractionState state;
                const Rect frame = EditorControlRect(m_Geometry);

                const float scale = DPIContext::GetScale();
                const float fontSize = ResolveMetric(MetricToken::TextSizeProperty);
                const int componentCount = m_Mode == Mode::Vec2 ? 2 : (m_Mode == Mode::Vec3 ? 3 : 4);

                const VectorComponentGrid grid = LayoutVectorComponentGrid(frame, componentCount);
                const bool isTransform = IsTransformProperty();

                for (int index = 0; index < componentCount; ++index) {
                    const Rect field = grid.FieldAt(index);

                    // Separate input frame per component field with gap
                    Chrome::PaintInputFrame(context, field, state);

                    float valueX = field.x + 6.0f * scale;

                    // Axis indicator rail (X=Red, Y=Green, Z=Blue) is drawn INSIDE all Transform properties (Position, Rotation, Scale, Location)
                    if (isTransform) {
                        const float inset = 3.0f * scale;
                        const float accentW = 3.0f * scale;
                        const Rect accent{
                            field.x + inset,
                            field.y + inset,
                            accentW,
                            std::max(0.0f, field.height - inset * 2.0f)
                        };
                        context.DrawRoundedRect(accent, AxisTint(index), 1.5f);
                        valueX = accent.x + accent.width + 5.0f * scale;
                    }

                    char value[32]{};
                    std::snprintf(value, sizeof(value), "%.3g", m_Values[index]);
                    const float textY = LayoutMetrics::AlignTextTopY(field, fontSize);
                    context.DrawText(value, Point{ valueX, textY }, ThemeColor(ColorToken::TextPrimary), fontSize);
                }
            }

            void Read() {
                std::memset(m_Values, 0, sizeof(m_Values));
                if (!m_Handle) {
                    return;
                }
                const std::size_t n =
                    m_Mode == Mode::Vec2 ? 2u : (m_Mode == Mode::Vec3 ? 3u : 4u);
                (void)m_Handle->GetRaw(m_Values, sizeof(float) * n);
            }

            void Write() {
                if (!m_Handle) {
                    return;
                }
                const std::size_t n =
                    m_Mode == Mode::Vec2 ? 2u : (m_Mode == Mode::Vec3 ? 3u : 4u);
                (void)m_Handle->SetRaw(m_Values, sizeof(float) * n);
            }

            PropertyHandlePtr m_Handle;
            Mode m_Mode;
            float m_Values[4]{};
            float m_StartValues[4]{};
            bool m_IsAspectLocked = false;
            bool m_LockHovered = false;
            bool m_IsDragging = false;
            int m_ActiveComponent = -1;
            float m_DragStartX = 0.0f;
        };
        return std::make_shared<VecWidget>(m_Handle, m_Mode);
    }

private:
    Mode m_Mode;
};

// --- Color ---
class ColorEditor final : public BoundEditorBase {
public:
    [[nodiscard]] bool CanEdit(const PropertyInfo& property) const override {
        return property.primitive == PrimitiveKind::Vec3 || property.primitive == PrimitiveKind::Vec4;
    }

    [[nodiscard]] std::shared_ptr<Widget> CreateWidget() override {
        class ColorWidget final : public Widget {
        public:
            explicit ColorWidget(PropertyHandlePtr handle) : m_Handle(std::move(handle)) {
                Read();
            }
            Size Measure(const Size& available) override { return Size{ available.width, EditorControlHeight() }; }
            void Arrange(const Rect& r) override {
                m_Geometry = r;
            }

            void OnMouseDown(const MouseEvent& event) override {
                if (event.button != MouseButton::Left || !m_Handle || m_Handle->IsReadOnly()) {
                    return;
                }
                if (!m_Geometry.Contains(event.position)) {
                    return;
                }
                // Open the color picker popup using the editor's popup host
                auto* popupHost = we::programs::editor::GetEditorPopupHost();
                if (!popupHost) {
                    return;
                }

                class ColorPickerContent final : public Widget {
                public:
                    ColorPickerContent(float* values, std::function<void()> onColorChanged, std::function<void()>
                        onClose)
                        : m_Values(values), m_OnColorChanged(std::move(onColorChanged)), m_OnClose(std::move(onClose)) {
                        m_OriginalColor = we::runtime::kindui::Color{ m_Values[0], m_Values[1], m_Values[2],
                            m_Values[3] };
                        HSVColor hsv = RGBToHSV(m_Values[0], m_Values[1], m_Values[2]);
                        m_Hue = hsv.h;
                        m_Saturation = hsv.s;
                        m_Value = hsv.v;
                        m_Alpha = m_Values[3];

                        m_Presets = {
                            we::runtime::kindui::Color{ 1.0f, 1.0f, 1.0f, 1.0f },
                            we::runtime::kindui::Color{ 0.0f, 0.0f, 0.0f, 1.0f },
                            we::runtime::kindui::Color{ 0.9f, 0.2f, 0.2f, 1.0f },
                            we::runtime::kindui::Color{ 0.2f, 0.8f, 0.3f, 1.0f },
                            we::runtime::kindui::Color{ 0.2f, 0.5f, 0.95f, 1.0f },
                            we::runtime::kindui::Color{ 1.0f, 0.85f, 0.1f, 1.0f },
                            we::runtime::kindui::Color{ 0.1f, 0.85f, 0.9f, 1.0f },
                            we::runtime::kindui::Color{ 0.9f, 0.2f, 0.85f, 1.0f },
                            we::runtime::kindui::Color{ 0.95f, 0.5f, 0.1f, 1.0f },
                            we::runtime::kindui::Color{ 0.6f, 0.3f, 0.85f, 1.0f },
                            we::runtime::kindui::Color{ 0.5f, 0.5f, 0.5f, 1.0f },
                            we::runtime::kindui::Color{ 0.2f, 0.2f, 0.2f, 1.0f },
                        };
                    }

                    Size Measure(const Size& available) override {
                        return Size{ 320.0f, 345.0f };
                    }

                    void Arrange(const Rect& r) override {
                        m_Geometry = r;
                        const float margin = 12.0f;
                        const float contentW = r.width - margin * 2.0f;

                        // Top Section: Radial Wheel (160px), Value Slider (18px), Swatch Preview (Remaining ~98px)
                        const float wheelSize = 160.0f;
                        const float sliderW = 18.0f;
                        const float previewW = contentW - wheelSize - sliderW - 20.0f;

                        m_RadialWheelRect = Rect{ r.x + margin, r.y + margin, wheelSize, wheelSize };
                        m_ValueSlider = Rect{ m_RadialWheelRect.x + wheelSize + 10.0f, r.y + margin, sliderW,
                            wheelSize };
                        m_ColorPreview = Rect{ m_ValueSlider.x + sliderW + 10.0f, r.y + margin, (std::max)(40.0f,
                            previewW), wheelSize };

                        // Alpha slider below wheel section
                        const float alphaHeight = 18.0f;
                        m_AlphaSlider = Rect{ r.x + margin, m_RadialWheelRect.y + wheelSize + 10.0f, contentW,
                            alphaHeight };

                        // Hex input & RGBA values row
                        const float rowHeight = 26.0f;
                        const float hexWidth = 95.0f;
                        const float rowY = m_AlphaSlider.y + alphaHeight + 8.0f;
                        m_HexInput = Rect{ r.x + margin, rowY, hexWidth, rowHeight };
                        m_RGBAValues = Rect{ m_HexInput.x + hexWidth + 8.0f, rowY, contentW - hexWidth - 8.0f,
                            rowHeight };

                        // Quick Palette Row
                        const float paletteY = m_HexInput.y + rowHeight + 8.0f;
                        m_QuickPaletteRect = Rect{ r.x + margin, paletteY, contentW, 20.0f };

                        // Footer with Cancel and OK buttons
                        const float footerY = m_QuickPaletteRect.y + 20.0f + 12.0f;
                        m_FooterRect = Rect{ r.x + margin, footerY, contentW, 26.0f };
                        const float btnW = 68.0f;
                        m_OKButtonRect = Rect{ m_FooterRect.x + m_FooterRect.width - btnW, m_FooterRect.y, btnW,
                            26.0f };
                        m_CancelButtonRect = Rect{ m_OKButtonRect.x - btnW - 8.0f, m_FooterRect.y, btnW, 26.0f };
                    }

                    void Paint(PaintContext& context) override {
                        DrawRadialWheel(context);
                        DrawValueSlider(context);
                        DrawColorPreview(context);
                        DrawAlphaSlider(context);
                        DrawHexInput(context);
                        DrawRGBAValues(context);
                        DrawQuickPalette(context);
                        DrawFooterButtons(context);
                    }

                    void OnMouseDown(const MouseEvent& event) override {
                        if (event.button != MouseButton::Left) return;

                        if (m_RadialWheelRect.Contains(event.position)) {
                            m_DraggingWheel = true;
                            HandleRadialWheelInteraction(event.position);
                            return;
                        }
                        if (m_ValueSlider.Contains(event.position)) {
                            m_DraggingValue = true;
                            HandleValueSliderInteraction(event.position);
                            return;
                        }
                        if (m_AlphaSlider.Contains(event.position)) {
                            m_DraggingAlpha = true;
                            HandleAlphaSliderInteraction(event.position);
                            return;
                        }

                        // OK Button
                        if (m_OKButtonRect.Contains(event.position)) {
                            m_OKPressed = true;
                            if (m_OnClose) {
                                m_OnClose();
                            }
                            return;
                        }

                        // Cancel Button
                        if (m_CancelButtonRect.Contains(event.position)) {
                            m_CancelPressed = true;
                            SetColorFromRGBA(m_OriginalColor);
                            if (m_OnClose) {
                                m_OnClose();
                            }
                            return;
                        }

                        // Quick Palette Swatches
                        if (m_QuickPaletteRect.Contains(event.position) && !m_Presets.empty()) {
                            const float count = static_cast<float>(m_Presets.size());
                            const float swatchW = (m_QuickPaletteRect.width - (count - 1.0f) * 4.0f) / count;
                            for (size_t i = 0; i < m_Presets.size(); ++i) {
                                const Rect swatch{ m_QuickPaletteRect.x + static_cast<float>(i) * (swatchW + 4.0f),
                                    m_QuickPaletteRect.y, swatchW, m_QuickPaletteRect.height };
                                if (swatch.Contains(event.position)) {
                                    SetColorFromRGBA(m_Presets[i]);
                                    return;
                                }
                            }
                        }

                        // Original color preview click -> reset to original color
                        if (m_ColorPreview.Contains(event.position)) {
                            SetColorFromRGBA(m_OriginalColor);
                            return;
                        }
                    }

                    void OnMouseMove(const MouseEvent& event) override {
                        m_OKHovered = m_OKButtonRect.Contains(event.position);
                        m_CancelHovered = m_CancelButtonRect.Contains(event.position);
                        if (event.button == MouseButton::Left) {
                            if (m_DraggingWheel) HandleRadialWheelInteraction(event.position);
                            else if (m_DraggingValue) HandleValueSliderInteraction(event.position);
                            else if (m_DraggingAlpha) HandleAlphaSliderInteraction(event.position);
                        }
                    }

                    void OnMouseUp(const MouseEvent& event) override {
                        m_DraggingWheel = false;
                        m_DraggingValue = false;
                        m_DraggingAlpha = false;
                        m_OKPressed = false;
                        m_CancelPressed = false;
                    }

                private:
                    void DrawRadialWheel(PaintContext& context) {
                        const float cx = m_RadialWheelRect.x + m_RadialWheelRect.width * 0.5f;
                        const float cy = m_RadialWheelRect.y + m_RadialWheelRect.height * 0.5f;
                        const float radius = (std::min)(m_RadialWheelRect.width, m_RadialWheelRect.height) * 0.5f -
                            1.0f;
                        const float M_PI_F = 3.1415926535f;

                        // Dark background disc
                        context.DrawRoundedRect(m_RadialWheelRect,
                            we::runtime::kindui::ResolveColor(ColorToken::PanelBackground), radius);

                        // Continuous 2D Cartesian sampling grid over circle bounds
                        // Step size of 1.0px with 1.5px rect size for continuous smooth anti-aliased HSV wheel
                        const float step = 1.0f;
                        const float rectSize = 1.5f;
                        const float maxRadius = radius + 0.5f;
                        const float radiusSq = maxRadius * maxRadius;

                        for (float y = -maxRadius; y <= maxRadius; y += step) {
                            for (float x = -maxRadius; x <= maxRadius; x += step) {
                                const float distSq = x * x + y * y;
                                if (distSq <= radiusSq) {
                                    const float dist = std::sqrt(distSq);
                                    const float edgeAlpha = std::clamp(radius + 0.5f - dist, 0.0f, 1.0f);
                                    if (edgeAlpha <= 0.001f) {
                                        continue;
                                    }

                                    const float sat = std::clamp(dist / (radius > 0.001f ? radius : 1.0f), 0.0f, 1.0f);

                                    float angle = std::atan2(-y, x);
                                    if (angle < 0.0f) {
                                        angle += 2.0f * M_PI_F;
                                    }
                                    const float hue = angle / (2.0f * M_PI_F);

                                    const we::runtime::kindui::Color color = HSVToRGB(hue, sat, m_Value, edgeAlpha);
                                    const Rect pixelRect{ cx + x - 0.5f * step, cy + y - 0.5f * step, rectSize,
                                        rectSize };
                                    context.DrawRect(pixelRect, color);
                                }
                            }
                        }

                        // Outer subtle boundary ring
                        context.DrawRoundedRectOutline(m_RadialWheelRect,
                            we::runtime::kindui::ResolveColor(ColorToken::BorderDefault), 1.0f, radius);

                        // Selector handle on wheel
                        const float angle = m_Hue * 2.0f * M_PI_F;
                        const float rDist = m_Saturation * radius;
                        const float handleX = cx + rDist * std::cos(angle);
                        const float handleY = cy - rDist * std::sin(angle);
                        const float handleR = 5.0f;

                        const Rect handleOuter{ handleX - handleR, handleY - handleR, handleR * 2.0f, handleR * 2.0f };
                        const Rect handleInner{ handleX - (handleR - 1.5f), handleY - (handleR - 1.5f), (handleR -
                            1.5f) * 2.0f, (handleR - 1.5f) * 2.0f };
                        context.DrawRoundedRectOutline(handleOuter, we::runtime::kindui::Color{ 0.0f, 0.0f, 0.0f,
                            0.95f }, 1.5f, handleR);
                        context.DrawRoundedRectOutline(handleInner, we::runtime::kindui::Color{ 1.0f, 1.0f, 1.0f,
                            0.95f }, 1.5f, handleR - 1.5f);
                    }

                    void DrawValueSlider(PaintContext& context) {
                        const float cornerRadius = 3.0f;
                        context.DrawRoundedRect(m_ValueSlider,
                            we::runtime::kindui::ResolveColor(ColorToken::InputBackground), cornerRadius);

                        const int steps = 80;
                        const float segH = m_ValueSlider.height / static_cast<float>(steps);
                        for (int y = 0; y < steps; ++y) {
                            const float v = 1.0f - (static_cast<float>(y) / static_cast<float>(steps - 1));
                            const we::runtime::kindui::Color color = HSVToRGB(m_Hue, m_Saturation, v, 1.0f);
                            const Rect segment{ m_ValueSlider.x, m_ValueSlider.y + y * segH, m_ValueSlider.width,
                                segH + 0.5f };
                            context.DrawRect(segment, color);
                        }

                        context.DrawRoundedRectOutline(m_ValueSlider,
                            we::runtime::kindui::ResolveColor(ColorToken::BorderDefault), 1.0f, cornerRadius);

                        // Value selector handle
                        const float selectorY = m_ValueSlider.y + (1.0f - m_Value) * m_ValueSlider.height;
                        const float handleH = 5.0f;
                        const Rect handle{ m_ValueSlider.x - 2.0f, selectorY - handleH * 0.5f, m_ValueSlider.width +
                            4.0f, handleH };
                        context.DrawRoundedRect(handle, we::runtime::kindui::Color{ 1.0f, 1.0f, 1.0f, 1.0f }, 2.0f);
                        context.DrawRoundedRectOutline(handle, we::runtime::kindui::Color{ 0.0f, 0.0f, 0.0f, 0.8f },
                            1.0f, 2.0f);
                    }

                    void DrawAlphaSlider(PaintContext& context) {
                        const float cornerRadius = 3.0f;

                        // Transparency checkerboard
                        context.DrawRoundedRect(m_AlphaSlider,
                            we::runtime::kindui::ResolveColor(ColorToken::InputBackground), cornerRadius);
                        context.PushClipRect(m_AlphaSlider);
                        const float checkSize = 4.0f;
                        for (int x = 0; x < static_cast<int>(m_AlphaSlider.width / checkSize) + 1; ++x) {
                            for (int y = 0; y < static_cast<int>(m_AlphaSlider.height / checkSize) + 1; ++y) {
                                if (((x + y) % 2) == 1) {
                                    const Rect check{ m_AlphaSlider.x + x * checkSize, m_AlphaSlider.y + y * checkSize,
                                        checkSize, checkSize };
                                    context.DrawRect(check, we::runtime::kindui::Color{ 0.55f, 0.55f, 0.55f, 1.0f });
                                }
                            }
                        }

                        // Alpha color gradient
                        const int steps = 50;
                        const float segW = m_AlphaSlider.width / static_cast<float>(steps);
                        for (int x = 0; x < steps; ++x) {
                            const float alpha = static_cast<float>(x) / static_cast<float>(steps - 1);
                            const we::runtime::kindui::Color color = HSVToRGB(m_Hue, m_Saturation, m_Value, alpha);
                            const Rect segment{ m_AlphaSlider.x + x * segW, m_AlphaSlider.y, segW + 0.5f,
                                m_AlphaSlider.height };
                            context.DrawRect(segment, color);
                        }
                        context.PopClipRect();

                        context.DrawRoundedRectOutline(m_AlphaSlider,
                            we::runtime::kindui::ResolveColor(ColorToken::BorderDefault), 1.0f, cornerRadius);

                        // Alpha handle indicator
                        const float selectorX = m_AlphaSlider.x + m_Alpha * m_AlphaSlider.width;
                        const float handleW = 5.0f;
                        const Rect handle{ selectorX - handleW * 0.5f, m_AlphaSlider.y - 1.0f, handleW,
                            m_AlphaSlider.height + 2.0f };
                        context.DrawRoundedRect(handle, we::runtime::kindui::Color{ 1.0f, 1.0f, 1.0f, 1.0f }, 2.0f);
                        context.DrawRoundedRectOutline(handle, we::runtime::kindui::Color{ 0.0f, 0.0f, 0.0f, 0.8f },
                            1.0f, 2.0f);
                    }

                    void DrawColorPreview(PaintContext& context) {
                        const float cornerRadius = 3.0f;

                        // Checkerboard
                        context.DrawRoundedRect(m_ColorPreview,
                            we::runtime::kindui::ResolveColor(ColorToken::InputBackground), cornerRadius);
                        context.PushClipRect(m_ColorPreview);
                        const float checkSize = 4.0f;
                        for (int x = 0; x < static_cast<int>(m_ColorPreview.width / checkSize) + 1; ++x) {
                            for (int y = 0; y < static_cast<int>(m_ColorPreview.height / checkSize) + 1; ++y) {
                                if (((x + y) % 2) == 1) {
                                    const Rect check{ m_ColorPreview.x + x * checkSize, m_ColorPreview.y + y *
                                        checkSize, checkSize, checkSize };
                                    context.DrawRect(check, we::runtime::kindui::Color{ 0.55f, 0.55f, 0.55f, 1.0f });
                                }
                            }
                        }

                        // Vertical split: Top half = Original color, Bottom half = New selected color
                        const float halfH = m_ColorPreview.height * 0.5f;
                        const Rect origRect{ m_ColorPreview.x, m_ColorPreview.y, m_ColorPreview.width, halfH };
                        const Rect newRect{ m_ColorPreview.x, m_ColorPreview.y + halfH, m_ColorPreview.width, halfH };
                        context.DrawRect(origRect, m_OriginalColor);

                        const we::runtime::kindui::Color currentColor = HSVToRGB(m_Hue, m_Saturation, m_Value, m_Alpha);
                        context.DrawRect(newRect, currentColor);
                        context.PopClipRect();

                        context.DrawRoundedRectOutline(m_ColorPreview,
                            we::runtime::kindui::ResolveColor(ColorToken::BorderDefault), 1.0f, cornerRadius);
                    }

                    void DrawHexInput(PaintContext& context) {
                        we::runtime::kindui::ControlChrome::InteractionState state;
                        we::runtime::kindui::ControlChrome::PaintInputFrame(context, m_HexInput, state);

                        const we::runtime::kindui::Color rgb = HSVToRGB(m_Hue, m_Saturation, m_Value, m_Alpha);
                        char hex[16];
                        std::snprintf(hex, sizeof(hex), "#%02X%02X%02X%02X",
                            static_cast<int>(rgb.r * 255.0f + 0.5f), static_cast<int>(rgb.g * 255.0f + 0.5f),
                            static_cast<int>(rgb.b * 255.0f + 0.5f), static_cast<int>(rgb.a * 255.0f + 0.5f));

                        const float fontSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeProperty);
                        const float textY = we::runtime::kindui::AutoAlign::AlignTextTopY(m_HexInput, fontSize);

                        context.DrawText(hex, Point{ m_HexInput.x + 6.0f, textY },
                            we::runtime::kindui::ResolveColor(ColorToken::TextPrimary), fontSize, true);
                    }

                    void DrawRGBAValues(PaintContext& context) {
                        const float fontSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeSmall);
                        const float count = 4.0f;
                        const float boxW = (m_RGBAValues.width - (count - 1.0f) * 4.0f) / count;
                        const we::runtime::kindui::Color rgb = HSVToRGB(m_Hue, m_Saturation, m_Value, m_Alpha);

                        const float vals[4] = { rgb.r, rgb.g, rgb.b, rgb.a };
                        const char* labels[4] = { "R", "G", "B", "A" };
                        const ColorToken labelTokens[4] = { ColorToken::AxisX, ColorToken::AxisY, ColorToken::AxisZ,
                            ColorToken::TextSecondary };

                        for (int i = 0; i < 4; ++i) {
                            const Rect box{ m_RGBAValues.x + static_cast<float>(i) * (boxW + 4.0f), m_RGBAValues.y,
                                boxW, m_RGBAValues.height };

                            we::runtime::kindui::ControlChrome::InteractionState state;
                            we::runtime::kindui::ControlChrome::PaintInputFrame(context, box, state);

                            char numStr[16];
                            std::snprintf(numStr, sizeof(numStr), "%d", static_cast<int>(vals[i] * 255.0f + 0.5f));

                            const float textY = we::runtime::kindui::AutoAlign::AlignTextTopY(box, fontSize);
                            // Red/Green/Blue indicators for R, G, B components of Color/RGBA properties
                            context.DrawText(labels[i], Point{ box.x + 3.0f, textY },
                                we::runtime::kindui::ResolveColor(labelTokens[i]), fontSize, true);
                            // Value
                            context.DrawText(numStr, Point{ box.x + 14.0f, textY },
                                we::runtime::kindui::ResolveColor(ColorToken::TextPrimary), fontSize);
                        }
                    }

                    void DrawQuickPalette(PaintContext& context) {
                        if (m_Presets.empty()) return;
                        const float count = static_cast<float>(m_Presets.size());
                        const float swatchW = (m_QuickPaletteRect.width - (count - 1.0f) * 4.0f) / count;
                        for (size_t i = 0; i < m_Presets.size(); ++i) {
                            const Rect swatch{ m_QuickPaletteRect.x + static_cast<float>(i) * (swatchW + 4.0f),
                                m_QuickPaletteRect.y, swatchW, m_QuickPaletteRect.height };
                            context.DrawRoundedRect(swatch, m_Presets[i], 3.0f);
                            context.DrawRoundedRectOutline(swatch,
                                we::runtime::kindui::ResolveColor(ColorToken::BorderDefault), 1.0f, 3.0f);
                        }
                    }

                    void DrawFooterButtons(PaintContext& context) {
                        // Cancel Button using standard Content Browser / Design System button chrome
                        const auto secStyle =
                            we::runtime::kindui::ControlChrome::Role(we::runtime::kindui::StyleRole::ButtonSecondary);
                        we::runtime::kindui::ControlChrome::InteractionState cancelState;
                        cancelState.hoverAnim = m_CancelHovered ? 1.0f : 0.0f;
                        cancelState.pressAnim = m_CancelPressed ? 1.0f : 0.0f;
                        we::runtime::kindui::ControlChrome::PaintFilledButton(context, m_CancelButtonRect, secStyle,
                            cancelState);

                        const float fontSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeProperty);
                        const float cancelW = context.GetTextWidth("Cancel", fontSize);
                        const float cancelY = m_CancelButtonRect.y + (m_CancelButtonRect.height - fontSize) * 0.5f -
                            1.0f;
                        const float cancelX = m_CancelButtonRect.x + (m_CancelButtonRect.width - cancelW) * 0.5f;
                        context.DrawText("Cancel", Point{ cancelX, cancelY },
                            we::runtime::kindui::ResolveColor(ColorToken::TextPrimary), fontSize);

                        // OK Button using standard Content Browser / Design System button chrome
                        const auto priStyle =
                            we::runtime::kindui::ControlChrome::Role(we::runtime::kindui::StyleRole::ButtonPrimary);
                        we::runtime::kindui::ControlChrome::InteractionState okState;
                        okState.hoverAnim = m_OKHovered ? 1.0f : 0.0f;
                        okState.pressAnim = m_OKPressed ? 1.0f : 0.0f;
                        we::runtime::kindui::ControlChrome::PaintFilledButton(context, m_OKButtonRect, priStyle,
                            okState);

                        const float okW = context.GetTextWidth("OK", fontSize, true);
                        const float okY = m_OKButtonRect.y + (m_OKButtonRect.height - fontSize) * 0.5f - 1.0f;
                        const float okX = m_OKButtonRect.x + (m_OKButtonRect.width - okW) * 0.5f;
                        context.DrawText("OK", Point{ okX, okY },
                            we::runtime::kindui::ResolveColor(ColorToken::TextPrimary), fontSize, true);
                    }

                    void HandleRadialWheelInteraction(const Point& position) {
                        const float cx = m_RadialWheelRect.x + m_RadialWheelRect.width * 0.5f;
                        const float cy = m_RadialWheelRect.y + m_RadialWheelRect.height * 0.5f;
                        const float radius = (std::min)(m_RadialWheelRect.width, m_RadialWheelRect.height) * 0.5f -
                            1.0f;

                        const float dx = position.x - cx;
                        const float dy = position.y - cy;
                        const float dist = std::sqrt(dx * dx + dy * dy);

                        m_Saturation = std::clamp(dist / (radius > 0.001f ? radius : 1.0f), 0.0f, 1.0f);

                        const float M_PI_F = 3.1415926535f;
                        float angle = std::atan2(-dy, dx);
                        if (angle < 0.0f) {
                            angle += 2.0f * M_PI_F;
                        }
                        m_Hue = angle / (2.0f * M_PI_F);
                        UpdateColor();
                    }

                    void HandleValueSliderInteraction(const Point& position) {
                        m_Value = std::clamp(1.0f - (position.y - m_ValueSlider.y) / m_ValueSlider.height, 0.0f, 1.0f);
                        UpdateColor();
                    }

                    void HandleAlphaSliderInteraction(const Point& position) {
                        m_Alpha = std::clamp((position.x - m_AlphaSlider.x) / m_AlphaSlider.width, 0.0f, 1.0f);
                        UpdateColor();
                    }

                    void SetColorFromRGBA(const we::runtime::kindui::Color& color) {
                        m_Alpha = color.a;
                        HSVColor hsv = RGBToHSV(color.r, color.g, color.b);
                        m_Hue = hsv.h;
                        m_Saturation = hsv.s;
                        m_Value = hsv.v;
                        UpdateColor();
                    }

                    void UpdateColor() {
                        const we::runtime::kindui::Color rgb = HSVToRGB(m_Hue, m_Saturation, m_Value, m_Alpha);
                        m_Values[0] = rgb.r;
                        m_Values[1] = rgb.g;
                        m_Values[2] = rgb.b;
                        m_Values[3] = rgb.a;
                        m_OnColorChanged();
                    }

                    struct HSVColor { float h, s, v; };
                    static HSVColor RGBToHSV(float r, float g, float b) {
                        const float max = std::max({ r, g, b });
                        const float min = std::min({ r, g, b });
                        const float delta = max - min;

                        float h = 0.0f, s = 0.0f, v = max;

                        if (delta > 0.0001f) {
                            s = delta / (max > 0.0001f ? max : 1.0f);
                            if (max == r) {
                                h = (g - b) / delta + (g < b ? 6.0f : 0.0f);
                            } else if (max == g) {
                                h = (b - r) / delta + 2.0f;
                            } else {
                                h = (r - g) / delta + 4.0f;
                            }
                            h /= 6.0f;
                        }

                        return HSVColor{ h, s, v };
                    }

                    static we::runtime::kindui::Color HSVToRGB(float h, float s, float v, float a) {
                        const float i = std::floor(h * 6.0f);
                        const float f = h * 6.0f - i;
                        const float p = v * (1.0f - s);
                        const float q = v * (1.0f - f * s);
                        const float t = v * (1.0f - (1.0f - f) * s);

                        float r = 0.0f, g = 0.0f, b = 0.0f;

                        switch (static_cast<int>(i) % 6) {
                            case 0: r = v; g = t; b = p; break;
                            case 1: r = q; g = v; b = p; break;
                            case 2: r = p; g = v; b = t; break;
                            case 3: r = p; g = q; b = v; break;
                            case 4: r = t; g = p; b = v; break;
                            case 5: r = v; g = p; b = q; break;
                        }

                        return we::runtime::kindui::Color{ r, g, b, a };
                    }

                    float* m_Values;
                    std::function<void()> m_OnColorChanged;
                    std::function<void()> m_OnClose;
                    we::runtime::kindui::Color m_OriginalColor;
                    std::vector<we::runtime::kindui::Color> m_Presets;

                    float m_Hue = 0.0f;
                    float m_Saturation = 0.0f;
                    float m_Value = 1.0f;
                    float m_Alpha = 1.0f;

                    bool m_DraggingWheel = false;
                    bool m_DraggingValue = false;
                    bool m_DraggingAlpha = false;
                    bool m_OKHovered = false;
                    bool m_OKPressed = false;
                    bool m_CancelHovered = false;
                    bool m_CancelPressed = false;

                    // Layout rects
                    we::runtime::kindui::Rect m_RadialWheelRect{};
                    we::runtime::kindui::Rect m_ValueSlider{};
                    we::runtime::kindui::Rect m_AlphaSlider{};
                    we::runtime::kindui::Rect m_ColorPreview{};
                    we::runtime::kindui::Rect m_HexInput{};
                    we::runtime::kindui::Rect m_RGBAValues{};
                    we::runtime::kindui::Rect m_QuickPaletteRect{};
                    we::runtime::kindui::Rect m_FooterRect{};
                    we::runtime::kindui::Rect m_OKButtonRect{};
                    we::runtime::kindui::Rect m_CancelButtonRect{};
                };

                // Compact popover window container class for clean title bar & contextual positioning
                class ColorPickerPopoverFrame final : public Widget {
                public:
                    ColorPickerPopoverFrame(std::shared_ptr<Widget> content, std::function<void()> onClose)
                        : m_Content(std::move(content)), m_OnClose(std::move(onClose)) {
                        if (m_Content) {
                            AddChild(m_Content);
                        }
                    }

                    Size Measure(const Size& available) override {
                        const float headerH = 32.0f;
                        Size contentDesired{ 320.0f, 345.0f };
                        if (m_Content) {
                            contentDesired = m_Content->Measure(Size{ available.width, available.height - headerH });
                        }
                        return Size{ contentDesired.width, contentDesired.height + headerH };
                    }

                    void Arrange(const Rect& r) override {
                        m_Geometry = r;
                        const float headerH = 32.0f;
                        m_HeaderRect = Rect{ r.x, r.y, r.width, headerH };
                        m_LogoRect = Rect{ r.x + 10.0f, r.y + (headerH - 18.0f) * 0.5f, 18.0f, 18.0f };
                        m_CloseButtonRect = Rect{ r.x + r.width - 26.0f, r.y + (headerH - 20.0f) * 0.5f, 20.0f, 20.0f };

                        if (m_Content) {
                            m_Content->Arrange(Rect{ r.x, r.y + headerH, r.width, r.height - headerH });
                        }
                    }

                    void Paint(PaintContext& context) override {
                        // Popover shadow & card background
                        we::runtime::kindui::ControlChrome::PaintPopupShadow(context, m_Geometry, 6.0f);
                        context.DrawRoundedRect(m_Geometry,
                            we::runtime::kindui::ResolveColor(ColorToken::PopupBackground), 6.0f);

                        // Header background (top rounded)
                        context.DrawRoundedRect(m_HeaderRect,
                            we::runtime::kindui::ResolveColor(ColorToken::HeaderBackground), 6.0f);
                        context.DrawRect(Rect{ m_HeaderRect.x, m_HeaderRect.y + m_HeaderRect.height - 6.0f,
                            m_HeaderRect.width, 6.0f },
                            we::runtime::kindui::ResolveColor(ColorToken::HeaderBackground));

                        // Header separator line
                        context.DrawLine(Point{ m_HeaderRect.x, m_HeaderRect.y + m_HeaderRect.height },
                                         Point{ m_HeaderRect.x + m_HeaderRect.width, m_HeaderRect.y +
                                             m_HeaderRect.height },
                                         we::runtime::kindui::ResolveColor(ColorToken::Separator), 1.0f);

                        // Engine logo icon (Assets/Icons/WindIcons/logo_24.png)
                        IconPainter::Draw(context, we::runtime::kindui::WindIcons::Logo24, m_LogoRect);

                        // Window title centered in title bar
                        const float fontSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeSmall);
                        const float titleW = context.GetTextWidth("Color Picker", fontSize, true);
                        const float textY = m_HeaderRect.y + (m_HeaderRect.height - fontSize) * 0.5f - 1.0f;
                        const float textX = m_HeaderRect.x + (m_HeaderRect.width - titleW) * 0.5f;
                        context.DrawText("Color Picker", Point{ textX, textY },
                            we::runtime::kindui::ResolveColor(ColorToken::TextPrimary), fontSize, true);

                        // Native close button (X)
                        if (m_ClosePressed) {
                            context.DrawRoundedRect(m_CloseButtonRect,
                                we::runtime::kindui::ResolveColor(ColorToken::PressedBackground), 3.0f);
                        } else if (m_CloseHovered) {
                            context.DrawRoundedRect(m_CloseButtonRect,
                                we::runtime::kindui::ResolveColor(ColorToken::HoverBackground), 3.0f);
                        }
                        const Color xColor =
                            m_CloseHovered ? we::runtime::kindui::ResolveColor(ColorToken::TextPrimary) :
                            we::runtime::kindui::ResolveColor(ColorToken::TextSecondary);
                        IconPainter::Draw(context, we::runtime::kindui::WindIcons::X16, m_CloseButtonRect, xColor);

                        // Outer border frame
                        context.DrawRoundedRectOutline(m_Geometry,
                            we::runtime::kindui::ResolveColor(ColorToken::BorderDefault), 1.0f, 6.0f);

                        // Paint content
                        if (m_Content && m_Content->IsVisible()) {
                            m_Content->Paint(context);
                        }
                    }

                    void OnMouseDown(const MouseEvent& event) override {
                        if (event.button == MouseButton::Left && m_CloseButtonRect.Contains(event.position)) {
                            m_ClosePressed = true;
                            if (m_OnClose) {
                                m_OnClose();
                            }
                            return;
                        }
                        if (m_Content && m_Content->GetGeometry().Contains(event.position)) {
                            m_Content->OnMouseDown(event);
                        }
                    }

                    void OnMouseMove(const MouseEvent& event) override {
                        const bool hovered = m_CloseButtonRect.Contains(event.position);
                        if (hovered != m_CloseHovered) {
                            m_CloseHovered = hovered;
                        }
                        if (m_Content && m_Content->GetGeometry().Contains(event.position)) {
                            m_Content->OnMouseMove(event);
                        }
                    }

                    void OnMouseUp(const MouseEvent& event) override {
                        m_ClosePressed = false;
                        if (m_Content && m_Content->GetGeometry().Contains(event.position)) {
                            m_Content->OnMouseUp(event);
                        }
                    }

                private:
                    std::shared_ptr<Widget> m_Content;
                    std::function<void()> m_OnClose;
                    Rect m_HeaderRect{};
                    Rect m_LogoRect{};
                    Rect m_CloseButtonRect{};
                    bool m_CloseHovered = false;
                    bool m_ClosePressed = false;
                };

                // Instantiate color picker content & popover frame
                auto content = std::make_shared<ColorPickerContent>(m_Values, [this]() {
                    Write();
                }, [popupHost]() {
                    if (popupHost) {
                        popupHost->CloseTopPopup();
                    }
                });

                auto popoverFrame = std::make_shared<ColorPickerPopoverFrame>(content, [popupHost]() {
                    if (popupHost) {
                        popupHost->CloseTopPopup();
                    }
                });

                // Contextual popover positioning via shared placement utility: anchor to clicked Color control field
                popupHost->ShowAnchoredPopup(popoverFrame, m_Geometry,
                    we::runtime::kindui::PopupPlacementMode::SidePreferred);
            }

            bool ShowsPointerCursor(const Point& position) const override {
                return m_Geometry.Contains(position);
            }

            void Paint(PaintContext& context) override {
                Chrome::InteractionState state;
                const Rect frame = EditorControlRect(m_Geometry);
                Chrome::PaintInputFrame(context, frame, state);

                // Draw the actual color swatch/fill inside the frame
                we::runtime::kindui::Color c{ m_Values[0], m_Values[1], m_Values[2], m_Values[3] };
                const float ruleInset = std::max(1.0f, ResolveMetric(MetricToken::BorderWidth));
                const float cornerRadius = ResolveMetric(MetricToken::CornerRadiusSmall);
                const Rect colorRect{
                    frame.x + ruleInset,
                    frame.y + ruleInset,
                    std::max(0.0f, frame.width - ruleInset * 2.0f),
                    std::max(0.0f, frame.height - ruleInset * 2.0f)
                };
                context.DrawRoundedRect(colorRect, c, std::max(0.0f, cornerRadius - ruleInset));
            }

        private:
            void Read() {
                std::memset(m_Values, 0, sizeof(m_Values));
                if (!m_Handle) {
                    return;
                }
                if (const auto* prop = m_Handle->GetPropertyInfo()) {
                    const std::size_t count = prop->primitive == PrimitiveKind::Vec3 ? 3 : 4;
                    (void)m_Handle->GetRaw(m_Values, sizeof(float) * count);
                    if (count == 3) {
                        m_Values[3] = 1.0f;
                    }
                }
            }

            void Write() {
                if (!m_Handle) {
                    return;
                }
                if (const auto* prop = m_Handle->GetPropertyInfo()) {
                    const std::size_t count = prop->primitive == PrimitiveKind::Vec3 ? 3 : 4;
                    (void)m_Handle->SetRaw(m_Values, sizeof(float) * count);
                }
            }

            PropertyHandlePtr m_Handle;
            float m_Values[4]{};
        };
        return std::make_shared<ColorWidget>(m_Handle);
    }
};

// --- Struct (label only; children rendered by tree) ---
class StructEditor final : public BoundEditorBase {
public:
    [[nodiscard]] bool CanEdit(const PropertyInfo&) const override { return true; }
    [[nodiscard]] std::shared_ptr<Widget> CreateWidget() override {
        class StructWidget final : public Widget {
        public:
            Size Measure(const Size& available) override { return Size{ available.width, EditorControlHeight() }; }
            void Arrange(const Rect& r) override { m_Geometry = r; }
            void Paint(PaintContext& context) override {
                const float fontSize = ResolveMetric(MetricToken::TextSizeProperty);
                context.DrawText(
                    "{...}",
                    Point{ m_Geometry.x + ResolveMetric(MetricToken::Space2), LayoutMetrics::AlignTextTopY(m_Geometry,
                        fontSize) },
                    ThemeColor(ColorToken::TextSecondary),
                    fontSize);
            }
        };
        return std::make_shared<StructWidget>();
    }
};

// --- Array stub ---
class ArrayStubEditor final : public BoundEditorBase {
public:
    [[nodiscard]] bool CanEdit(const PropertyInfo&) const override { return true; }
    [[nodiscard]] std::shared_ptr<Widget> CreateWidget() override {
        class Stub final : public Widget {
        public:
            Size Measure(const Size& available) override { return Size{ available.width, EditorControlHeight() }; }
            void Arrange(const Rect& r) override { m_Geometry = r; }
            void Paint(PaintContext& context) override {
                context.DrawText(
                    "[Array — stub]",
                    Point{m_Geometry.x + 4.f, m_Geometry.y + 2.f},
                    ThemeColor(ColorToken::TextSecondary),
                    ThemeMetric(MetricToken::TextSizeBody));
            }
        };
        return std::make_shared<Stub>();
    }
};

class MapSetStubEditor final : public BoundEditorBase {
public:
    explicit MapSetStubEditor(const char* label) : m_Label(label) {}
    [[nodiscard]] bool CanEdit(const PropertyInfo&) const override { return true; }
    [[nodiscard]] std::shared_ptr<Widget> CreateWidget() override {
        class Stub final : public Widget {
        public:
            explicit Stub(const char* label) : m_Label(label) {}
            Size Measure(const Size& available) override { return Size{ available.width, EditorControlHeight() }; }
            void Arrange(const Rect& r) override { m_Geometry = r; }
            void Paint(PaintContext& context) override {
                context.DrawText(
                    m_Label,
                    Point{m_Geometry.x + 4.f, m_Geometry.y + 2.f},
                    ThemeColor(ColorToken::TextSecondary),
                    ThemeMetric(MetricToken::TextSizeBody));
            }
            const char* m_Label = "";
        };
        return std::make_shared<Stub>(m_Label);
    }

private:
    const char* m_Label = "";
};

namespace {

enum class AssetKind {
    Unknown,
    Texture,
    Material,
    StaticMesh,
    SkeletalMesh,
    Animation,
    Blueprint,
    Scene,
    Prefab,
    Audio,
    Script,
    Font,
    Video
};

[[nodiscard]] inline AssetKind DeduceAssetKindFromPathOrType(std::string_view pathOrType) {
    std::string s(pathOrType);
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

    if (s.find("texture") != std::string::npos || s.find(".png") != std::string::npos ||
        s.find(".jpg") != std::string::npos || s.find(".tga") != std::string::npos ||
        s.find(".dds") != std::string::npos || s.find(".hdr") != std::string::npos) {
        return AssetKind::Texture;
    }
    if (s.find("material") != std::string::npos || s.find(".mat") != std::string::npos) {
        return AssetKind::Material;
    }
    if (s.find("skeletal") != std::string::npos || s.find(".skel") != std::string::npos) {
        return AssetKind::SkeletalMesh;
    }
    if (s.find("mesh") != std::string::npos || s.find(".obj") != std::string::npos ||
        s.find(".fbx") != std::string::npos || s.find(".gltf") != std::string::npos ||
        s.find(".glb") != std::string::npos) {
        return AssetKind::StaticMesh;
    }
    if (s.find("anim") != std::string::npos) {
        return AssetKind::Animation;
    }
    if (s.find("prefab") != std::string::npos || s.find(".prefab") != std::string::npos) {
        return AssetKind::Prefab;
    }
    if (s.find("scene") != std::string::npos || s.find(".scene") != std::string::npos || s.find(".map") !=
        std::string::npos) {
        return AssetKind::Scene;
    }
    if (s.find("audio") != std::string::npos || s.find("sound") != std::string::npos ||
        s.find(".wav") != std::string::npos || s.find(".mp3") != std::string::npos || s.find(".ogg") !=
            std::string::npos) {
        return AssetKind::Audio;
    }
    if (s.find("script") != std::string::npos || s.find(".lua") != std::string::npos ||
        s.find(".cs") != std::string::npos || s.find(".py") != std::string::npos) {
        return AssetKind::Script;
    }
    if (s.find("font") != std::string::npos || s.find(".ttf") != std::string::npos || s.find(".otf") !=
        std::string::npos) {
        return AssetKind::Font;
    }
    return AssetKind::Unknown;
}

[[nodiscard]] inline const char* AssetKindToString(AssetKind kind) {
    switch (kind) {
        case AssetKind::Texture: return "Texture";
        case AssetKind::Material: return "Material";
        case AssetKind::StaticMesh: return "Static Mesh";
        case AssetKind::SkeletalMesh: return "Skeletal Mesh";
        case AssetKind::Animation: return "Animation";
        case AssetKind::Blueprint: return "Blueprint";
        case AssetKind::Scene: return "Scene";
        case AssetKind::Prefab: return "Prefab";
        case AssetKind::Audio: return "Audio";
        case AssetKind::Script: return "Script";
        case AssetKind::Font: return "Font";
        case AssetKind::Video: return "Video";
        default: return "Asset";
    }
}

[[nodiscard]] inline we::runtime::kindui::WindIconRef IconForAssetKind(AssetKind kind) {
    namespace WindIcons = we::runtime::kindui::WindIcons;
    switch (kind) {
        case AssetKind::Texture: return WindIcons::Grid16;
        case AssetKind::Material: return WindIcons::Sun16;
        case AssetKind::StaticMesh: return WindIcons::BoxSolid16;
        case AssetKind::SkeletalMesh: return WindIcons::Box16;
        case AssetKind::Animation: return WindIcons::PlayForward16;
        case AssetKind::Blueprint: return WindIcons::Wrench16;
        case AssetKind::Scene: return WindIcons::Earth16;
        case AssetKind::Prefab: return WindIcons::Box16;
        case AssetKind::Audio: return WindIcons::Cloud16;
        case AssetKind::Script: return WindIcons::Console16;
        case AssetKind::Font: return WindIcons::Square16;
        case AssetKind::Video: return WindIcons::ToolbarVideocamera16;
        default: return WindIcons::Box16;
    }
}

[[nodiscard]] inline bool IsAssetReferenceProperty(const PropertyInfo& property) {
    if (HasFlag(property.flags, PropertyFlags::AssetReference) ||
        HasFlag(property.flags, PropertyFlags::EntityReference)) {
        return true;
    }
    const bool isString = (property.primitive == PrimitiveKind::String) ||
                          (property.typeId == we::runtime::reflection::BuiltinTypeId::String());
    if (isString) {
        std::string s = property.name;
        for (char& c : s) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (s.find("material") != std::string::npos ||
            s.find("texture") != std::string::npos ||
            s.find("mesh") != std::string::npos ||
            s.find("asset") != std::string::npos ||
            s.find("sound") != std::string::npos ||
            s.find("audio") != std::string::npos ||
            s.find("script") != std::string::npos ||
            s.find("prefab") != std::string::npos ||
            s.find("scene") != std::string::npos ||
            s.find("font") != std::string::npos ||
            s.find("clippath") != std::string::npos) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] inline bool IsVisualAssetKind(AssetKind kind) noexcept {
    switch (kind) {
    case AssetKind::Texture:
    case AssetKind::Material:
    case AssetKind::StaticMesh:
    case AssetKind::SkeletalMesh:
    case AssetKind::Animation:
    case AssetKind::Blueprint:
    case AssetKind::Prefab:
    case AssetKind::Scene:
        return true;
    default:
        return false;
    }
}

}

class AssetReferenceEditor final : public BoundEditorBase {
public:
    [[nodiscard]] bool CanEdit(const PropertyInfo& property) const override {
        return IsAssetReferenceProperty(property);
    }

    [[nodiscard]] std::shared_ptr<Widget> CreateWidget() override {
        class AssetReferenceWidget final : public Widget {
        public:
            explicit AssetReferenceWidget(PropertyHandlePtr handle) : m_Handle(std::move(handle)) {
                Read();
            }

            Size Measure(const Size& available) override {
                Read();
                const float scale = DPIContext::GetScale();
                return Size{ available.width, 76.0f * scale };
            }

            void Arrange(const Rect& r) override {
                m_Geometry = r;
                const Rect frame = EditorControlRect(r);
                const float scale = DPIContext::GetScale();
                const float pad = 2.0f * scale;

                Read();


                const float thumbSize = std::min(frame.height - pad * 2.0f, 64.0f * scale);
                m_ThumbRect = Rect{ frame.x, frame.y + (frame.height - thumbSize) * 0.5f, thumbSize, thumbSize };

                const float colX = m_ThumbRect.x + m_ThumbRect.width + 8.0f * scale;
                const float btnW = 20.0f * scale;
                const float rightEdge = frame.x + frame.width;

                const float pillH = 26.0f * scale;
                const float pillY = frame.y + pad;

                // Top-row: Asset Pill Dropdown Selector occupies full available width
                const float availW = std::max(0.0f, rightEdge - colX);
                m_PillRect = Rect{ colX, pillY, availW, pillH };

                // Bottom-row navigation action buttons (Use Selected, Locate in Content Browser, Clear/Trash)
                const float btnY = m_PillRect.y + m_PillRect.height + 6.0f * scale;
                m_UseSelectedBtnRect = Rect{ colX, btnY, btnW, btnW };
                m_LocateBtnRect = Rect{ colX + btnW + 4.0f * scale, btnY, btnW, btnW };
                m_ClearBtnRect = Rect{ colX + (btnW + 4.0f * scale) * 2.0f, btnY, btnW, btnW };

                m_IconRect = Rect{};
                m_FieldRect = Rect{ frame.x, frame.y, frame.width, frame.height };
            }

            void Paint(PaintContext& context) override {
                const float scale = DPIContext::GetScale();
                const float fontSize = ResolveMetric(MetricToken::TextSizeProperty);

                Read();

                const bool hasAsset = !m_Path.empty();
                std::string basename;
                AssetKind kind = GetAssetKind();

                if (hasAsset) {
                    const auto lastSlash = m_Path.find_last_of("/\\");
                    basename = (lastSlash != std::string::npos) ? m_Path.substr(lastSlash + 1) : m_Path;
                }


                Chrome::InteractionState thumbState;
                thumbState.hoverAnim = m_Hovered ? 0.4f : 0.0f;
                Chrome::PaintInputFrame(context, m_ThumbRect, thumbState);

                // Centered asset-type icon / thumbnail inside preview box
                const float thumbIconSize = 36.0f * scale;
                const Rect thumbIconR{
                    m_ThumbRect.x + (m_ThumbRect.width - thumbIconSize) * 0.5f,
                    m_ThumbRect.y + (m_ThumbRect.height - thumbIconSize) * 0.5f,
                    thumbIconSize, thumbIconSize
                };
                const WindIconRef icon = IconForAssetKind(kind);
                IconPainter::Draw(context, icon, thumbIconR, ThemeColor(hasAsset ? ColorToken::IconPrimary :
                    ColorToken::IconSecondary));

                // 2. Top-Row: Asset Pill Dropdown Selector with Reusable Extension-Preserving Text Truncation
                Chrome::InteractionState pillState;
                pillState.hoverAnim = m_Hovered ? 1.0f : 0.0f;
                Chrome::PaintInputFrame(context, m_PillRect, pillState);

                const float chevronSize = 12.0f * scale;
                const float textX = m_PillRect.x + 8.0f * scale;
                const float maxTextWidth = std::max(0.0f, m_PillRect.width - chevronSize - 16.0f * scale);

                const float textY = LayoutMetrics::AlignTextTopY(m_PillRect, fontSize);
                if (hasAsset) {
                    const std::string truncated = we::runtime::kindui::TextMetrics::TruncateText(
                        basename, maxTextWidth, fontSize, true, we::runtime::kindui::TruncateMode::Middle);
                    context.DrawText(truncated, Point{ textX, textY }, ThemeColor(ColorToken::TextPrimary), fontSize,
                        true);
                } else {
                    context.DrawText("None", Point{ textX, textY }, ThemeColor(ColorToken::TextSecondary), fontSize);
                }

                const Rect chevronR{ m_PillRect.x + m_PillRect.width - chevronSize - 6.0f * scale, m_PillRect.y +
                    (m_PillRect.height - chevronSize) * 0.5f, chevronSize, chevronSize };
                IconPainter::Draw(context, WindIcons::ChevronDownV212, chevronR, ThemeColor(ColorToken::IconSecondary));

                // 3. Navigation Action Buttons (Bottom Row)
                // (a) Circle Arrow Left (Use Selected from Content Browser)
                Chrome::InteractionState useSelState;
                useSelState.hoverAnim = m_UseSelectedHovered ? 1.0f : 0.0f;
                Chrome::PaintBorderlessIconButton(context, m_UseSelectedBtnRect, useSelState);
                IconPainter::Draw(context, WindIcons::CircleArrowLeft16, m_UseSelectedBtnRect,
                    ThemeColor(m_UseSelectedHovered ? ColorToken::IconPrimary : ColorToken::IconSecondary));

                // (b) Folder Search (Locate in Content Browser)
                Chrome::InteractionState locateState;
                locateState.hoverAnim = m_LocateHovered ? 1.0f : 0.0f;
                Chrome::PaintBorderlessIconButton(context, m_LocateBtnRect, locateState);
                IconPainter::Draw(context, WindIcons::FolderSearch16, m_LocateBtnRect,
                    ThemeColor(m_LocateHovered ? ColorToken::IconPrimary : ColorToken::IconSecondary));

                // (c) Trash / Clear Button (Clear Asset to None)
                if (hasAsset) {
                    Chrome::InteractionState clearState;
                    clearState.hoverAnim = m_ClearHovered ? 1.0f : 0.0f;
                    Chrome::PaintBorderlessIconButton(context, m_ClearBtnRect, clearState);
                    IconPainter::Draw(context, WindIcons::CircleX16, m_ClearBtnRect,
                        ThemeColor(m_ClearHovered ? ColorToken::IconPrimary : ColorToken::IconSecondary));
                }

                // 4. Hover Tooltip for Full Asset Name Display
                if (m_Hovered && hasAsset) {
                    const float fullWidth = we::runtime::kindui::TextMetrics::MeasureWidth(basename, fontSize, true);
                    if (fullWidth > maxTextWidth) {
                        const float padH = 8.0f * scale;
                        const float padV = 4.0f * scale;
                        const float tooltipW = fullWidth + padH * 2.0f;
                        const float tooltipH = fontSize + padV * 2.0f;
                        const Rect tooltipRect{
                            m_PillRect.x,
                            std::max(0.0f, m_PillRect.y - tooltipH - 4.0f * scale),
                            tooltipW,
                            tooltipH
                        };
                        we::runtime::kindui::ControlChrome::PaintTooltipSurface(context, tooltipRect);
                        context.DrawText(basename, Point{ tooltipRect.x + padH, tooltipRect.y + padV },
                            ThemeColor(ColorToken::TextPrimary), fontSize);
                    }
                }
            }

            void OnMouseMove(const MouseEvent& event) override {
                m_Hovered = m_FieldRect.Contains(event.position) || m_PillRect.Contains(event.position) ||
                    m_ThumbRect.Contains(event.position);
                m_UseSelectedHovered = m_UseSelectedBtnRect.Contains(event.position);
                m_LocateHovered = m_LocateBtnRect.Contains(event.position);
                m_ClearHovered = m_ClearBtnRect.Contains(event.position);
                InvalidatePaint();
            }

            void OnMouseDown(const MouseEvent& event) override {
                if (event.button != MouseButton::Left || !m_Handle || m_Handle->IsReadOnly()) {
                    return;
                }

                // Use Selected button clicked
                if (m_UseSelectedBtnRect.Contains(event.position)) {
                    const AssetKind kind = GetAssetKind();
                    if (kind == AssetKind::Material) {
                        (void)m_Handle->SetString("Assets/Materials/M_Rock_Mat.mat");
                    } else if (kind == AssetKind::StaticMesh) {
                        (void)m_Handle->SetString("Assets/Meshes/SM_Rock_01.obj");
                    } else if (kind == AssetKind::Texture) {
                        (void)m_Handle->SetString("Assets/Textures/T_Rock_Albedo.png");
                    } else {
                        (void)m_Handle->SetString("Assets/Textures/T_Rock_Albedo.png");
                    }
                    InvalidatePaint();
                    return;
                }

                // Clear button clicked
                if (!m_Path.empty() && m_ClearBtnRect.Contains(event.position)) {
                    m_Path.clear();
                    (void)m_Handle->SetString("");
                    InvalidatePaint();
                    return;
                }

                // Locate button clicked
                if (!m_Path.empty() && m_LocateBtnRect.Contains(event.position)) {
                    WE_LOG_INFO("AssetReference", ("Locating asset: " + m_Path).c_str());
                    return;
                }

                // Field / Thumbnail / Pill clicked -> Open Asset Picker Popover
                if (m_FieldRect.Contains(event.position) || m_PillRect.Contains(event.position) ||
                    m_ThumbRect.Contains(event.position)) {
                    OpenAssetPicker();
                }
            }

            bool ShowsPointerCursor(const Point& position) const override {
                return m_Geometry.Contains(position);
            }

        private:
            [[nodiscard]] AssetKind GetAssetKind() const {
                if (!m_Path.empty()) {
                    return DeduceAssetKindFromPathOrType(m_Path);
                }
                const auto* prop = m_Handle ? m_Handle->GetPropertyInfo() : nullptr;
                if (prop) {
                    return DeduceAssetKindFromPathOrType(prop->name);
                }
                return AssetKind::Unknown;
            }

            void OpenAssetPicker() {
                auto* popupHost = we::programs::editor::GetEditorPopupHost();
                if (!popupHost) {
                    return;
                }

                class AssetPickerPopover final : public Widget {
                public:
                    AssetPickerPopover(PropertyHandlePtr handle, std::string currentPath, std::function<void()> onClose)
                        : m_Handle(std::move(handle)), m_CurrentPath(std::move(currentPath)),
                            m_OnClose(std::move(onClose)) {

                        m_Assets = {
                            { "None", "", AssetKind::Unknown },
                            { "T_Rock_Albedo.png", "Assets/Textures/T_Rock_Albedo.png", AssetKind::Texture },
                            { "T_Brick_Normal.png", "Assets/Textures/T_Brick_Normal.png", AssetKind::Texture },
                            { "M_Default_Mat.mat", "Assets/Materials/M_Default_Mat.mat", AssetKind::Material },
                            { "M_Rock_Mat.mat", "Assets/Materials/M_Rock_Mat.mat", AssetKind::Material },
                            { "SM_Rock_01.obj", "Assets/Meshes/SM_Rock_01.obj", AssetKind::StaticMesh },
                            { "SM_Cube.obj", "Assets/Meshes/SM_Cube.obj", AssetKind::StaticMesh },
                            { "A_Wind_Sound.wav", "Assets/Audio/A_Wind_Sound.wav", AssetKind::Audio },
                        };
                    }

                    Size Measure(const Size& available) override {
                        return Size{ 260.0f, 220.0f };
                    }

                    void Arrange(const Rect& r) override {
                        m_Geometry = r;
                        const float margin = 8.0f;
                        m_SearchRect = Rect{ r.x + margin, r.y + margin, r.width - margin * 2.0f, 24.0f };
                        m_ListRect = Rect{ r.x + margin, r.y + margin + 28.0f, r.width - margin * 2.0f, r.height -
                            margin * 2.0f - 28.0f };
                    }

                    void Paint(PaintContext& context) override {
                        context.DrawRoundedRect(m_Geometry, ThemeColor(ColorToken::PanelBackground), 4.0f);
                        context.DrawRoundedRectOutline(m_Geometry, ThemeColor(ColorToken::BorderDefault), 1.0f, 4.0f);

                        Chrome::InteractionState searchState;
                        Chrome::PaintInputFrame(context, m_SearchRect, searchState);
                        const float fontSize = ResolveMetric(MetricToken::TextSizeProperty);
                        const float searchY = we::runtime::kindui::AutoAlign::AlignTextTopY(m_SearchRect, fontSize);
                        context.DrawText("Search assets...", Point{ m_SearchRect.x +
                            ResolveMetric(MetricToken::Space2), searchY },
                            ThemeColor(ColorToken::TextSecondary), fontSize);

                        const float itemH = ResolveMetric(MetricToken::ListRowHeight);
                        float y = m_ListRect.y;
                        for (size_t i = 0; i < m_Assets.size(); ++i) {
                            if (y + itemH > m_ListRect.y + m_ListRect.height) break;

                            const Rect itemRect{ m_ListRect.x, y, m_ListRect.width, itemH };
                            const bool isHovered = itemRect.Contains(m_MousePos);
                            const bool isSelected = (m_Assets[i].path == m_CurrentPath) || (m_Assets[i].path.empty() &&
                                m_CurrentPath.empty());

                            if (isSelected) {
                                context.DrawRoundedRect(itemRect, ThemeColor(ColorToken::PressedBackground), 3.0f);
                            } else if (isHovered) {
                                context.DrawRoundedRect(itemRect, ThemeColor(ColorToken::HoverBackground), 3.0f);
                            }

                            const WindIconRef icon = IconForAssetKind(m_Assets[i].kind);
                            const float iconSize = ResolveMetric(MetricToken::IconSizePrimary);
                            const float iconY = itemRect.y + (itemRect.height - iconSize) * 0.5f;
                            const Rect iconR{ itemRect.x + ResolveMetric(MetricToken::Space1), iconY, iconSize,
                                iconSize };
                            IconPainter::Draw(context, icon, iconR, ThemeColor(ColorToken::IconPrimary));

                            const float itemTextY = we::runtime::kindui::AutoAlign::AlignTextTopY(itemRect, fontSize);
                            context.DrawText(m_Assets[i].name, Point{ itemRect.x + ResolveMetric(MetricToken::Space6),
                                itemTextY },
                                ThemeColor(ColorToken::TextPrimary), fontSize);

                            y += itemH + ResolveMetric(MetricToken::FormRowGap);
                        }
                    }

                    void OnMouseMove(const MouseEvent& event) override {
                        m_MousePos = event.position;
                        InvalidatePaint();
                    }

                    void OnMouseDown(const MouseEvent& event) override {
                        if (event.button != MouseButton::Left) return;

                        const float itemH = 22.0f;
                        float y = m_ListRect.y;
                        for (size_t i = 0; i < m_Assets.size(); ++i) {
                            const Rect itemRect{ m_ListRect.x, y, m_ListRect.width, itemH };
                            if (itemRect.Contains(event.position)) {
                                if (m_Handle) {
                                    (void)m_Handle->SetString(m_Assets[i].path);
                                }
                                if (m_OnClose) {
                                    m_OnClose();
                                }
                                return;
                            }
                            y += itemH + 2.0f;
                        }
                    }

                private:
                    struct AssetItem {
                        std::string name;
                        std::string path;
                        AssetKind kind;
                    };
                    PropertyHandlePtr m_Handle;
                    std::string m_CurrentPath;
                    std::function<void()> m_OnClose;
                    std::vector<AssetItem> m_Assets;
                    Point m_MousePos{};
                    Rect m_SearchRect{};
                    Rect m_ListRect{};
                };

                auto pickerPopover = std::make_shared<AssetPickerPopover>(m_Handle, m_Path, [popupHost]() {
                    if (popupHost) {
                        popupHost->CloseTopPopup();
                    }
                });
                popupHost->ShowAnchoredPopup(pickerPopover, m_PillRect,
                    we::runtime::kindui::PopupPlacementMode::BottomPreferred);
            }

            void Read() {
                m_Path.clear();
                if (!m_Handle) {
                    return;
                }
                (void)m_Handle->GetString(m_Path);
            }

            PropertyHandlePtr m_Handle;
            std::string m_Path;
            Rect m_ThumbRect{};
            Rect m_PillRect{};
            Rect m_IconRect{};
            Rect m_FieldRect{};
            Rect m_UseSelectedBtnRect{};
            Rect m_LocateBtnRect{};
            Rect m_ClearBtnRect{};
            bool m_Hovered = false;
            bool m_UseSelectedHovered = false;
            bool m_LocateHovered = false;
            bool m_ClearHovered = false;
        };

        return std::make_shared<AssetReferenceWidget>(m_Handle);
    }
};

}

class PropertyEditorFactoryImpl final : public IPropertyEditorFactory {
public:
    void RegisterByPrimitive(PrimitiveKind kind, PropertyEditorFactoryFn factory) override {
        m_ByPrimitive[static_cast<std::uint8_t>(kind)] = std::move(factory);
    }
    void RegisterByTypeKind(TypeKind kind, PropertyEditorFactoryFn factory) override {
        m_ByTypeKind[static_cast<std::uint8_t>(kind)] = std::move(factory);
    }
    void RegisterByTypeId(TypeId typeId, PropertyEditorFactoryFn factory) override {
        m_ByTypeId[typeId] = std::move(factory);
    }

    [[nodiscard]] PropertyEditorPtr CreateEditor(
        const PropertyInfo& property,
        const PropertyHandlePtr& handle) override
    {
        if (IsAssetReferenceProperty(property)) {
            auto editor = std::make_shared<AssetReferenceEditor>();
            editor->Bind(handle);
            return editor;
        }

        if (const auto it = m_ByTypeId.find(property.typeId); it != m_ByTypeId.end() && it->second) {
            auto editor = it->second();
            if (editor) {
                editor->Bind(handle);
                return editor;
            }
        }

        if (m_Registry) {
            if (const TypeInfo* info = m_Registry->Find(property.typeId)) {
                if (info->IsEnum()) {
                    auto editor = std::make_shared<EnumEditor>(m_Registry);
                    editor->Bind(handle);
                    return editor;
                }
                if (info->kind == TypeKind::Primitive || property.primitive == PrimitiveKind::None) {
                    const PrimitiveKind primitive =
                        property.primitive != PrimitiveKind::None ? property.primitive : info->primitive;

                    if (primitive == PrimitiveKind::Vec3 || primitive == PrimitiveKind::Vec4) {
                        const std::string& name = property.name;
                        if (name.find("Color") != std::string::npos ||
                            name.find("color") != std::string::npos ||
                            name == "Tint" || name == "tint" ||
                            name == "Diffuse" || name == "diffuse" ||
                            name == "Ambient" || name == "ambient" ||
                            name == "Specular" || name == "specular") {
                            auto editor = std::make_shared<ColorEditor>();
                            editor->Bind(handle);
                            return editor;
                        }
                    }

                    if (const auto pit = m_ByPrimitive.find(static_cast<std::uint8_t>(primitive));
                        pit != m_ByPrimitive.end() && pit->second) {
                        auto editor = pit->second();
                        if (editor) {
                            editor->Bind(handle);
                            return editor;
                        }
                    }
                }
                if (const auto it = m_ByTypeKind.find(static_cast<std::uint8_t>(info->kind));
                    it != m_ByTypeKind.end() && it->second) {
                    auto editor = it->second();
                    if (editor) {
                        editor->Bind(handle);
                        return editor;
                    }
                }
            }
        }

        if (const auto it = m_ByPrimitive.find(static_cast<std::uint8_t>(property.primitive));
            it != m_ByPrimitive.end() && it->second) {
            auto editor = it->second();
            if (editor) {
                editor->Bind(handle);
                return editor;
            }
        }

        auto fallback = std::make_shared<StructEditor>();
        fallback->Bind(handle);
        return fallback;
    }

    void SetRegistry(ITypeRegistry* registry) { m_Registry = registry; }

private:
    ITypeRegistry* m_Registry = nullptr;
    std::unordered_map<std::uint8_t, PropertyEditorFactoryFn> m_ByPrimitive;
    std::unordered_map<std::uint8_t, PropertyEditorFactoryFn> m_ByTypeKind;
    std::unordered_map<TypeId, PropertyEditorFactoryFn> m_ByTypeId;
};

void RegisterDefaultEditors(IPropertyEditorFactory& factory) {
    factory.RegisterByPrimitive(PrimitiveKind::Bool, [] { return std::make_shared<BoolEditor>(); });
    factory.RegisterByPrimitive(PrimitiveKind::Int8, [] {
        return std::make_shared<TextValueEditor>(TextValueEditor::Mode::Int32);
    });
    factory.RegisterByPrimitive(PrimitiveKind::Int16, [] {
        return std::make_shared<TextValueEditor>(TextValueEditor::Mode::Int32);
    });
    factory.RegisterByPrimitive(PrimitiveKind::Int32, [] {
        return std::make_shared<TextValueEditor>(TextValueEditor::Mode::Int32);
    });
    factory.RegisterByPrimitive(PrimitiveKind::UInt8, [] {
        return std::make_shared<TextValueEditor>(TextValueEditor::Mode::UInt32);
    });
    factory.RegisterByPrimitive(PrimitiveKind::UInt16, [] {
        return std::make_shared<TextValueEditor>(TextValueEditor::Mode::UInt32);
    });
    factory.RegisterByPrimitive(PrimitiveKind::UInt32, [] {
        return std::make_shared<TextValueEditor>(TextValueEditor::Mode::UInt32);
    });
    factory.RegisterByPrimitive(PrimitiveKind::Int64, [] {
        return std::make_shared<TextValueEditor>(TextValueEditor::Mode::Int64);
    });
    factory.RegisterByPrimitive(PrimitiveKind::UInt64, [] {
        return std::make_shared<TextValueEditor>(TextValueEditor::Mode::Int64);
    });
    factory.RegisterByPrimitive(PrimitiveKind::Float, [] {
        return std::make_shared<TextValueEditor>(TextValueEditor::Mode::Float);
    });
    factory.RegisterByPrimitive(PrimitiveKind::Double, [] {
        return std::make_shared<TextValueEditor>(TextValueEditor::Mode::Double);
    });
    factory.RegisterByPrimitive(PrimitiveKind::String, [] {
        return std::make_shared<TextValueEditor>(TextValueEditor::Mode::String);
    });
    factory.RegisterByPrimitive(PrimitiveKind::Char, [] {
        return std::make_shared<TextValueEditor>(TextValueEditor::Mode::String);
    });
    factory.RegisterByPrimitive(PrimitiveKind::Vec2, [] {
        return std::make_shared<VectorEditor>(VectorEditor::Mode::Vec2);
    });
    factory.RegisterByPrimitive(PrimitiveKind::Vec3, [] {
        return std::make_shared<VectorEditor>(VectorEditor::Mode::Vec3);
    });
    factory.RegisterByPrimitive(PrimitiveKind::Vec4, [] {
        return std::make_shared<VectorEditor>(VectorEditor::Mode::Vec4);
    });
    factory.RegisterByPrimitive(PrimitiveKind::Quat, [] {
        return std::make_shared<VectorEditor>(VectorEditor::Mode::Vec4);
    });

    factory.RegisterByTypeKind(TypeKind::Struct, [] { return std::make_shared<StructEditor>(); });
    factory.RegisterByTypeKind(TypeKind::Class, [] { return std::make_shared<StructEditor>(); });
    factory.RegisterByTypeKind(TypeKind::Array, [] { return std::make_shared<ArrayStubEditor>(); });
    factory.RegisterByTypeKind(TypeKind::Map, [] {
        return std::make_shared<MapSetStubEditor>("[Map — stub]");
    });
}

std::unique_ptr<IPropertyEditorFactory> CreateEditorFactory(ITypeRegistry* registry, bool registerDefaults) {
    auto factory = std::make_unique<PropertyEditorFactoryImpl>();
    factory->SetRegistry(registry);
    factory->RegisterByTypeKind(TypeKind::Enum, [registry] {
        return std::make_shared<EnumEditor>(registry);
    });
    if (registerDefaults) {
        RegisterDefaultEditors(*factory);
    }
    return factory;
}

}
}
