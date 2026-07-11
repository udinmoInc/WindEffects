#include "Widgets/Toolbar.h"
#include "Core/PaintContext.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include "Core/Icon.h"
#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"
#include "Core/DPIContext.h"
#include "Widgets/ToolButton.h"
#include <algorithm>
#include <functional>

namespace WindEffects::Editor::UI {
namespace {
    float ScaledMetric(ThemeToken token) {
        return ResolveThemeMetric(token) * (std::max)(1.0f, DPIContext::GetScale());
    }

    void VisitToolbarWidgets(const std::shared_ptr<Widget>& widget, const std::function<void(const std::shared_ptr<Widget>&)>& visitor) {
        if (!widget) return;
        visitor(widget);
        for (const auto& child : widget->GetChildren()) {
            VisitToolbarWidgets(child, visitor);
        }
    }

    std::shared_ptr<Widget> HitWidgetAt(const std::shared_ptr<Widget>& widget, const Point& position) {
        if (!widget || !widget->IsVisible()) return nullptr;

        for (auto it = widget->GetChildren().rbegin(); it != widget->GetChildren().rend(); ++it) {
            if (auto hit = HitWidgetAt(*it, position)) {
                return hit;
            }
        }

        if (widget->GetGeometry().Contains(position)) {
            return widget;
        }
        return nullptr;
    }
}

Toolbar::Toolbar()
    : m_Style(WidgetStyle::Panel())
{
}

Size Toolbar::Measure(const Size& availableSize) {
    for (auto& tool : m_Tools) {
        if (tool.button && tool.button->IsVisible()) {
            tool.button->Measure(availableSize);
        }
    }
    m_DesiredSize = Size{ availableSize.width, m_Height };
    return m_DesiredSize;
}

void Toolbar::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    const float itemSpacing = ScaledMetric(ThemeToken::Space2);
    const float groupSpacing = ScaledMetric(ThemeToken::ButtonGroupSpacing);

    std::vector<ToolInfo*> leftTools, centerTools, rightTools;
    for (auto& tool : m_Tools) {
        if (tool.button && !tool.button->IsVisible()) continue;

        if (tool.align == ToolbarAlignment::Left) leftTools.push_back(&tool);
        else if (tool.align == ToolbarAlignment::Center) centerTools.push_back(&tool);
        else if (tool.align == ToolbarAlignment::Right) rightTools.push_back(&tool);
    }

    auto layoutGroup = [&](const std::vector<ToolInfo*>& tools, float startX, bool isRightAligned) -> float {
        struct ItemToPlace {
            std::shared_ptr<Widget> button;
            float width;
            float marginBefore;
        };

        std::vector<ItemToPlace> items;
        float pendingSpacing = 0.0f;
        bool isFirstButton = true;

        for (auto* tool : tools) {
            if (tool->isSeparator && tool->button) {
                if (!isFirstButton) {
                    items.push_back({ tool->button, groupSpacing, tool->button->GetDesiredSize().width });
                    pendingSpacing = itemSpacing;
                }
            } else if (tool->button) {
                float w = tool->button->GetDesiredSize().width;
                float margin = isFirstButton ? 0.0f : pendingSpacing;
                if (!isFirstButton && pendingSpacing == 0.0f) margin = itemSpacing;
                items.push_back({ tool->button, w, margin });
                pendingSpacing = itemSpacing;
                isFirstButton = false;
            }
        }

        float totalWidth = 0.0f;
        for (const auto& item : items) {
            totalWidth += item.marginBefore + item.width;
        }

        float currentX = startX;
        if (isRightAligned) {
            currentX = startX - totalWidth;
        }

        for (const auto& item : items) {
            currentX += item.marginBefore;
            float itemHeight = item.button->GetDesiredSize().height;
            float y = allottedRect.y + (m_Height - itemHeight) * 0.5f;
            item.button->Arrange(Rect{ currentX, y, item.width, itemHeight });
            currentX += item.width;
        }

        return totalWidth;
    };

    layoutGroup(leftTools, allottedRect.x + m_LeftInset, false);
    layoutGroup(rightTools, allottedRect.x + allottedRect.width - m_EdgePadding - m_RightInset, true);

    auto computeWidth = [&](const std::vector<ToolInfo*>& tools) -> float {
        float pendingSpacing = 0.0f;
        bool isFirstButton = true;
        float totalWidth = 0.0f;
        for (auto* tool : tools) {
            if (tool->isSeparator && tool->button) {
                if (!isFirstButton) {
                    totalWidth += groupSpacing + tool->button->GetDesiredSize().width;
                    pendingSpacing = itemSpacing;
                }
            } else if (tool->button) {
                float margin = isFirstButton ? 0.0f : pendingSpacing;
                if (!isFirstButton && pendingSpacing == 0.0f) margin = itemSpacing;
                totalWidth += margin + tool->button->GetDesiredSize().width;
                pendingSpacing = itemSpacing;
                isFirstButton = false;
            }
        }
        return totalWidth;
    };

    float centerTotalWidth = computeWidth(centerTools);
    float centerStartX = allottedRect.x + (allottedRect.width - centerTotalWidth) * 0.5f;
    layoutGroup(centerTools, centerStartX, false);
}

void Toolbar::Paint(PaintContext& context) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());

    context.DrawRect(m_Geometry, ThemeColor(ThemeToken::ToolbarBackground));

    Rect bottomEdge{
        m_Geometry.x,
        m_Geometry.y + m_Geometry.height - 1.0f * uiScale,
        m_Geometry.width,
        1.0f * uiScale
    };
    context.DrawRect(bottomEdge, ThemeColor(ThemeToken::BorderDark));

    for (auto& tool : m_Tools) {
        if (tool.button && tool.button->IsVisible()) {
            tool.button->Paint(context);
        }
    }
}

std::shared_ptr<ToolButton> Toolbar::AddTool(const std::string& iconName, const std::string& label, std::function<void()> onClick, const std::string& tooltip, bool isPlayButton, ToolbarAlignment align) {
    ToolInfo tool;
    tool.iconName = iconName;
    tool.isSeparator = false;
    tool.align = align;

    auto btn = std::make_shared<ToolButton>(iconName, label, onClick, tooltip);
    if (isPlayButton) {
        btn->SetButtonStyle(ToolButtonStyle::PlayButton);
    } else if (label.empty()) {
        btn->SetButtonStyle(ToolButtonStyle::ToolbarIconOnly);
    }
    tool.button = btn;
    m_Tools.push_back(tool);

    return btn;
}

void Toolbar::AddSeparator(ToolbarAlignment align) {
    ToolInfo tool;
    tool.isSeparator = true;
    tool.align = align;
    tool.button = std::make_shared<ToolbarSeparator>();
    m_Tools.push_back(tool);
}

void Toolbar::AddWidget(std::shared_ptr<Widget> widget, ToolbarAlignment align) {
    ToolInfo tool;
    tool.iconName = "";
    tool.isSeparator = false;
    tool.align = align;
    tool.button = widget;
    m_Tools.push_back(tool);
}

void Toolbar::AddGroup(std::shared_ptr<Widget> group, ToolbarAlignment align) {
    AddWidget(std::move(group), align);
}

void Toolbar::Clear() {
    m_Tools.clear();
    m_ActiveTool.clear();
}

void Toolbar::SetActiveTool(const std::string& iconName) {
    m_ActiveTool = iconName;

    for (auto& tool : m_Tools) {
        if (!tool.button || tool.isSeparator) continue;

        VisitToolbarWidgets(tool.button, [&](const std::shared_ptr<Widget>& widget) {
            if (auto toolBtn = std::dynamic_pointer_cast<ToolButton>(widget)) {
                toolBtn->SetActive(toolBtn->GetIconName() == iconName);
            }
        });
    }
}

std::shared_ptr<Widget> Toolbar::HitToolAt(const Point& position) const {
    for (auto it = m_Tools.rbegin(); it != m_Tools.rend(); ++it) {
        if (!it->button || !it->button->IsVisible()) continue;
        if (auto hit = HitWidgetAt(it->button, position)) {
            return hit;
        }
    }
    return nullptr;
}

void Toolbar::OnMouseDown(const MouseEvent& event) {
    if (auto hit = HitToolAt(event.position)) {
        hit->OnMouseDown(event);
    }
}

void Toolbar::OnMouseMove(const MouseEvent& event) {
    if (auto hit = HitToolAt(event.position)) {
        hit->OnMouseMove(event);
    }
}

void Toolbar::OnMouseUp(const MouseEvent& event) {
    if (auto hit = HitToolAt(event.position)) {
        hit->OnMouseUp(event);
    }
}

void Toolbar::OnMouseWheel(const MouseEvent& event) {
    if (auto hit = HitToolAt(event.position)) {
        hit->OnMouseWheel(event);
    }
}

bool Toolbar::ShowsPointerCursor(const Point& position) const {
    if (auto hit = HitToolAt(position)) {
        return hit->ShowsPointerCursor(position);
    }
    return false;
}

ToolbarSeparator::ToolbarSeparator() {}

Size ToolbarSeparator::Measure(const Size& availableSize) {
    (void)availableSize;
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    return Size{ 1.0f * uiScale, ThemeMetric(ThemeToken::IconButtonSize) * uiScale };
}

void ToolbarSeparator::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void ToolbarSeparator::Paint(PaintContext& context) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float lineHeight = 14.0f * uiScale;
    const float centerY = m_Geometry.y + m_Geometry.height * 0.5f;
    const float halfH = lineHeight * 0.5f;

    Rect lineRect{
        m_Geometry.x + m_Geometry.width * 0.5f - 0.5f * uiScale,
        centerY - halfH,
        1.0f * uiScale,
        lineHeight
    };
    context.DrawRect(lineRect, ThemeColor(ThemeToken::Separator));
}

ToolbarGroup::ToolbarGroup() = default;

void ToolbarGroup::AddChildWidget(const std::shared_ptr<Widget>& child) {
    if (child) {
        m_Items.push_back(child);
        AddChild(child);
    }
}

Size ToolbarGroup::Measure(const Size& availableSize) {
    (void)availableSize;
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float padH = ThemeMetric(ThemeToken::Space1) * uiScale;
    const float itemGap = ThemeMetric(ThemeToken::ButtonSpacing) * uiScale;
    float maxHeight = ThemeMetric(ThemeToken::IconButtonSize) * uiScale;
    float width = padH * 2.0f;

    for (size_t i = 0; i < m_Items.size(); ++i) {
        if (i > 0) width += itemGap;
        m_Items[i]->Measure(availableSize);
        width += m_Items[i]->GetDesiredSize().width;
        maxHeight = (std::max)(maxHeight, m_Items[i]->GetDesiredSize().height);
    }

    m_DesiredSize = Size{ width, maxHeight + padH * 2.0f };
    return m_DesiredSize;
}

void ToolbarGroup::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float padH = ThemeMetric(ThemeToken::Space1) * uiScale;
    const float itemGap = ThemeMetric(ThemeToken::ButtonSpacing) * uiScale;

    float currentX = allottedRect.x + padH;
    for (const auto& item : m_Items) {
        const Size itemSize = item->GetDesiredSize();
        const float y = allottedRect.y + (allottedRect.height - itemSize.height) * 0.5f;
        item->Arrange(Rect{ currentX, y, itemSize.width, itemSize.height });
        currentX += itemSize.width + itemGap;
    }
}

void ToolbarGroup::Paint(PaintContext& context) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float radius = ThemeMetric(ThemeToken::IconButtonRadius) * uiScale;
    Color groupBg = ThemeColor(ThemeToken::ButtonPrimaryBackground);
    if (m_Elevated) {
        groupBg = Color::Lerp(groupBg, ThemeColor(ThemeToken::HoverBackground), 0.18f);
    }
    context.DrawRoundedRect(m_Geometry, groupBg, radius);

    for (const auto& item : m_Items) {
        if (item && item->IsVisible()) {
            item->Paint(context);
        }
    }
}

std::shared_ptr<Widget> ToolbarGroup::HitChildAt(const Point& position) const {
    for (auto it = m_Items.rbegin(); it != m_Items.rend(); ++it) {
        if (*it && (*it)->IsVisible() && (*it)->GetGeometry().Contains(position)) {
            return *it;
        }
    }
    return nullptr;
}

void ToolbarGroup::OnMouseDown(const MouseEvent& event) {
    if (auto hit = HitChildAt(event.position)) hit->OnMouseDown(event);
}

void ToolbarGroup::OnMouseMove(const MouseEvent& event) {
    if (auto hit = HitChildAt(event.position)) hit->OnMouseMove(event);
}

void ToolbarGroup::OnMouseUp(const MouseEvent& event) {
    if (auto hit = HitChildAt(event.position)) hit->OnMouseUp(event);
}

void ToolbarGroup::OnMouseWheel(const MouseEvent& event) {
    if (auto hit = HitChildAt(event.position)) hit->OnMouseWheel(event);
}

bool ToolbarGroup::ShowsPointerCursor(const Point& position) const {
    if (auto hit = HitChildAt(position)) {
        return hit->ShowsPointerCursor(position);
    }
    return false;
}

} // namespace WindEffects::Editor::UI
