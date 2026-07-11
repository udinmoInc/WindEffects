#pragma once

#include "PropertyEditor/Export.h"

#include "Core/Widget.h"
#include "Core/Style.h"
#include <string>
#include <functional>
#include <vector>

namespace WindEffects::Editor::UI {

// Property types
enum class PropertyType {
    Bool,
    Int,
    Float,
    String,
    Color,
    Vector2,
    Vector3,
    Enum,
    Button
};

// Property data structure
struct Property {
    std::string name;
    std::string category;
    PropertyType type;
    
    // Value storage (as string for simplicity)
    std::string value;
    std::string defaultValue;
    
    // For enum types
    std::vector<std::string> enumOptions;
    
    // Callbacks
    std::function<void(const std::string&)> onValueChanged;
    std::function<void()> onReset;
    
    // UI state
    bool expanded = true;
    bool hasOverride = false;
};

// Property editor widget for inspector panels
class PROPERTYEDITOR_API PropertyEditor : public Widget {
public:
    PropertyEditor();
    virtual ~PropertyEditor() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseWheel(const MouseEvent& event) override;

    // Property management
    void AddProperty(const Property& property);
    void RemoveProperty(const std::string& name);
    void Clear();
    
    // Get/set property values
    std::string GetPropertyValue(const std::string& name) const;
    void SetPropertyValue(const std::string& name, const std::string& value);
    
    // Categories
    void SetCategoryExpanded(const std::string& category, bool expanded);
    
    // Styling
    void SetPropertyHeight(float height) { m_PropertyHeight = height; }
    void SetLabelWidth(float width) { m_LabelWidth = width; }

private:
    struct CategoryGroup {
        std::string name;
        std::vector<size_t> propertyIndices;
        bool expanded = true;
        Rect headerGeometry;
        float contentHeight = 0.0f;
    };

    void BuildCategories();
    void CalculateGeometries();
    Property* GetPropertyAtPosition(const Point& pos);
    CategoryGroup* GetCategoryAtPosition(const Point& pos);

    std::vector<Property> m_Properties;
    std::vector<CategoryGroup> m_Categories;
    
    float m_ScrollOffset = 0.0f;
    float m_PropertyHeight = 22.0f;
    float m_LabelWidth = 120.0f;
    float m_CategoryHeaderHeight = 22.0f;
    
    std::string m_HoveredProperty;
    std::string m_HoveredCategory;

    WidgetStyle m_Style;
};

// Helper widgets for property editing
class BoolPropertyWidget : public Widget {
public:
    BoolPropertyWidget(bool* value, std::function<void(bool)> onChanged = nullptr);
    virtual ~BoolPropertyWidget() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseDown(const MouseEvent& event) override;

private:
    bool* m_Value;
    std::function<void(bool)> m_OnChanged;
};

class ColorPropertyWidget : public Widget {
public:
    ColorPropertyWidget(Color* value, std::function<void(const Color&)> onChanged = nullptr);
    virtual ~ColorPropertyWidget() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseDown(const MouseEvent& event) override;

private:
    Color* m_Value;
    std::function<void(const Color&)> m_OnChanged;
    bool m_Picking = false;
};

} // namespace we::editor::propertyeditor::UI
