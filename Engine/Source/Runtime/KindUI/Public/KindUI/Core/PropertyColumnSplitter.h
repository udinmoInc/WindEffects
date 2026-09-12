// ==============================================================================
// WindEffects — KindUI — PropertyColumnSplitter
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Geometry.h"

#include <string>
#include <string_view>

namespace we::runtime::kindui {

/// Manages draggable column ratio state, bounds clamping, hit testing, and persistence
/// for two-column property tree views across KindUI and Editor panels.
class KINDUI_API PropertyColumnSplitterState {
public:
    explicit PropertyColumnSplitterState(std::string key = "Default", float defaultRatio = 0.40f);

    [[nodiscard]] float GetRatio() const noexcept { return m_Ratio; }
    void SetRatio(float ratio);

    [[nodiscard]] const std::string& GetKey() const noexcept { return m_Key; }

    [[nodiscard]] bool IsHovered() const noexcept { return m_IsHovered; }
    void SetHovered(bool hovered) noexcept { m_IsHovered = hovered; }

    [[nodiscard]] bool IsDragging() const noexcept { return m_IsDragging; }

    /// Calculate hit box around the column divider line inside a given row or viewport bounds.
    [[nodiscard]] Rect GetSplitterHitRect(const Rect& bounds, float columnDividerX, float hitPadding = 6.0f) const;

    [[nodiscard]] bool HitTest(const Point& pos, const Rect& bounds, float columnDividerX, float hitPadding = 6.0f)
        const;

    /// Update mouse state and calculate ratio updates during drag.
    /// Returns true if layout/paint invalidation is needed.
    bool OnMouseDown(const Point& pos, const Rect& bounds, float columnDividerX, float hitPadding = 6.0f);
    bool OnMouseMove(const Point& pos, const Rect& bounds, float columnDividerX = 0.0f, float hitPadding = 6.0f);
    bool OnMouseUp(const Point& pos, const Rect& bounds = {}, float columnDividerX = 0.0f, float hitPadding = 6.0f);

    /// System cursor management (SizeWE <-> Arrow)
    void ApplyResizeCursor(bool active) const;

    /// Clamps the ratio given row width and padding/actions parameters to enforce min column widths.
    [[nodiscard]] float ClampRatio(float rawRatio, float availableWidth, float minLabelW = 80.0f, float minValueW =
        80.0f) const;

    /// Save state to global persistence registry.
    void Save();
    /// Load state from global persistence registry.
    void Load();

private:
    std::string m_Key;
    float m_Ratio = 0.40f;
    bool m_IsHovered = false;
    bool m_IsDragging = false;
    float m_DragStartX = 0.0f;
    float m_StartRatio = 0.40f;
};

/// Global in-memory & session persistence registry for splitter ratios by key.
class KINDUI_API PropertyColumnSplitterRegistry {
public:
    static void SetRatio(std::string_view key, float ratio);
    [[nodiscard]] static float GetRatio(std::string_view key, float defaultRatio = 0.40f);
};

/// Global registry for proportional aspect lock state by property path.
class KINDUI_API PropertyAspectLockRegistry {
public:
    static void SetLocked(std::string_view path, bool locked);
    [[nodiscard]] static bool IsLocked(std::string_view path, bool defaultLocked = true);
};

} // namespace we::runtime::kindui
