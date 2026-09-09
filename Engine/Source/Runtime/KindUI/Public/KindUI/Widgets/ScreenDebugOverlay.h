// ==============================================================================
// WindEffects — KindUI — ScreenDebugOverlay
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
#include <vector>

namespace we::runtime::kindui {

// Top-left live readout for WE_SCREEN_DEBUG=1. Pointer-transparent so it
// never eats clicks; paints the latest ScreenRecorder sample as text lines.
class KINDUI_API ScreenDebugOverlay : public Widget {
public:
    ScreenDebugOverlay();
    ~ScreenDebugOverlay() override = default;

    void Tick(float deltaTime) override;
    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    [[nodiscard]] bool IsPointerTransparent() const override { return true; }

private:
    std::vector<std::string> m_Lines;
    float m_RefreshTimer = 0.0f;
};

} // namespace we::runtime::kindui
