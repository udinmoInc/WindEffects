// ==============================================================================
// WindEffects — ContentBrowser — TreeColumnHeader
// UI widget used by the ContentBrowser module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "ContentBrowser/Widgets/TreeColumnHeader.h"

#include "KindUI/Panel/PanelChrome.h"

namespace we::editor::contentbrowser {
namespace Chrome = ::we::runtime::kindui::panels::PanelChrome;

using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Size;

Size TreeColumnHeader::Measure(const Size& availableSize) {
    m_DesiredSize = Size{
        availableSize.width < 1.0e8f ? availableSize.width : 0.0f,
        Chrome::ColumnHeaderRowHeight()
    };
    return m_DesiredSize;
}

void TreeColumnHeader::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void TreeColumnHeader::Paint(PaintContext& context) {
    Chrome::PaintExplorerColumnHeader(context, m_Geometry, "Item Label");
}

} // namespace we::editor::contentbrowser
