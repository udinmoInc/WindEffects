// ==============================================================================
// WindEffects — MainFrame — WindowShell
// Public API surface for the MainFrame module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "MainFrame/Export.h"

#include <KindUI/EditorUI.h>
#include <memory>
namespace we::editor::shell {
using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::WidgetStyle;


// Draws a square border around the full application client area.
class MAINFRAME_API WindowShell : public Widget {
public:
    WindowShell();
    ~WindowShell() override;

    void SetContent(const std::shared_ptr<Widget>& content);
    const std::shared_ptr<Widget>& GetContent() const { return m_Content; }

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

private:
    std::shared_ptr<Widget> m_Content;
};

} // namespace we::editor::shell