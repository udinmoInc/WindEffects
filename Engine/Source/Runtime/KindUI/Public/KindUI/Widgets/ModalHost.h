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

    void SetOnScrimClicked(std::function<void()> cb) { m_OnScrimClicked = std::move(cb); }
    void SetDismissOnScrim(bool enabled) { m_DismissOnScrim = enabled; }

private:
    std::shared_ptr<Widget> m_Content;
    float m_DialogWidth = 520.0f;
    float m_DialogHeight = 0.0f;
    bool m_DismissOnScrim = true;
    std::function<void()> m_OnScrimClicked;
};

[[nodiscard]] KINDUI_API std::shared_ptr<ModalHost> MakeModalHost();

} // namespace we::runtime::kindui
