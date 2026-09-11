// ==============================================================================
// WindEffects — Toolbar — WindowsPanelMenuButton
// Public API surface for the Toolbar module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Toolbar/Export.h"
#include "KindUI/Core/Widget.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace we::editor::extensions {
struct PanelRegistration;
class UIExtensionRegistry;
}

namespace we::editor::toolbar {

struct WindowsPanelMenuEntry {
    std::string panelId;
    std::string label;
    bool visible = true;
};

/// Panel visibility dropdown for the main editor toolbar (Window menu parity).
class TOOLBAR_API WindowsPanelMenuButton : public ::we::runtime::kindui::Widget {
public:
    static std::shared_ptr<WindowsPanelMenuButton> Create(
        const ::we::editor::extensions::UIExtensionRegistry& extensions,
        std::function<void(const std::string& panelId)> onTogglePanel,
        std::function<bool(const std::string& panelId)> isPanelVisible);

    ::we::runtime::kindui::Size Measure(const ::we::runtime::kindui::Size& availableSize) override;
    void Arrange(const ::we::runtime::kindui::Rect& allottedRect) override;
    void Tick(float deltaTime) override;
    void Paint(::we::runtime::kindui::PaintContext& context) override;
    void OnMouseDown(const ::we::runtime::kindui::MouseEvent& event) override;
    void OnMouseUp(const ::we::runtime::kindui::MouseEvent& event) override;
    bool ShowsPointerCursor(const ::we::runtime::kindui::Point& position) const override;

    void RefreshVisibilityState();

private:
    WindowsPanelMenuButton() = default;
    void ShowMenu();

    std::vector<WindowsPanelMenuEntry> m_Entries;
    std::function<void(const std::string& panelId)> m_OnTogglePanel;
    std::function<bool(const std::string& panelId)> m_IsPanelVisible;
    float m_HoverAnim = 0.0f;
};

} // namespace we::editor::toolbar
