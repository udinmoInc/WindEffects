#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Layout/OverlayManager.h"
#include "KindUI/Layout/ScrollLayout.h"
#include "KindUI/Profiling/UiInputDebug.h"
#include "KindUI/Profiling/UiInputLatencyAudit.h"
#include "Platform/Platform.h"

#include <cstdint>
#include <string>

namespace we::runtime::kindui {

EventSystem::~EventSystem() = default;

namespace {

std::string Utf8FromCodepoint(char32_t codepoint) {
    if (codepoint <= 0x7F) {
        return std::string(1, static_cast<char>(codepoint));
    }
    if (codepoint <= 0x7FF) {
        std::string out(2, '\0');
        out[0] = static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
        out[1] = static_cast<char>(0x80 | (codepoint & 0x3F));
        return out;
    }
    if (codepoint <= 0xFFFF) {
        std::string out(3, '\0');
        out[0] = static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
        out[1] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        out[2] = static_cast<char>(0x80 | (codepoint & 0x3F));
        return out;
    }
    std::string out(4, '\0');
    out[0] = static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07));
    out[1] = static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
    out[2] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
    out[3] = static_cast<char>(0x80 | (codepoint & 0x3F));
    return out;
}

void DispatchMouseWheel(const std::shared_ptr<Widget>& start, const MouseEvent& event) {
    for (auto node = start; node; node = node->GetParent()) {
        if (node->CanReceiveMouseWheelAt(event.position)) {
            node->OnMouseWheel(event);
            return;
        }
    }
}

} // namespace

std::shared_ptr<Widget> EventSystem::HitTest(const std::shared_ptr<Widget>& root, const Point& pos) {
    if (!root) {
        return nullptr;
    }
    return root->HitTestPoint(pos, nullptr);
}

void EventSystem::ProcessMouseEvent(const MouseEvent& event) {
    if (!m_Root) return;

    if (UiInputLatencyAudit::IsEnabled()) {
        UiInputLatencyAudit::Get().OnEventSystemReceive();
    }

    std::shared_ptr<Widget> hitWidget = HitTest(m_Root, event.position);

    std::shared_ptr<Widget> oldHovered = m_HoveredWidget.lock();
    if (hitWidget != oldHovered) {
        if (oldHovered) {
            oldHovered->SetHovered(false);
        }
        if (hitWidget) {
            hitWidget->SetHovered(true);
        }
        m_HoveredWidget = hitWidget;
    }

    if (event.type == MouseEventType::MouseMove && !m_SuppressSystemCursor) {
        UpdateCursorForWidget(hitWidget, event.position);
    }

    std::shared_ptr<Widget> targetWidget = hitWidget;
    if (auto captured = m_CapturedWidget.lock()) {
        targetWidget = captured;
    } else if (event.type == MouseEventType::MouseUp) {
        if (auto focused = m_FocusedWidget.lock()) {
            if (m_PopupHost && hitWidget && m_PopupHost->IsWidgetInPopup(hitWidget)) {
                targetWidget = hitWidget;
            } else {
                targetWidget = focused;
            }
        }
    }

    UiInputDebug::OnMouseEvent(
        event,
        event.position,
        hitWidget,
        targetWidget,
        m_FocusedWidget.lock(),
        m_CapturedWidget.lock());

    if (targetWidget) {
        if (event.type == MouseEventType::MouseDown) {
            if (m_PopupHost) {
                if (m_PopupHost->HasOpenPopups() && !m_PopupHost->IsWidgetInPopup(hitWidget)) {
                    m_PopupHost->CloseAllPopups();
                }
            }

            if (hitWidget && hitWidget->IsFocusable()) {
                SetFocusedWidget(hitWidget);
            } else if (!hitWidget || !m_PopupHost || !m_PopupHost->IsWidgetInPopup(hitWidget)) {
                SetFocusedWidget(nullptr);
            }
            SetCapturedWidget(targetWidget);
            targetWidget->OnMouseDown(event);
        } else if (event.type == MouseEventType::MouseUp) {
            targetWidget->OnMouseUp(event);
            m_CapturedWidget.reset();
        } else if (event.type == MouseEventType::MouseMove) {
            targetWidget->OnMouseMove(event);
        } else if (event.type == MouseEventType::MouseWheel) {
            DispatchMouseWheel(hitWidget ? hitWidget : targetWidget, event);
        }
    } else {
        if (event.type == MouseEventType::MouseDown) {
            if (m_PopupHost) {
                m_PopupHost->CloseAllPopups();
            }
            SetFocusedWidget(nullptr);
            m_CapturedWidget.reset();
        } else if (event.type == MouseEventType::MouseUp) {
            m_CapturedWidget.reset();
        } else if (event.type == MouseEventType::MouseWheel) {
            DispatchMouseWheel(m_Root, event);
        }
    }

    if (UiInputLatencyAudit::IsEnabled()) {
        UiInputLatencyAudit::Get().OnWidgetHandler();
    }
}

void EventSystem::ProcessTextInput(char32_t codepoint) {
    if (codepoint < 32 && codepoint != '\t') {
        return;
    }

    auto focused = m_FocusedWidget.lock();
    UiInputDebug::OnTextInput(codepoint, focused);
    if (!focused) {
        return;
    }

    focused->OnTextInput(Utf8FromCodepoint(codepoint));
}

void EventSystem::UpdateCursorForWidget(const std::shared_ptr<Widget>& widget, const Point& position) {
    const bool shouldUsePointerCursor = widget && widget->ShowsPointerCursor(position);
    if (shouldUsePointerCursor == m_UsingPointerCursor) {
        return;
    }

    auto& platform = we::platform::Platform::Get();
    platform.SetSystemCursor(shouldUsePointerCursor
        ? we::platform::SystemCursor::Hand
        : we::platform::SystemCursor::Arrow);
    m_UsingPointerCursor = shouldUsePointerCursor;
}

void EventSystem::ProcessKeyEvent(const KeyEvent& event) {
    if (event.type == KeyEventType::KeyDown &&
        event.key == we::platform::KeyCode::Tab) {
        FocusNext(event.shiftDown);
        return;
    }

    if (auto focused = m_FocusedWidget.lock()) {
        if (event.type == KeyEventType::KeyUp) {
            focused->OnKeyUp(event);
        } else {
            focused->OnKeyDown(event);
        }
    }
}

void EventSystem::CollectFocusable(const std::shared_ptr<Widget>& node, std::vector<std::shared_ptr<Widget>>& out) const {
    if (!node || !node->IsVisible() || !node->IsActive() || !node->IsFocusable()) return;
    out.push_back(node);
    for (const auto& child : node->GetChildren()) {
        CollectFocusable(child, out);
    }
}

void EventSystem::FocusNext(bool reverse) {
    if (!m_Root) return;
    std::vector<std::shared_ptr<Widget>> focusable;
    CollectFocusable(m_Root, focusable);
    if (focusable.empty()) return;

    auto current = m_FocusedWidget.lock();
    int index = 0;
    if (current) {
        for (size_t i = 0; i < focusable.size(); ++i) {
            if (focusable[i] == current) {
                index = static_cast<int>(i);
                break;
            }
        }
    }

    if (reverse) {
        index = (index - 1 + static_cast<int>(focusable.size())) % static_cast<int>(focusable.size());
    } else {
        index = (index + 1) % static_cast<int>(focusable.size());
    }
    SetFocusedWidget(focusable[static_cast<size_t>(index)]);
}

void EventSystem::SetFocusedWidget(const std::shared_ptr<Widget>& widget) {
    std::shared_ptr<Widget> oldFocused = m_FocusedWidget.lock();
    if (widget == oldFocused) return;

    if (oldFocused) {
        oldFocused->OnBlur();
    }
    if (widget) {
        widget->OnFocus();
    }
    m_FocusedWidget = widget;
}

} // namespace we::runtime::kindui
