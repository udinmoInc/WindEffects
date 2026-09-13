// ==============================================================================
// WindEffects — KindUI — EventSystem
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Layout/OverlayManager.h"
#include "KindUI/Layout/ScrollLayout.h"
#include "KindUI/Profiling/UiInputDebug.h"
#include "KindUI/Profiling/UiInputLatencyAudit.h"
#include "Platform/Platform.h"

#include <cmath>
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

bool IsWidgetHierarchyValidForInput(const std::shared_ptr<Widget>& widget) {
    if (!widget) return false;
    for (auto cur = widget; cur; cur = cur->GetParent()) {
        if (!cur->IsVisible() || !cur->IsEnabled() || !cur->IsActive()) {
            return false;
        }
    }
    return true;
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

    auto captured = m_CapturedWidget.lock();
    if (captured && !IsWidgetHierarchyValidForInput(captured)) {
        captured->OnCaptureLost();
        m_CapturedWidget.reset();
        captured = nullptr;
    }

    // Filter sub-pixel OS jitter: skip move events where the cursor hasn't moved
    // more than 0.5 logical pixels. Windows fires WM_MOUSEMOVE even while the
    // mouse is stationary (sub-pixel noise), which would otherwise trigger the full
    // hover chain, OnMouseMove dispatch, tooltip recalc, and InvalidatePaint every frame.
    if (event.type == MouseEventType::MouseMove && !captured) {
        const float dx = event.position.x - m_LastMousePos.x;
        const float dy = event.position.y - m_LastMousePos.y;
        if (std::abs(dx) < 0.5f && std::abs(dy) < 0.5f) {
            return;
        }
    }
    m_LastMousePos = event.position;

    std::shared_ptr<Widget> hitWidget = HitTest(m_Root, event.position);
    if (hitWidget && !IsWidgetHierarchyValidForInput(hitWidget)) {
        hitWidget = nullptr;
    }
    std::shared_ptr<Widget> oldHovered = m_HoveredWidget.lock();

    if (hitWidget != oldHovered) {
        UiInputDebug::OnHoverChanged(oldHovered, hitWidget, event.position);

        if (m_HoveredWidget.lock() != hitWidget) {
            m_HoveredWidget = hitWidget;

            std::vector<std::shared_ptr<Widget>> newChain;
            for (auto curr = hitWidget; curr; curr = curr->GetParent()) {
                newChain.push_back(curr);
            }

            // Determine widgets that lost hover: in m_HoverChain but not in newChain
            for (auto& weakOld : m_HoverChain) {
                if (auto old = weakOld.lock()) {
                    bool stillHovered = false;
                    for (const auto& nw : newChain) {
                        if (nw == old) {
                            stillHovered = true;
                            break;
                        }
                    }
                    if (!stillHovered) {
                        old->SetHovered(false);
                    }
                }
            }

            // Determine widgets that gained hover: in newChain but not in m_HoverChain
            for (const auto& nw : newChain) {
                bool wasHovered = false;
                for (const auto& weakOld : m_HoverChain) {
                    if (weakOld.lock() == nw) {
                        wasHovered = true;
                        break;
                    }
                }
                if (!wasHovered) {
                    nw->SetHovered(true);
                }
            }

            m_HoverChain.clear();
            m_HoverChain.reserve(newChain.size());
            for (const auto& nw : newChain) {
                m_HoverChain.push_back(nw);
            }
        }

        if (oldHovered && !m_SuppressSystemCursor) {
            we::platform::Platform::Get().SetSystemCursor(we::platform::SystemCursor::Arrow);
            m_UsingPointerCursor = false;
        }
        m_HoveredWidget = hitWidget;
    }

    if (event.type == MouseEventType::MouseMove && !m_SuppressSystemCursor) {
        if (!hitWidget || IsWidgetHierarchyValidForInput(hitWidget)) {
            UpdateCursorForWidget(hitWidget, event.position);
        }
    }

    std::shared_ptr<Widget> targetWidget = captured ? captured : hitWidget;
    if (targetWidget && !IsWidgetHierarchyValidForInput(targetWidget)) {
        targetWidget = nullptr;
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
            ClearCapture();
            targetWidget = hitWidget;

            if (m_PopupHost) {
                if (m_PopupHost->HasOpenPopups() && (!hitWidget || !m_PopupHost->IsWidgetInPopup(hitWidget))) {
                    m_PopupHost->CloseTransientPopups();
                    // Re-test hitWidget after popup closure in case background controls became visible/unblocked
                    hitWidget = HitTest(m_Root, event.position);
                    targetWidget = hitWidget;
                }
            }

            if (hitWidget && hitWidget->IsFocusable()) {
                SetFocusedWidget(hitWidget);
            } else if (!hitWidget || !m_PopupHost || !m_PopupHost->IsWidgetInPopup(hitWidget)) {
                SetFocusedWidget(nullptr);
            }

            if (targetWidget) {
                SetCapturedWidget(targetWidget);
                targetWidget->OnMouseDown(event);
            }
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
            ClearCapture();
            if (m_PopupHost) {
                m_PopupHost->CloseTransientPopups();
            }
            SetFocusedWidget(nullptr);
        } else if (event.type == MouseEventType::MouseUp) {
            ClearCapture();
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
    if (focused && !IsWidgetHierarchyValidForInput(focused)) {
        SetFocusedWidget(nullptr);
        focused = nullptr;
    }

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

void EventSystem::ClearHover() {
    for (auto& weak : m_HoverChain) {
        if (auto w = weak.lock()) {
            w->SetHovered(false);
        }
    }
    m_HoverChain.clear();
    m_HoveredWidget.reset();
    if (!m_SuppressSystemCursor && m_UsingPointerCursor) {
        we::platform::Platform::Get().SetSystemCursor(we::platform::SystemCursor::Arrow);
        m_UsingPointerCursor = false;
    }
}

void EventSystem::ClearCapture() {
    if (auto captured = m_CapturedWidget.lock()) {
        captured->OnCaptureLost();
    }
    m_CapturedWidget.reset();
}

void EventSystem::ClearAllInputState() {
    ClearCapture();
    SetFocusedWidget(nullptr);
    ClearHover();
}

void EventSystem::ProcessKeyEvent(const KeyEvent& event) {
    if (event.type == KeyEventType::KeyDown &&
        event.key == we::platform::KeyCode::Tab) {
        FocusNext(event.shiftDown);
        return;
    }

    if (auto focused = m_FocusedWidget.lock()) {
        if (!IsWidgetHierarchyValidForInput(focused)) {
            SetFocusedWidget(nullptr);
        } else {
            if (event.type == KeyEventType::KeyUp) {
                focused->OnKeyUp(event);
            } else {
                focused->OnKeyDown(event);
            }
        }
    }
}

void EventSystem::CollectFocusable(const std::shared_ptr<Widget>& node, std::vector<std::shared_ptr<Widget>>& out)
    const {
    if (!node || !node->IsVisible() || !node->IsActive()) return;
    if (node->IsFocusable()) {
        out.push_back(node);
    }
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
 
