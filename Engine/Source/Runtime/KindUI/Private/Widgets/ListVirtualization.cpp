// ==============================================================================
// WindEffects — KindUI — ListVirtualization
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/UI/ListVirtualization.h"

#include <unordered_map>

namespace we::runtime::kindui {

void WidgetRecyclePool::SyncWindow(
    ListVisibleRange range,
    std::vector<Slot>& slots,
    const std::function<std::shared_ptr<Widget>(size_t index, std::shared_ptr<Widget> recycled)>& acquire)
{
    if (!acquire) {
        slots.clear();
        return;
    }

    // Map currently active widgets by data index.
    std::unordered_map<size_t, std::shared_ptr<Widget>> keep;
    keep.reserve(slots.size());
    for (Slot& slot : slots) {
        if (!slot.widget || slot.dataIndex == static_cast<size_t>(-1)) {
            continue;
        }
        if (range.Contains(slot.dataIndex)) {
            keep.emplace(slot.dataIndex, std::move(slot.widget));
        } else {
            m_Free.push_back(std::move(slot.widget));
            ++m_RecycleCount;
        }
    }
    slots.clear();
    if (range.Empty()) {
        return;
    }
    if (slots.capacity() < range.count) {
        slots.reserve(range.count);
    }

    for (size_t i = 0; i < range.count; ++i) {
        const size_t dataIndex = range.first + i;
        Slot slot{};
        slot.dataIndex = dataIndex;

        auto it = keep.find(dataIndex);
        if (it != keep.end()) {
            slot.widget = std::move(it->second);
            keep.erase(it);
        } else {
            std::shared_ptr<Widget> recycled;
            if (!m_Free.empty()) {
                recycled = std::move(m_Free.back());
                m_Free.pop_back();
                ++m_RecycleCount;
            }
            slot.widget = acquire(dataIndex, recycled);
            ++m_AcquireCount;
            if (!recycled) {
                ++m_CreateCount;
            }
        }
        if (slot.widget) {
            slots.push_back(std::move(slot));
        }
    }

    // Any leftover keep entries (shouldn't happen) go back to the pool.
    for (auto& [idx, widget] : keep) {
        (void)idx;
        if (widget) {
            m_Free.push_back(std::move(widget));
            ++m_RecycleCount;
        }
    }
}

void WidgetRecyclePool::Clear() {
    m_Free.clear();
    ResetCounters();
}

} // namespace we::runtime::kindui
