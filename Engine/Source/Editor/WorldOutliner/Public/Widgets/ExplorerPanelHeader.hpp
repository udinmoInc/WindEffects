#pragma once

#include "Core/Widget.hpp"
#include <functional>
#include <string>
#include <vector>

namespace we::UI {

class ExplorerPanelHeader : public Widget {
public:
    static constexpr float kDefaultHeight = 30.0f;
    static constexpr float kLogoLogicalSize = 16.0f;

    ExplorerPanelHeader();

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override;

    void SetTitle(const std::string& title) { m_Title = title; }
    void SetHeight(float height) { m_Height = height; }

    void SetOnAdd(std::function<void()> callback) { m_OnAdd = std::move(callback); }
    void SetOnFilter(std::function<void()> callback) { m_OnFilter = std::move(callback); }
    void SetOnSettings(std::function<void()> callback) { m_OnSettings = std::move(callback); }
    void SetOnMore(std::function<void()> callback) { m_OnMore = std::move(callback); }

private:
    struct HeaderButton {
        std::string iconName;
        Rect geometry;
    };

    int HitButtonIndex(const Point& position) const;

    std::string m_Title = "Explorer";
    float m_Height = kDefaultHeight;
    int m_HoveredButton = -1;
    int m_PressedButton = -1;

    std::function<void()> m_OnAdd;
    std::function<void()> m_OnFilter;
    std::function<void()> m_OnSettings;
    std::function<void()> m_OnMore;

    std::vector<HeaderButton> m_Buttons;
};

} // namespace we::UI
