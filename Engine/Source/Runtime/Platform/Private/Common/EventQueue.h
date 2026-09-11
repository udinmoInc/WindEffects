// ==============================================================================
// WindEffects — Platform — EventQueue
// Internal implementation for the Platform module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Platform/Events.h"
#include "Core/LoopExecutionTrace.h"

#include <chrono>
#include <mutex>
#include <span>
#include <string>
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
        const auto waitStart = std::chrono::steady_clock::now();
        std::scoped_lock lock(m_Mutex);
        if (we::runtime::core::LoopExecutionTrace::IsEnabled()) {
            const double waitMs = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - waitStart).count();
            we::runtime::core::LoopExecutionTrace::MutexWait("EventQueue.Push", waitMs);
        }
        if (m_Coalesce && TryCoalesceLocked(event)) {
            ++m_Coalesced;
            return;
        }
        m_Pending.push_back(std::move(event));
    }

    void FlushToFrame() {
        const auto waitStart = std::chrono::steady_clock::now();
        std::scoped_lock lock(m_Mutex);
        if (we::runtime::core::LoopExecutionTrace::IsEnabled()) {
            const double waitMs = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - waitStart).count();
            we::runtime::core::LoopExecutionTrace::MutexWait("EventQueue.FlushToFrame", waitMs);
            we::runtime::core::LoopExecutionTrace::Event(
                "EventQueue.FlushToFrame",
                "pending=" + std::to_string(m_Pending.size())
                    + " coalesced=" + std::to_string(m_Coalesced));
        }
        m_Frame.clear();
        m_Frame.swap(m_Pending);
    }

    [[nodiscard]] std::span<const PlatformEvent> FrameEvents() const noexcept {
        return m_Frame;
    }

    [[nodiscard]] size_t PendingCount() const {
        std::scoped_lock lock(m_Mutex);
        return m_Pending.size();
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
