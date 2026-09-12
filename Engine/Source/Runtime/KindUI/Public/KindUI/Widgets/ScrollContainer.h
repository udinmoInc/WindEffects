// ==============================================================================
// WindEffects — KindUI — ScrollContainer
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Layout/ScrollViewport.h"
#include <memory>

namespace we::runtime::kindui {

/// Reusable scroll container wrapping any child widget layout with automatic scrollbars.
class KINDUI_API ScrollContainer : public Widget {
public:
    explicit ScrollContainer(std::shared_ptr<Widget> contentWidget = nullptr);
    ~ScrollContainer() override;

    void SetContentWidget(std::shared_ptr<Widget> contentWidget);
    [[nodiscard]] std::shared_ptr<Widget> GetContentWidget() const { return m_ContentWidget; }

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseWheel(const MouseEvent& event) override;

    [[nodiscard]] bool CanReceiveMouseWheelAt(const Point& pos) const override;
    [[nodiscard]] std::optional<Rect> GetHitTestClipRect() const override { return m_ScrollMetrics.viewport; }

private:
    void SyncScroll();

    std::shared_ptr<Widget> m_ContentWidget;
    ScrollViewport m_Scroll;
    ScrollViewportMetrics m_ScrollMetrics;
};

} // namespace we::runtime::kindui
