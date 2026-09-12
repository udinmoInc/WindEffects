// ==============================================================================
// WindEffects — KindUI — PropertyColumnSplitter
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Core/PropertyColumnSplitter.h"
#include "Platform/Platform.h"
#include "Platform/Types.h"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <unordered_map>

namespace we::runtime::kindui {

namespace {
std::unordered_map<std::string, float>& GetRegistryMap() {
    static std::unordered_map<std::string, float> s_Map;
    return s_Map;
}
std::mutex s_RegistryMutex;
} // namespace

void PropertyColumnSplitterRegistry::SetRatio(std::string_view key, float ratio) {
    std::lock_guard<std::mutex> lock(s_RegistryMutex);
    GetRegistryMap()[std::string(key)] = ratio;
}

float PropertyColumnSplitterRegistry::GetRatio(std::string_view key, float defaultRatio) {
    std::lock_guard<std::mutex> lock(s_RegistryMutex);
    auto& map = GetRegistryMap();
    auto it = map.find(std::string(key));
    if (it != map.end()) {
        return it->second;
    }
    return defaultRatio;
}

PropertyColumnSplitterState::PropertyColumnSplitterState(std::string key, float defaultRatio)
    : m_Key(std::move(key)), m_Ratio(defaultRatio), m_StartRatio(defaultRatio) {
    Load();
}

void PropertyColumnSplitterState::SetRatio(float ratio) {
    m_Ratio = std::clamp(ratio, 0.15f, 0.85f);
}

void PropertyColumnSplitterState::ApplyResizeCursor(bool active) const {
    auto& platform = we::platform::Platform::Get();
    if (active) {
        platform.SetSystemCursor(we::platform::SystemCursor::SizeWE);
    } else {
        platform.SetSystemCursor(we::platform::SystemCursor::Arrow);
    }
}

Rect PropertyColumnSplitterState::GetSplitterHitRect(const Rect& bounds, float columnDividerX, float hitPadding) const {
    return Rect{
        columnDividerX - hitPadding,
        bounds.y,
        hitPadding * 2.0f,
        bounds.height
    };
}

bool PropertyColumnSplitterState::HitTest(const Point& pos, const Rect& bounds, float columnDividerX, float hitPadding)
    const {
    const Rect hitRect = GetSplitterHitRect(bounds, columnDividerX, hitPadding);
    return hitRect.Contains(pos);
}

float PropertyColumnSplitterState::ClampRatio(float rawRatio, float availableWidth, float minLabelW, float minValueW)
    const {
    if (availableWidth <= 0.0f) {
        return rawRatio;
    }
    const float minRatio = minLabelW / availableWidth;
    const float maxRatio = std::max(minRatio, 1.0f - (minValueW / availableWidth));
    return std::clamp(rawRatio, minRatio, maxRatio);
}

bool PropertyColumnSplitterState::OnMouseDown(const Point& pos, const Rect& bounds, float columnDividerX,
    float hitPadding) {
    if (HitTest(pos, bounds, columnDividerX, hitPadding)) {
        m_IsDragging = true;
        m_DragStartX = pos.x;
        m_StartRatio = m_Ratio;
        ApplyResizeCursor(true);
        return true;
    }
    return false;
}

bool PropertyColumnSplitterState::OnMouseMove(const Point& pos, const Rect& bounds, float columnDividerX,
    float hitPadding) {
    bool stateChanged = false;
    const bool isHit = HitTest(pos, bounds, columnDividerX, hitPadding);
    const bool wasHovered = m_IsHovered;
    m_IsHovered = isHit;

    if (m_IsDragging || m_IsHovered) {
        ApplyResizeCursor(true);
    } else if (wasHovered && !m_IsDragging) {
        ApplyResizeCursor(false);
    }

    if (wasHovered != m_IsHovered) {
        stateChanged = true;
    }

    if (m_IsDragging) {
        if (bounds.width > 0.0f) {
            const float deltaX = pos.x - m_DragStartX;
            const float deltaRatio = deltaX / bounds.width;
            const float newRatio = std::clamp(m_StartRatio + deltaRatio, 0.15f, 0.85f);
            if (std::abs(newRatio - m_Ratio) > 0.0001f) {
                m_Ratio = newRatio;
                Save();
                stateChanged = true;
            }
        }
    }

    return stateChanged;
}

bool PropertyColumnSplitterState::OnMouseUp(const Point& pos, const Rect& bounds, float columnDividerX,
    float hitPadding) {
    if (m_IsDragging) {
        m_IsDragging = false;
        Save();
        const bool isHit = HitTest(pos, bounds, columnDividerX, hitPadding);
        m_IsHovered = isHit;
        if (!isHit) {
            ApplyResizeCursor(false);
        }
        return true;
    }
    return false;
}

void PropertyColumnSplitterState::Save() {
    PropertyColumnSplitterRegistry::SetRatio(m_Key, m_Ratio);
}

void PropertyColumnSplitterState::Load() {
    m_Ratio = PropertyColumnSplitterRegistry::GetRatio(m_Key, m_Ratio);
}

namespace {
std::unordered_map<std::string, bool>& GetLockRegistryMap() {
    static std::unordered_map<std::string, bool> s_LockMap;
    return s_LockMap;
}
std::mutex s_LockRegistryMutex;
} // namespace

void PropertyAspectLockRegistry::SetLocked(std::string_view path, bool locked) {
    std::lock_guard<std::mutex> lock(s_LockRegistryMutex);
    GetLockRegistryMap()[std::string(path)] = locked;
}

bool PropertyAspectLockRegistry::IsLocked(std::string_view path, bool defaultLocked) {
    std::lock_guard<std::mutex> lock(s_LockRegistryMutex);
    auto& map = GetLockRegistryMap();
    auto it = map.find(std::string(path));
    if (it != map.end()) {
        return it->second;
    }
    return defaultLocked;
}

} // namespace we::runtime::kindui
