// ==============================================================================
// WindEffects — KindUI — TreeColumnHeader
// UI widget used by the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Widgets/TreeColumnHeader.h"

#include "KindUI/Panel/PanelChrome.h"

namespace we::runtime::kindui {
namespace Chrome = ::we::runtime::kindui::panels::PanelChrome;

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

} // namespace we::runtime::kindui
