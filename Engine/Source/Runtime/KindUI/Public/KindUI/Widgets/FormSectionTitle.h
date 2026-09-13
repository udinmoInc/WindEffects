// ==============================================================================
// WindEffects — KindUI — FormSectionTitle
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"

#include <string>

namespace we::runtime::kindui {

/// Reusable form/list section title band (Inspector/Landscape-style labeled header).
class KINDUI_API FormSectionTitle : public Widget {
public:
    explicit FormSectionTitle(std::string title, bool leadingGap = false);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void SetTitle(std::string title);
    [[nodiscard]] const std::string& GetTitle() const { return m_Title; }
    void SetLeadingGap(bool leadingGap);

private:
    std::string m_Title;
    bool m_LeadingGap = false;
    Rect m_TitleBand;
};

} // namespace we::runtime::kindui
