// ==============================================================================
// WindEffects — MainFrame — StatusBar
// Public API surface for the MainFrame module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "MainFrame/Export.h"

#include <KindUI/EditorUI.h>
#include <string>
#include <functional>
#include <vector>

namespace we::editor::toolbar { class ToolButton; }

namespace we::editor::shell {
using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::WidgetStyle;
using ::we::runtime::kindui::Row;
using ::we::runtime::kindui::VerticalDivider;

/// Clean, robust FooterBar / StatusBar widget for application status information.
/// Uses a flat child structure to ensure exact alignment at the bottom of the window.
class MAINFRAME_API StatusBar : public Row {
public:
    StatusBar();
    ~StatusBar() override;

    void Construct() override;
    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void SetHeight(float height) { m_Height = height; InvalidateLayout(); }
    void SetActiveFooterTab(int index);
    void SetOnFooterTabChanged(std::function<void(int)> onChanged);
    void SetOnCommandSubmitted(std::function<void(const std::string&)> onSubmitted);

    void SetOnOutputLogClicked(std::function<void()> onClicked);
    void SetOnBuildMenuClicked(std::function<void()> onClicked);
    void SetOnTraceClicked(std::function<void()> onClicked);
    void SetOnQualityMenuClicked(std::function<void()> onClicked);
    void SetQualityLabel(const std::string& label, const std::string& tooltip = {});

private:
    void SelectPanelTab(int index, bool notify);

    float m_Height = 0.0f;
    int m_ActivePanelTab = 0;

    std::function<void(int)> m_OnFooterTabChanged;
    std::function<void()> m_OnOutputLogClicked;

    // Flat direct children controls for predictable bottom-bar placement
    std::shared_ptr<::we::editor::toolbar::ToolButton> m_AssetsPanelButton;
    std::shared_ptr<VerticalDivider> m_Divider1;
    std::shared_ptr<::we::editor::toolbar::ToolButton> m_DiagnosticsPanelButton;
    std::shared_ptr<VerticalDivider> m_Divider2;
    std::shared_ptr<class CommandInput> m_CommandInput;
    std::shared_ptr<VerticalDivider> m_Divider3;
    std::shared_ptr<::we::editor::toolbar::ToolButton> m_OutputLogButton;
    std::shared_ptr<VerticalDivider> m_Divider4;
    std::shared_ptr<::we::editor::toolbar::ToolButton> m_BuildMenuButton;
    std::shared_ptr<VerticalDivider> m_Divider5;
    std::shared_ptr<::we::editor::toolbar::ToolButton> m_TraceButton;
    std::shared_ptr<VerticalDivider> m_Divider6;
    std::shared_ptr<::we::editor::toolbar::ToolButton> m_QualityMenuButton;
};

} // namespace we::editor::shell