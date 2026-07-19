#include "PropertyEditorInternal.h"
#include "PropertyEditor/IPropertyEditor.h"
#include "PropertyEditor/IPropertyEditorFactory.h"

#include "KindUI/Core/Icon.h"
#include "KindUI/Widgets/TextBox.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unordered_map>

namespace we::editor::property {
namespace detail {
namespace {

using we::runtime::kindui::Color;
using we::runtime::kindui::ColorToken;
using we::runtime::kindui::MetricToken;
using we::runtime::kindui::MouseButton;
using we::runtime::kindui::MouseEvent;
using we::runtime::kindui::PaintContext;
using we::runtime::kindui::Point;
using we::runtime::kindui::Rect;
using we::runtime::kindui::Size;
using we::runtime::kindui::TextBox;
using we::runtime::kindui::Widget;
namespace Icons = we::runtime::kindui::Icons;
using we::runtime::kindui::IconPainter;

class BoundEditorBase : public IPropertyEditor {
public:
    void Bind(const PropertyHandlePtr& handle) override { m_Handle = handle; }

protected:
    PropertyHandlePtr m_Handle;
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
            Size Measure(const Size&) override { return Size{40.f, 20.f}; }
            void Arrange(const Rect& r) override { m_Geometry = r; }
            void Paint(PaintContext& context) override {
                const float checkSize = 16.f;
                Rect checkRect{
                    m_Geometry.x + (m_Geometry.width - checkSize) * 0.5f,
                    m_Geometry.y + (m_Geometry.height - checkSize) * 0.5f,
                    checkSize,
                    checkSize};
                Color bg = m_Value ? ThemeColor(ColorToken::AccentPrimary) : ThemeColor(ColorToken::InputBackground);
                context.DrawRoundedRect(checkRect, bg, 3.f);
                context.DrawRoundedRectOutline(checkRect, ThemeColor(ColorToken::BorderDefault), 1.f, 3.f);
                if (m_Value) {
                    IconPainter::DrawIcon(
                        context,
                        Icons::CheckName,
                        Rect{checkRect.x + 2.f, checkRect.y + 2.f, checkSize - 4.f, checkSize - 4.f},
                        ThemeColor(ColorToken::TextPrimary));
                }
                if (m_Handle && m_Handle->GetValueState() == PropertyValueState::Mixed) {
                    context.DrawText(
                        "—",
                        Point{m_Geometry.x + 4.f, m_Geometry.y + 2.f},
                        ThemeColor(ColorToken::TextSecondary),
                        ThemeMetric(MetricToken::TextSizeBody));
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
                return Size{available.width, 20.f};
            }
            void Arrange(const Rect& r) override { m_Geometry = r; }
            void Paint(PaintContext& context) override {
                context.DrawRoundedRect(m_Geometry, ThemeColor(ColorToken::InputBackground), 3.f);
                context.DrawRoundedRectOutline(m_Geometry, ThemeColor(ColorToken::BorderDefault), 1.f, 3.f);
                const std::string label =
                    m_Handle && m_Handle->GetValueState() == PropertyValueState::Mixed ? "—" : m_CurrentName;
                context.DrawText(
                    label,
                    Point{m_Geometry.x + 6.f, m_Geometry.y + 2.f},
                    ThemeColor(ColorToken::TextPrimary),
                    ThemeMetric(MetricToken::TextSizeBody));
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
    enum class Mode { Vec2, Vec3, Vec4, Color };

    explicit VectorEditor(Mode mode) : m_Mode(mode) {}

    [[nodiscard]] bool CanEdit(const PropertyInfo& property) const override {
        switch (m_Mode) {
        case Mode::Vec2:
            return property.primitive == PrimitiveKind::Vec2;
        case Mode::Vec3:
            return property.primitive == PrimitiveKind::Vec3;
        case Mode::Vec4:
            return property.primitive == PrimitiveKind::Vec4 || property.primitive == PrimitiveKind::Quat;
        case Mode::Color:
            return property.primitive == PrimitiveKind::Vec4; // color often stored as float4
        }
        return false;
    }

    [[nodiscard]] std::shared_ptr<Widget> CreateWidget() override {
        class VecWidget final : public Widget {
        public:
            VecWidget(PropertyHandlePtr handle, Mode mode) : m_Handle(std::move(handle)), m_Mode(mode) {
                Read();
            }
            Size Measure(const Size& available) override { return Size{available.width, 20.f}; }
            void Arrange(const Rect& r) override { m_Geometry = r; }
            void Paint(PaintContext& context) override {
                if (m_Mode == Mode::Color) {
                    Color c{m_Values[0], m_Values[1], m_Values[2], m_Values[3]};
                    Rect swatch{m_Geometry.x, m_Geometry.y + 2.f, 16.f, 16.f};
                    context.DrawRoundedRect(swatch, c, 3.f);
                    context.DrawRoundedRectOutline(swatch, ThemeColor(ColorToken::BorderDefault), 1.f, 3.f);
                }
                char buf[128]{};
                if (m_Handle && m_Handle->GetValueState() == PropertyValueState::Mixed) {
                    std::snprintf(buf, sizeof(buf), "—");
                } else if (m_Mode == Mode::Vec2) {
                    std::snprintf(buf, sizeof(buf), "%.3g, %.3g", m_Values[0], m_Values[1]);
                } else if (m_Mode == Mode::Vec3) {
                    std::snprintf(buf, sizeof(buf), "%.3g, %.3g, %.3g", m_Values[0], m_Values[1], m_Values[2]);
                } else {
                    std::snprintf(
                        buf,
                        sizeof(buf),
                        "%.3g, %.3g, %.3g, %.3g",
                        m_Values[0],
                        m_Values[1],
                        m_Values[2],
                        m_Values[3]);
                }
                const float textX = m_Mode == Mode::Color ? m_Geometry.x + 22.f : m_Geometry.x + 4.f;
                context.DrawText(
                    buf,
                    Point{textX, m_Geometry.y + 2.f},
                    ThemeColor(ColorToken::TextPrimary),
                    ThemeMetric(MetricToken::TextSizeBody));
            }

        private:
            void Read() {
                std::memset(m_Values, 0, sizeof(m_Values));
                if (!m_Handle) {
                    return;
                }
                const std::size_t n =
                    m_Mode == Mode::Vec2 ? 2u : (m_Mode == Mode::Vec3 ? 3u : 4u);
                (void)m_Handle->GetRaw(m_Values, sizeof(float) * n);
            }
            PropertyHandlePtr m_Handle;
            Mode m_Mode;
            float m_Values[4]{};
        };
        return std::make_shared<VecWidget>(m_Handle, m_Mode);
    }

private:
    Mode m_Mode;
};

// --- Struct (label only; children rendered by tree) ---
class StructEditor final : public BoundEditorBase {
public:
    [[nodiscard]] bool CanEdit(const PropertyInfo&) const override { return true; }
    [[nodiscard]] std::shared_ptr<Widget> CreateWidget() override {
        class StructWidget final : public Widget {
        public:
            Size Measure(const Size& available) override { return Size{available.width, 18.f}; }
            void Arrange(const Rect& r) override { m_Geometry = r; }
            void Paint(PaintContext& context) override {
                context.DrawText(
                    "{...}",
                    Point{m_Geometry.x + 4.f, m_Geometry.y + 2.f},
                    ThemeColor(ColorToken::TextSecondary),
                    ThemeMetric(MetricToken::TextSizeBody));
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
            Size Measure(const Size& available) override { return Size{available.width, 18.f}; }
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
            Size Measure(const Size& available) override { return Size{available.width, 18.f}; }
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

class AssetStubEditor final : public BoundEditorBase {
public:
    [[nodiscard]] bool CanEdit(const PropertyInfo& property) const override {
        return HasFlag(property.flags, PropertyFlags::AssetReference) ||
               HasFlag(property.flags, PropertyFlags::EntityReference);
    }
    [[nodiscard]] std::shared_ptr<Widget> CreateWidget() override {
        class Stub final : public Widget {
        public:
            explicit Stub(PropertyHandlePtr handle) : m_Handle(std::move(handle)) {}
            Size Measure(const Size& available) override { return Size{available.width, 18.f}; }
            void Arrange(const Rect& r) override { m_Geometry = r; }
            void Paint(PaintContext& context) override {
                const char* label = "Asset picker — stub";
                if (m_Handle && m_Handle->GetPropertyInfo() &&
                    HasFlag(m_Handle->GetPropertyInfo()->flags, PropertyFlags::EntityReference)) {
                    label = "Entity picker — stub";
                }
                context.DrawText(
                    label,
                    Point{m_Geometry.x + 4.f, m_Geometry.y + 2.f},
                    ThemeColor(ColorToken::TextSecondary),
                    ThemeMetric(MetricToken::TextSizeBody));
            }
            PropertyHandlePtr m_Handle;
        };
        return std::make_shared<Stub>(m_Handle);
    }
};

} // namespace

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
        if (HasFlag(property.flags, PropertyFlags::AssetReference) ||
            HasFlag(property.flags, PropertyFlags::EntityReference)) {
            auto editor = std::make_shared<AssetStubEditor>();
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

} // namespace detail
} // namespace we::editor::property
