#include "Widgets/PropertyEditor.h"
#include "Core/PaintContext.h"
#include "Core/Theme.h"
#include "Core/Icon.h"
#include <cmath>
#include <algorithm>

namespace we::UI {

PropertyEditor::PropertyEditor()
    : m_Style(WidgetStyle::Panel())
{}

Size PropertyEditor::Measure(const Size& availableSize) {
    BuildCategories();
    CalculateGeometries();
    
    float totalHeight = 0.0f;
    for (const auto& cat : m_Categories) {
        totalHeight += m_CategoryHeaderHeight;
        if (cat.expanded) {
            totalHeight += cat.contentHeight;
        }
    }
    
    return Size{ availableSize.width, totalHeight };
}

void PropertyEditor::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    CalculateGeometries();
}

void PropertyEditor::Paint(PaintContext& context) {
    // Draw background
    context.DrawRect(m_Geometry, Theme::Get().ContentBrowserBackground); // #25272B
    
    float y = m_Geometry.y - m_ScrollOffset;
    
    for (const auto& cat : m_Categories) {
        // Skip if entirely outside visible area
        if (y + m_CategoryHeaderHeight < m_Geometry.y || y > m_Geometry.y + m_Geometry.height) {
            y += m_CategoryHeaderHeight;
            if (cat.expanded) y += cat.contentHeight;
            continue;
        }
        
        // Draw category header
        Rect headerRect{ m_Geometry.x, y, m_Geometry.width, m_CategoryHeaderHeight };
        
        Color headerBg = Theme::Get().BorderDark; // #2E3136
        if (cat.name == m_HoveredCategory) {
            headerBg = Theme::Get().HoverOverlay;
        }
        
        context.DrawRoundedRect(headerRect, headerBg, 4.0f);
        
        // Draw 1px separator at bottom of header
        context.DrawRect({ m_Geometry.x, y + m_CategoryHeaderHeight - 1.0f, m_Geometry.width, 1.0f }, Theme::Get().Separator);
        
        // Draw expand/collapse chevron
        float chevronSize = 12.0f;
        float chevronX = headerRect.x + 8.0f;
        float chevronY = headerRect.y + (m_CategoryHeaderHeight - chevronSize) / 2.0f;
        
        Rect chevronRect{ chevronX, chevronY, chevronSize, chevronSize };
        const char* chevronIcon = cat.expanded ? Icons::ChevronDownName : Icons::ChevronRightName;
        IconPainter::DrawIcon(context, chevronIcon, chevronRect, Theme::Get().TextSecondary);
        
        // Draw category name
        float textX = chevronX + chevronSize + 8.0f;
        float textY = headerRect.y + (m_CategoryHeaderHeight - Theme::Get().TextSizeHeader) / 2.0f;
        context.DrawText(cat.name, Point{ textX, textY }, Theme::Get().TextPrimary, Theme::Get().TextSizeHeader, true);
        
        y += m_CategoryHeaderHeight;
        
        // Draw properties in category
        if (cat.expanded) {
            for (size_t propIdx : cat.propertyIndices) {
                const auto& prop = m_Properties[propIdx];
                
                Rect propRect{ m_Geometry.x + 8.0f, y, m_Geometry.width - 16.0f, m_PropertyHeight };
                
                // Skip if outside visible area
                if (propRect.y + propRect.height < m_Geometry.y || propRect.y > m_Geometry.y + m_Geometry.height) {
                    y += m_PropertyHeight;
                    continue;
                }
                
                // Draw hover background
                if (prop.name == m_HoveredProperty) {
                    context.DrawRoundedRect(propRect, Theme::Get().HoverOverlay, 2.0f);
                }
                
                // Draw property label
                float labelX = propRect.x + 4.0f;
                float labelY = propRect.y + (m_PropertyHeight - Theme::Get().TextSizeBody) / 2.0f;
                Color labelColor = prop.hasOverride ? Theme::Get().SelectedAccent : Theme::Get().TextSecondary;
                context.DrawText(prop.name, Point{ labelX, labelY }, labelColor, Theme::Get().TextSizeBody);
                
                // Draw property value as a rounded input box
                float valueWidth = propRect.width - m_LabelWidth - 8.0f;
                Rect valueRect{ propRect.x + m_LabelWidth, propRect.y + (m_PropertyHeight - 20.0f) / 2.0f, valueWidth, 20.0f };
                
                context.DrawRoundedRect(valueRect, Theme::Get().InputBackground, 4.0f); // Input box background
                context.DrawRoundedRectOutline(valueRect, Theme::Get().BorderDefault, 1.0f, 4.0f); // Input box border
                
                float valueX = valueRect.x + 6.0f;
                float valueY = valueRect.y + (20.0f - 13.0f) / 2.0f;
                context.DrawText(prop.value, Point{ valueX, valueY }, Theme::Get().TextPrimary, 13.0f);
                
                // Draw reset button if has override
                if (prop.hasOverride) {
                    float resetSize = 16.0f;
                    float resetX = propRect.x + propRect.width - resetSize - 4.0f;
                    float resetY = propRect.y + (m_PropertyHeight - resetSize) / 2.0f;
                    
                    Rect resetRect{ resetX, resetY, resetSize, resetSize };
                    IconPainter::DrawIcon(context, Icons::XName, resetRect, Theme::Get().TextSecondary);
                }
                
                y += m_PropertyHeight;
            }
        }
    }
}

void PropertyEditor::OnMouseDown(const MouseEvent& event) {
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

void PropertyEditor::OnMouseWheel(const MouseEvent& event) {
    float scrollAmount = event.wheelDeltaY * m_PropertyHeight * 0.5f;
    m_ScrollOffset -= scrollAmount;
    
    // Calculate total height
    float totalHeight = 0.0f;
    for (const auto& cat : m_Categories) {
        totalHeight += m_CategoryHeaderHeight;
        if (cat.expanded) {
            totalHeight += cat.contentHeight;
        }
    }
    
    float maxScroll = std::max(0.0f, totalHeight - m_Geometry.height);
    m_ScrollOffset = std::max(0.0f, std::min(m_ScrollOffset, maxScroll));
    
    CalculateGeometries();
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
    m_ScrollOffset = 0.0f;
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
        cat.headerGeometry = Rect{ m_Geometry.x, y, m_Geometry.width, m_CategoryHeaderHeight };
        y += m_CategoryHeaderHeight;
        
        cat.contentHeight = 0.0f;
        if (cat.expanded) {
            for (size_t propIdx : cat.propertyIndices) {
                cat.contentHeight += m_PropertyHeight;
            }
            y += cat.contentHeight;
        }
    }
}

Property* PropertyEditor::GetPropertyAtPosition(const Point& pos) {
    float y = -m_ScrollOffset;
    
    for (const auto& cat : m_Categories) {
        y += m_CategoryHeaderHeight;
        
        if (cat.expanded) {
            for (size_t propIdx : cat.propertyIndices) {
                Rect propRect{ m_Geometry.x + 8.0f, y, m_Geometry.width - 16.0f, m_PropertyHeight };
                
                if (pos.x >= propRect.x && pos.x <= propRect.x + propRect.width &&
                    pos.y >= propRect.y && pos.y <= propRect.y + propRect.height) {
                    return &m_Properties[propIdx];
                }
                
                y += m_PropertyHeight;
            }
        }
    }
    
    return nullptr;
}

PropertyEditor::CategoryGroup* PropertyEditor::GetCategoryAtPosition(const Point& pos) {
    float y = -m_ScrollOffset;
    
    for (auto& cat : m_Categories) {
        Rect headerRect{ m_Geometry.x + 4.0f, y, m_Geometry.width - 8.0f, m_CategoryHeaderHeight };
        
        if (pos.x >= headerRect.x && pos.x <= headerRect.x + headerRect.width &&
            pos.y >= headerRect.y && pos.y <= headerRect.y + headerRect.height) {
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
    
    Color bgColor = Theme::Get().ToolbarBackground;
    Color borderColor = Theme::Get().BorderDefault;
    
    if (*m_Value) {
        bgColor = Theme::Get().SelectedAccent;
        borderColor = Theme::Get().SelectedAccent;
    }
    
    context.DrawRoundedRect(checkRect, bgColor, 3.0f);
    context.DrawRoundedRectOutline(checkRect, borderColor, 1.0f, 3.0f);
    
    // Draw checkmark if checked
    if (*m_Value) {
        Rect iconRect{ checkX + 2.0f, checkY + 2.0f, checkSize - 4.0f, checkSize - 4.0f };
        IconPainter::DrawIcon(context, Icons::CheckName, iconRect, Color{1, 1, 1, 1});
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
    context.DrawRoundedRectOutline(colorRect, Theme::Get().BorderDefault, 1.0f, 3.0f);
    
    // Draw hex value
    char hex[16];
    snprintf(hex, sizeof(hex), "#%02X%02X%02X",
        static_cast<int>(m_Value->r * 255),
        static_cast<int>(m_Value->g * 255),
        static_cast<int>(m_Value->b * 255));
    
    float textX = colorX + colorSize + 8.0f;
    float textY = m_Geometry.y + (m_Geometry.height - 12.0f) / 2.0f;
    context.DrawText(hex, Point{ textX, textY }, Theme::Get().TextPrimary, 12.0f);
}

void ColorPropertyWidget::OnMouseDown(const MouseEvent& event) {
    // TODO: Implement color picker
}

} // namespace we::editor::propertyeditor::UI
