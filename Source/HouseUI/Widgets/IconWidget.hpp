#pragma once

#include "../Core/Widget.hpp"
#include "../Core/Icon.hpp"
#include "../Core/Theme.hpp"
#include <string>

namespace HouseEngine::UI {

class IconWidget : public Widget {
public:
    IconWidget(const std::string& iconName, float size = 20.0f) : m_IconName(iconName), m_Size(size) {}
    virtual ~IconWidget() = default;

    Size Measure(const Size& availableSize) override {
        m_DesiredSize = Size{ m_Size, m_Size };
        return m_DesiredSize;
    }

    void Arrange(const Rect& allottedRect) override {
        m_Geometry = allottedRect;
    }

    void Paint(PaintContext& context) override {
        Rect iconRect = { m_Geometry.x, m_Geometry.y, m_Size, m_Size };
        IconPainter::DrawIcon(context, m_IconName, iconRect, Theme::Get().TextPrimary);
    }

private:
    std::string m_IconName;
    float m_Size;
};

} // namespace HouseEngine::UI
