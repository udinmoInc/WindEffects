#include "PropertyEditor/Compat/LegacyPropertyEditor.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/Icon.h"
#include <cmath>
#include <algorithm>

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;

namespace we::editor::property {
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::IconPainter;
namespace PanelChrome = ::we::editor::panels::PanelChrome;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;
namespace Icons = ::we::runtime::kindui::Icons;


PropertyEditor::PropertyEditor()
    : m_Style(WidgetStyle::Panel())
{}

void PropertyEditor::SyncScrollMetrics() {
    m_Scroll.Sync(m_Geometry.height, m_ContentHeight);
    const float uiScale = std::max(1.0f, DPIContext::GetScale());
    m_ScrollMetrics = m_Scroll.ComputeMetrics(m_Geometry, m_ContentHeight, uiScale);
}

Size PropertyEditor::Measure(const Size& availableSize) {
    BuildCategories();
    CalculateGeometries();
    return Size{ availableSize.width, availableSize.height };
}

void PropertyEditor::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    CalculateGeometries();
    SyncScrollMetrics();
}

void PropertyEditor::Paint(PaintContext& context) {
    PanelChrome::PaintContentRegion(context, m_Geometry);
    SyncScrollMetrics();

    const float uiScale = std::max(1.0f, DPIContext::GetScale());
    const float viewTop = m_ScrollMetrics.viewport.y;
    const float viewBottom = m_ScrollMetrics.viewport.y + m_ScrollMetrics.viewport.height;
    float y = m_ScrollMetrics.viewport.y - m_Scroll.offset;

    context.PushClipRect(m_ScrollMetrics.viewport);

    for (const auto& cat : m_Categories) {
        if (y + m_CategoryHeaderHeight < viewTop || y > viewBottom) {
            y += m_CategoryHeaderHeight;
            if (cat.expanded) {
                y += cat.contentHeight;
            }
            continue;
        }

        Rect headerRect{
            m_ScrollMetrics.viewport.x,
            y,
            m_ScrollMetrics.viewport.width,
            m_CategoryHeaderHeight
        };
        PanelChrome::PaintCategoryHeader(context, headerRect, cat.name, cat.expanded, cat.name == m_HoveredCategory);

        y += m_CategoryHeaderHeight;

        if (cat.expanded) {
            for (size_t propIdx : cat.propertyIndices) {
                const auto& prop = m_Properties[propIdx];

                Rect propRect{
                    m_ScrollMetrics.viewport.x,
                    y,
                    m_ScrollMetrics.viewport.width,
                    m_PropertyHeight
                };

                if (propRect.y + propRect.height < viewTop || propRect.y > viewBottom) {
                    y += m_PropertyHeight;
                    continue;
                }

                if (prop.name == m_HoveredProperty) {
                    PanelChrome::PaintListRowBackground(context, propRect, true, false);
                }

                const float labelX = propRect.x + PanelChrome::PanelPaddingH();
                const float labelY = propRect.y + (m_PropertyHeight - ThemeMetric(MetricToken::TextSizeBody)) * 0.5f;
                Color labelColor = prop.hasOverride ? ThemeColor(ColorToken::AccentPrimary) : ThemeColor(ColorToken::TextSecondary);
                context.DrawText(prop.name, Point{ labelX, labelY }, labelColor, ThemeMetric(MetricToken::TextSizeBody));

                float valueWidth = propRect.width - m_LabelWidth - PanelChrome::PanelPaddingH() * 2.0f;
                const float inputHeight = ThemeMetric(MetricToken::SearchBoxHeight) - ThemeMetric(MetricToken::Space2);
                Rect valueRect{
                    propRect.x + m_LabelWidth,
                    propRect.y + (m_PropertyHeight - inputHeight) * 0.5f,
                    valueWidth,
                    inputHeight
                };

                context.DrawRoundedRect(valueRect, ThemeColor(ColorToken::InputBackground), ThemeMetric(MetricToken::CornerRadiusSmall));

                const float valueX = valueRect.x + ThemeMetric(MetricToken::Space2);
                const float valueY = valueRect.y + (inputHeight - ThemeMetric(MetricToken::TextSizeBody)) * 0.5f;
                context.DrawText(prop.value, Point{ valueX, valueY }, ThemeColor(ColorToken::TextPrimary), ThemeMetric(MetricToken::TextSizeBody));

                if (prop.hasOverride) {
                    const float resetSize = static_cast<float>(IconMetrics::NativeIconTierPx(ThemeMetric(MetricToken::IconSizeTree)));
                    const float resetX = propRect.x + propRect.width - resetSize - PanelChrome::PanelPaddingH();
                    const float resetY = propRect.y + (m_PropertyHeight - resetSize) * 0.5f;
                    IconPainter::DrawIcon(context, Icons::XName, Rect{ resetX, resetY, resetSize, resetSize }, ThemeColor(ColorToken::TextSecondary));
                }

                y += m_PropertyHeight;
            }
        }
    }

    context.PopClipRect();
    m_Scroll.Paint(context, m_ScrollMetrics, m_Scroll.IsThumbHovered());
}

void PropertyEditor::OnMouseDown(const MouseEvent& event) {
    SyncScrollMetrics();
    if (m_Scroll.OnMouseDown(event, m_ScrollMetrics, m_Geometry.height, m_ContentHeight)) {
        Arrange(m_Geometry);
        return;
    }

    CategoryGroup* clickedCategory = GetCategoryAtPosition(event.position);
    if (clickedCategory) {
        // Check if clicked on chevron area
        float chevronX = m_Geometry.x + 4.0f + 8.0f;
        float chevronSize = 12.0f;
        Rect chevronRect{ chevronX, clickedCategory->headerGeometry.y + (m_CategoryHeaderHeight - chevronSize) / 2.0f, chevronSize, chevronSize };
        
        if (event.position.x >= chevronRect.x && event.position.x <= chevronRect.x + chevronRect.width &&
            event.position.y >= chevronRect.y && event.position.y <= chevronRect.y + chevronRect.height) {
            clickedCategory->expanded = !clickedCategory->expanded;
            CalculateGeometries();
            return;
        }
        
        clickedCategory->expanded = !clickedCategory->expanded;
        CalculateGeometries();
        return;
    }
    
    Property* clickedProperty = GetPropertyAtPosition(event.position);
    if (clickedProperty) {
        // Check if clicked on reset button
        if (clickedProperty->hasOverride) {
            float resetSize = 16.0f;
            float propX = m_Geometry.x + 8.0f;
            float propWidth = m_Geometry.width - 16.0f;
            float resetX = propX + propWidth - resetSize - 4.0f;
            
            if (event.position.x >= resetX && event.position.x <= resetX + resetSize) {
                clickedProperty->value = clickedProperty->defaultValue;
                clickedProperty->hasOverride = false;
                if (clickedProperty->onReset) {
                    clickedProperty->onReset();
                }
                return;
            }
        }
        
        // TODO: Handle property value editing
        if (clickedProperty->type == PropertyType::Bool) {
            const bool current = clickedProperty->value == "true" || clickedProperty->value == "1";
            const std::string next = current ? "false" : "true";
            SetPropertyValue(clickedProperty->name, next);
            return;
        }
    }
}

void PropertyEditor::OnMouseMove(const MouseEvent& event) {
    SyncScrollMetrics();
    m_Scroll.OnMouseMove(event, m_ScrollMetrics, m_Geometry.height, m_ContentHeight);
    if (m_Scroll.IsDraggingThumb()) {
        Arrange(m_Geometry);
        return;
    }

    m_HoveredCategory.clear();
    m_HoveredProperty.clear();
    
    CategoryGroup* hoveredCategory = GetCategoryAtPosition(event.position);
    if (hoveredCategory) {
        m_HoveredCategory = hoveredCategory->name;
        return;
    }
    
    Property* hoveredProperty = GetPropertyAtPosition(event.position);
    if (hoveredProperty) {
        m_HoveredProperty = hoveredProperty->name;
    }
}

void PropertyEditor::OnMouseUp(const MouseEvent& event) {
    m_Scroll.OnMouseUp(event);
}

void PropertyEditor::OnMouseWheel(const MouseEvent& event) {
    SyncScrollMetrics();
    m_Scroll.ApplyWheel(
        event.wheelDeltaY,
        m_PropertyHeight * 0.5f,
        m_Geometry.height,
        m_ContentHeight);
    CalculateGeometries();
    SyncScrollMetrics();
}

bool PropertyEditor::ShowsPointerCursor(const Point& position) const {
    return m_ScrollMetrics.showsScrollbar &&
        (m_ScrollMetrics.thumb.Contains(position) || m_ScrollMetrics.track.Contains(position));
}

void PropertyEditor::AddProperty(const Property& property) {
    m_Properties.push_back(property);
    BuildCategories();
    CalculateGeometries();
}

void PropertyEditor::RemoveProperty(const std::string& name) {
    m_Properties.erase(
        std::remove_if(m_Properties.begin(), m_Properties.end(),
            [&name](const Property& p) { return p.name == name; }),
        m_Properties.end()
    );
    BuildCategories();
    CalculateGeometries();
}

void PropertyEditor::Clear() {
    m_Properties.clear();
    m_Categories.clear();
    m_Scroll.offset = 0.0f;
    m_ContentHeight = 0.0f;
}

std::string PropertyEditor::GetPropertyValue(const std::string& name) const {
    for (const auto& prop : m_Properties) {
        if (prop.name == name) {
            return prop.value;
        }
    }
    return "";
}

void PropertyEditor::SetPropertyValue(const std::string& name, const std::string& value) {
    for (auto& prop : m_Properties) {
        if (prop.name == name) {
            prop.value = value;
            prop.hasOverride = (value != prop.defaultValue);
            if (prop.onValueChanged) {
                prop.onValueChanged(value);
            }
            return;
        }
    }
}

void PropertyEditor::SetCategoryExpanded(const std::string& category, bool expanded) {
    for (auto& cat : m_Categories) {
        if (cat.name == category) {
            cat.expanded = expanded;
            CalculateGeometries();
            return;
        }
    }
}

void PropertyEditor::BuildCategories() {
    m_Categories.clear();
    
    // Group properties by category
    std::unordered_map<std::string, CategoryGroup> categoryMap;
    
    for (size_t i = 0; i < m_Properties.size(); ++i) {
        const auto& prop = m_Properties[i];
        std::string category = prop.category.empty() ? "General" : prop.category;
        
        if (categoryMap.find(category) == categoryMap.end()) {
            CategoryGroup cat;
            cat.name = category;
            categoryMap[category] = cat;
        }
        
        categoryMap[category].propertyIndices.push_back(i);
    }
    
    // Convert to vector
    for (auto& pair : categoryMap) {
        m_Categories.push_back(pair.second);
    }
}

void PropertyEditor::CalculateGeometries() {
    float y = 0.0f;

    for (auto& cat : m_Categories) {
        cat.headerGeometry = Rect{ 0.0f, y, m_Geometry.width, m_CategoryHeaderHeight };
        y += m_CategoryHeaderHeight;

        cat.contentHeight = 0.0f;
        if (cat.expanded) {
            for (size_t propIdx : cat.propertyIndices) {
                (void)propIdx;
                cat.contentHeight += m_PropertyHeight;
            }
            y += cat.contentHeight;
        }
    }

    m_ContentHeight = y;
}

Property* PropertyEditor::GetPropertyAtPosition(const Point& pos) {
    SyncScrollMetrics();
    float y = m_ScrollMetrics.viewport.y - m_Scroll.offset;

    for (const auto& cat : m_Categories) {
        y += m_CategoryHeaderHeight;

        if (cat.expanded) {
            for (size_t propIdx : cat.propertyIndices) {
                Rect propRect{
                    m_ScrollMetrics.viewport.x + ThemeMetric(MetricToken::Space2),
                    y,
                    m_ScrollMetrics.viewport.width - ThemeMetric(MetricToken::Space4),
                    m_PropertyHeight
                };

                if (propRect.Contains(pos)) {
                    return &m_Properties[propIdx];
                }

                y += m_PropertyHeight;
            }
        }
    }

    return nullptr;
}

PropertyEditor::CategoryGroup* PropertyEditor::GetCategoryAtPosition(const Point& pos) {
    SyncScrollMetrics();
    float y = m_ScrollMetrics.viewport.y - m_Scroll.offset;

    for (auto& cat : m_Categories) {
        Rect headerRect{
            m_ScrollMetrics.viewport.x + 4.0f,
            y,
            m_ScrollMetrics.viewport.width - 8.0f,
            m_CategoryHeaderHeight
        };

        if (headerRect.Contains(pos)) {
            return &cat;
        }

        y += m_CategoryHeaderHeight;
        if (cat.expanded) {
            y += cat.contentHeight;
        }
    }

    return nullptr;
}

// BoolPropertyWidget implementation
BoolPropertyWidget::BoolPropertyWidget(bool* value, std::function<void(bool)> onChanged)
    : m_Value(value)
    , m_OnChanged(onChanged)
{}

Size BoolPropertyWidget::Measure(const Size& availableSize) {
    return Size{ 40.0f, 20.0f };
}

void BoolPropertyWidget::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void BoolPropertyWidget::Paint(PaintContext& context) {
    // Draw checkbox
    float checkSize = 16.0f;
    float checkX = m_Geometry.x + (m_Geometry.width - checkSize) / 2.0f;
    float checkY = m_Geometry.y + (m_Geometry.height - checkSize) / 2.0f;
    
    Rect checkRect{ checkX, checkY, checkSize, checkSize };
    
    Color bgColor = ThemeColor(ColorToken::InputBackground);
    Color borderColor = ThemeColor(ColorToken::BorderDefault);
    
    if (*m_Value) {
        bgColor = ThemeColor(ColorToken::AccentPrimary);
        borderColor = ThemeColor(ColorToken::AccentPrimary);
    }
    
    context.DrawRoundedRect(checkRect, bgColor, 3.0f);
    context.DrawRoundedRectOutline(checkRect, borderColor, 1.0f, 3.0f);
    
    // Draw checkmark if checked
    if (*m_Value) {
        Rect iconRect{ checkX + 2.0f, checkY + 2.0f, checkSize - 4.0f, checkSize - 4.0f };
        IconPainter::DrawIcon(context, Icons::CheckName, iconRect, ThemeColor(ColorToken::TextPrimary));
    }
}

void BoolPropertyWidget::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        *m_Value = !*m_Value;
        if (m_OnChanged) {
            m_OnChanged(*m_Value);
        }
    }
}

// ColorPropertyWidget implementation
ColorPropertyWidget::ColorPropertyWidget(Color* value, std::function<void(const Color&)> onChanged)
    : m_Value(value)
    , m_OnChanged(onChanged)
{}

Size ColorPropertyWidget::Measure(const Size& availableSize) {
    return Size{ 60.0f, 20.0f };
}

void ColorPropertyWidget::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void ColorPropertyWidget::Paint(PaintContext& context) {
    // Draw color preview box
    float colorSize = 20.0f;
    float colorX = m_Geometry.x;
    float colorY = m_Geometry.y + (m_Geometry.height - colorSize) / 2.0f;
    
    Rect colorRect{ colorX, colorY, colorSize, colorSize };
    context.DrawRoundedRect(colorRect, *m_Value, 3.0f);
    context.DrawRoundedRectOutline(colorRect, ThemeColor(ColorToken::BorderDefault), 1.0f, 3.0f);
    
    // Draw hex value
    char hex[16];
    snprintf(hex, sizeof(hex), "#%02X%02X%02X",
        static_cast<int>(m_Value->r * 255),
        static_cast<int>(m_Value->g * 255),
        static_cast<int>(m_Value->b * 255));
    
    float textX = colorX + colorSize + 8.0f;
    float textY = m_Geometry.y + (m_Geometry.height - 12.0f) / 2.0f;
    context.DrawText(hex, Point{ textX, textY }, ThemeColor(ColorToken::TextPrimary), 12.0f);
}

void ColorPropertyWidget::OnMouseDown(const MouseEvent& event) {
    // TODO: Implement color picker
}

} // namespace we::editor::propertyeditor
