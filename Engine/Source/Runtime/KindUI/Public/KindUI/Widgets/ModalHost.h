// ==============================================================================
// WindEffects — KindUI — ModalHost
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"

#include <functional>
#include <memory>

namespace we::runtime::kindui {

/// Centered modal overlay with scrim. Content is supplied declaratively via DialogService.
class KINDUI_API ModalHost : public Widget {
public:
    void SetContent(const std::shared_ptr<Widget>& content);
    void SetDialogWidth(float width) { m_DialogWidth = width; }
    void SetDialogHeight(float height) { m_DialogHeight = height; }
    [[nodiscard]] bool HasContent() const { return m_Content != nullptr; }

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseDown(const MouseEvent& event) override;
    void Tick(float deltaTime) override;
    [[nodiscard]] bool IsPointerTransparent() const override;

    void SetOnScrimClicked(std::function<void()> cb) { m_OnScrimClicked = std::move(cb); }
    void SetDismissOnScrim(bool enabled) { m_DismissOnScrim = enabled; }
    void SetShowScrim(bool show) { m_ShowScrim = show; }
    void SetAnchorPosition(const std::optional<Point>& anchor) { m_AnchorPosition = anchor; }
    void SetCenterInParent(bool center) { m_CenterInParent = center; }

private:
    std::shared_ptr<Widget> m_Content;
    float m_DialogWidth = 520.0f;
    float m_DialogHeight = 0.0f;
    bool m_DismissOnScrim = true;
    bool m_ShowScrim = false;
    bool m_CenterInParent = true;
    std::optional<Point> m_AnchorPosition;
    std::function<void()> m_OnScrimClicked;
};

[[nodiscard]] KINDUI_API std::shared_ptr<ModalHost> MakeModalHost();

}
