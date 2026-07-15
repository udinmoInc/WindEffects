#pragma once

#include "Platform/Events.h"

#include <mutex>
#include <span>
#include <variant>
#include <vector>

namespace we::platform {

// Thread-safe pending queue with optional coalescing of high-frequency events
// (mouse move, resize, move, raw mouse) while preserving deterministic order.
class EventQueue {
public:
    void SetCoalescingEnabled(bool enabled) noexcept { m_Coalesce = enabled; }
    [[nodiscard]] bool IsCoalescingEnabled() const noexcept { return m_Coalesce; }
    [[nodiscard]] uint64_t CoalescedCount() const noexcept { return m_Coalesced; }
    void ResetCoalescedCount() noexcept { m_Coalesced = 0; }

    void Push(PlatformEvent event) {
        std::scoped_lock lock(m_Mutex);
        if (m_Coalesce && TryCoalesceLocked(event)) {
            ++m_Coalesced;
            return;
        }
        m_Pending.push_back(std::move(event));
    }

    void FlushToFrame() {
        std::scoped_lock lock(m_Mutex);
        m_Frame.clear();
        m_Frame.swap(m_Pending);
    }

    [[nodiscard]] std::span<const PlatformEvent> FrameEvents() const noexcept {
        return m_Frame;
    }

    void Clear() {
        std::scoped_lock lock(m_Mutex);
        m_Pending.clear();
        m_Frame.clear();
    }

private:
    template <typename T>
    static bool SameWindow(const T& a, const T& b) noexcept {
        return a.window == b.window;
    }

    bool TryCoalesceLocked(const PlatformEvent& incoming) {
        if (m_Pending.empty()) {
            return false;
        }
        PlatformEvent& last = m_Pending.back();

        if (auto* move = std::get_if<MouseMoveEvent>(&incoming)) {
            if (auto* prev = std::get_if<MouseMoveEvent>(&last); prev && SameWindow(*prev, *move)) {
                prev->position = move->position;
                prev->delta.x += move->delta.x;
                prev->delta.y += move->delta.y;
                prev->relative = move->relative;
                return true;
            }
        }
        if (auto* raw = std::get_if<RawMouseEvent>(&incoming)) {
            if (auto* prev = std::get_if<RawMouseEvent>(&last); prev && SameWindow(*prev, *raw)) {
                prev->delta.x += raw->delta.x;
                prev->delta.y += raw->delta.y;
                prev->buttons = raw->buttons;
                return true;
            }
        }
        if (auto* resize = std::get_if<WindowResizeEvent>(&incoming)) {
            if (auto* prev = std::get_if<WindowResizeEvent>(&last); prev && SameWindow(*prev, *resize)) {
                *prev = *resize;
                return true;
            }
        }
        if (auto* winMove = std::get_if<WindowMoveEvent>(&incoming)) {
            if (auto* prev = std::get_if<WindowMoveEvent>(&last); prev && SameWindow(*prev, *winMove)) {
                *prev = *winMove;
                return true;
            }
        }
        return false;
    }

    mutable std::mutex m_Mutex;
    std::vector<PlatformEvent> m_Pending;
    std::vector<PlatformEvent> m_Frame;
    bool m_Coalesce = true;
    uint64_t m_Coalesced = 0;
};

} // namespace we::platform
