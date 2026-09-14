// ==============================================================================
// WindEffects — KindUI — VirtualList
// Canonical ListView path: only visible rows (+ overscan) are created/arranged/
// painted. Widgets are recycled while scrolling. Selection lives on the list.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/Observable.h"
#include "KindUI/UI/ListVirtualization.h"

#include <functional>
#include <unordered_set>
#include <vector>

namespace we::runtime::kindui {

/// Virtualized list (ListView). Small datasets that fit the viewport still use the
/// same path — the active window simply covers all items.
class KINDUI_API VirtualList : public Widget {
public:
    /// Create or rebind a row. When `recycled` is non-null, update that widget for
    /// `index` and return it (preferred). Otherwise create a new widget.
    using ItemFactory = std::function<std::shared_ptr<Widget>(size_t index, std::shared_ptr<Widget> recycled)>;
    /// Optional per-index height. When set, enables variable-height virtualization.
    using HeightProvider = std::function<float(size_t index)>;

    VirtualList();
    ~VirtualList() override {
        // Rows live only in m_Slots / m_Pool — never dual-owned via m_Children.
        m_ItemFactory = {};
        m_HeightProvider = {};
        m_Slots.clear();
        m_Pool.Clear();
    }

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void Tick(float deltaTime) override;
    void OnMouseWheel(const MouseEvent& event) override;
    void OnMouseDown(const MouseEvent& event) override;
    void OnKeyDown(const KeyEvent& event) override;
    [[nodiscard]] std::shared_ptr<Widget> HitTestPoint(const Point& pos, const Rect* clip = nullptr) override;
    [[nodiscard]] std::optional<Rect> GetHitTestClipRect() const override;
    [[nodiscard]] bool IsInteractiveContainer() const override { return true; }
    [[nodiscard]] bool CanReceiveMouseWheelAt(const Point& pos) const override;

    void SetItemCount(size_t count);
    void SetItemHeight(float height);
    void SetOverscan(size_t rows);
    void SetItemFactory(ItemFactory factory);
    /// Compatibility: create-only factory (no recycle rebind). Prefer the two-argument form.
    void SetItemFactory(std::function<std::shared_ptr<Widget>(size_t index)> factory);
    void SetHeightProvider(HeightProvider provider);
    void SetScrollOffset(float offset);
    void ScrollIntoView(size_t index);

    [[nodiscard]] float GetScrollOffset() const { return m_ScrollOffset; }
    [[nodiscard]] size_t GetItemCount() const { return m_ItemCount; }
    [[nodiscard]] size_t GetActiveWidgetCount() const { return m_Slots.size(); }
    [[nodiscard]] ListVisibleRange GetVisibleRange() const { return m_Range; }
    [[nodiscard]] const WidgetRecyclePool& GetRecyclePool() const { return m_Pool; }
    [[nodiscard]] WidgetRecyclePool& GetRecyclePool() { return m_Pool; }
    void ResetRecycleCounters() { m_Pool.ResetCounters(); }

    void SetSelectedIndex(size_t index, bool multi = false);
    void ClearSelection();
    [[nodiscard]] const std::unordered_set<size_t>& GetSelectedIndices() const { return m_Selected; }
    void SetOnSelectionChanged(std::function<void()> cb) { m_OnSelectionChanged = std::move(cb); }

    template <typename T>
    void BindItems(ObservableList<T>& list, std::function<std::shared_ptr<Widget>(const T&, size_t)> factory) {
        m_ItemFactory = [factory, &list](size_t index, std::shared_ptr<Widget> recycled) -> std::shared_ptr<Widget> {
            (void)recycled;
            const auto& items = list.Items();
            if (index >= items.size()) {
                return nullptr;
            }
            return factory(items[index], index);
        };
        SetItemCount(list.Items().size());
        (void)list.Subscribe([this, &list]() {
            SetItemCount(list.Items().size());
            InvalidateModel();
        });
    }

    /// Force recycle pool flush + window rebuild (model identity changed).
    void InvalidateModel();

private:
    void SyncVisibleWindow();
    void RebuildHeightPrefix();
    [[nodiscard]] float ContentHeight() const;
    [[nodiscard]] float RowOffset(size_t index) const;
    [[nodiscard]] float RowHeight(size_t index) const;
    [[nodiscard]] size_t IndexAtY(float contentY) const;

    size_t m_ItemCount = 0;
    float m_ItemHeight = 0.0f;
    float m_ScrollOffset = 0.0f;
    size_t m_Overscan = 4;
    ItemFactory m_ItemFactory;
    HeightProvider m_HeightProvider;
    std::vector<float> m_HeightPrefix;
    bool m_HeightPrefixDirty = true;

    WidgetRecyclePool m_Pool;
    std::vector<WidgetRecyclePool::Slot> m_Slots;
    ListVisibleRange m_Range{};

    std::unordered_set<size_t> m_Selected;
    size_t m_AnchorIndex = static_cast<size_t>(-1);
    std::function<void()> m_OnSelectionChanged;
};

using ListView = VirtualList;

[[nodiscard]] KINDUI_API std::shared_ptr<VirtualList> MakeVirtualList();
[[nodiscard]] KINDUI_API std::shared_ptr<ListView> MakeListView();

} // namespace we::runtime::kindui
