// ==============================================================================
// WindEffects — KindUI — UIResourceResidency
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Core/UIResourceResidency.h"

#include <algorithm>
#include <cstdlib>

namespace we::runtime::kindui {
namespace {

uint64_t ParseBytesEnv(const char* name, uint64_t fallback) {
    const char* v = std::getenv(name);
    if (!v || !v[0]) {
        return fallback;
    }
    char* end = nullptr;
    const unsigned long long n = std::strtoull(v, &end, 10);
    if (end == v) {
        return fallback;
    }
    uint64_t bytes = static_cast<uint64_t>(n);
    if (end && (*end == 'M' || *end == 'm')) {
        bytes *= 1024ull * 1024ull;
    } else if (end && (*end == 'K' || *end == 'k')) {
        bytes *= 1024ull;
    }
    return bytes > 0 ? bytes : fallback;
}

uint32_t ParseU32Env(const char* name, uint32_t fallback) {
    const char* v = std::getenv(name);
    if (!v || !v[0]) {
        return fallback;
    }
    char* end = nullptr;
    const unsigned long n = std::strtoul(v, &end, 10);
    if (end == v || n == 0) {
        return fallback;
    }
    return static_cast<uint32_t>(n);
}

} // namespace

UIResourceResidency& UIResourceResidency::Get() {
    static UIResourceResidency instance;
    return instance;
}

void UIResourceResidency::LoadBudgetsFromEnv() {
    m_Budgets.iconGpuBudgetBytes = ParseBytesEnv("WE_UI_ICON_BUDGET", m_Budgets.iconGpuBudgetBytes);
    m_Budgets.textGeometryBudgetBytes = ParseBytesEnv("WE_UI_TEXT_GEOM_BUDGET", m_Budgets.textGeometryBudgetBytes);
    m_Budgets.glyphEntryBudget = ParseU32Env("WE_UI_GLYPH_BUDGET", m_Budgets.glyphEntryBudget);
    m_Budgets.iconIdleFramesBeforeEvictable = ParseU32Env("WE_UI_ICON_IDLE_FRAMES", m_Budgets.iconIdleFramesBeforeEvictable);
    m_Budgets.textGeomIdleFramesBeforeEvictable = ParseU32Env("WE_UI_TEXT_GEOM_IDLE_FRAMES", m_Budgets.textGeomIdleFramesBeforeEvictable);
    m_Budgets.maxEvictionsPerTick = ParseU32Env("WE_UI_RESIDENCY_MAX_EVICT", m_Budgets.maxEvictionsPerTick);
    m_Budgets.tickIntervalFrames = ParseU32Env("WE_UI_RESIDENCY_TICK", m_Budgets.tickIntervalFrames);
    m_Budgets.gpuDeferFrames = ParseU32Env("WE_UI_RESIDENCY_GPU_DEFER", m_Budgets.gpuDeferFrames);
    m_BudgetsLoaded = true;
}

void UIResourceResidency::BeginFrame(uint64_t frameNumber, uint32_t framesInFlight) {
    if (!m_BudgetsLoaded) {
        LoadBudgetsFromEnv();
    }
    m_Frame = frameNumber;
    m_Budgets.gpuDeferFrames = (std::max)(m_Budgets.gpuDeferFrames, framesInFlight + 1u);
    m_EvictionPassNotedThisFrame = false;
    if (m_Budgets.tickIntervalFrames == 0) {
        m_EvictionTickThisFrame = true;
    } else if (m_Frame == 0) {
        m_EvictionTickThisFrame = false;
    } else {
        m_EvictionTickThisFrame =
            (m_Frame - m_LastEvictionPassFrame) >= m_Budgets.tickIntervalFrames;
    }
}

bool UIResourceResidency::ShouldRunEvictionPass() const {
    return m_EvictionTickThisFrame;
}

void UIResourceResidency::NoteEvictionPass(bool performed) {
    // Icon + text both tick on the same armed frame; only advance the interval once.
    if (!m_EvictionPassNotedThisFrame) {
        m_LastEvictionPassFrame = m_Frame;
        m_EvictionPassNotedThisFrame = true;
        if (performed) {
            ++m_Stats.evictionPasses;
        } else {
            ++m_Stats.skippedEvictionPasses;
        }
    } else if (performed) {
        // Promote a prior skip to a real pass if a later owner evicted.
        if (m_Stats.skippedEvictionPasses > 0) {
            --m_Stats.skippedEvictionPasses;
        }
        ++m_Stats.evictionPasses;
    }
}

void UIResourceResidency::PinThroughGpuHorizon() {
    m_PinHorizon = m_Frame + m_Budgets.gpuDeferFrames;
}

void UIResourceResidency::NoteHit() { ++m_Stats.cacheHits; }
void UIResourceResidency::NoteMiss() { ++m_Stats.cacheMisses; }
void UIResourceResidency::NoteLoad() { ++m_Stats.loads; }
void UIResourceResidency::NoteUpload() { ++m_Stats.uploads; }

void UIResourceResidency::NoteEviction(uint32_t count) {
    m_Stats.evictions += count;
}

void UIResourceResidency::NoteDeferredRelease(uint32_t count) {
    m_Stats.deferredReleases += count;
}

void UIResourceResidency::SetResidentSnapshot(
    uint32_t iconCount,
    uint64_t iconGpuBytes,
    uint32_t textGeomCount,
    uint64_t textGeomBytes,
    uint32_t glyphCount,
    uint64_t atlasCpuBytes)
{
    m_Stats.residentIconCount = iconCount;
    m_Stats.residentIconGpuBytes = iconGpuBytes;
    m_Stats.residentTextGeomCount = textGeomCount;
    m_Stats.residentTextGeomCpuBytes = textGeomBytes;
    m_Stats.residentGlyphCount = glyphCount;
    m_Stats.residentAtlasCpuBytes = atlasCpuBytes;
}

void UIResourceResidency::ResetStats() {
    const auto snap = m_Stats;
    m_Stats = {};
    // Keep last residency snapshot visible across reset windows.
    m_Stats.residentIconCount = snap.residentIconCount;
    m_Stats.residentIconGpuBytes = snap.residentIconGpuBytes;
    m_Stats.residentTextGeomCount = snap.residentTextGeomCount;
    m_Stats.residentTextGeomCpuBytes = snap.residentTextGeomCpuBytes;
    m_Stats.residentGlyphCount = snap.residentGlyphCount;
    m_Stats.residentAtlasCpuBytes = snap.residentAtlasCpuBytes;
}

} // namespace we::runtime::kindui
