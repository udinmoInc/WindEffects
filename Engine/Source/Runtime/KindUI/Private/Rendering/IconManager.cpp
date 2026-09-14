// ==============================================================================
// WindEffects — KindUI — IconManager
// Internal implementation for the KindUI module.
//
// Individual PNG WindIcons: one decode + one GPU texture per stem/size, shared
// globally, lazy-loaded. Residency eviction via UIResourceResidency + deferred
// GPU destroy (OverlayRenderer::RetireTexture).
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Host/IconManager.h"

#include "Icons/Assets/PngLoader.h"
#include "Core/Logger.h"
#include "KindUI/Core/UIResourceResidency.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Host/OverlayRenderer.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace we::runtime::kindui {

IconManager::IconManager() = default;

IconManager::~IconManager()
{
    Shutdown();
}

bool IconManager::Init(OverlayRenderer* renderer, const std::filesystem::path& windIconsRoot)
{
    m_Renderer = renderer;
    m_WindIconsRoot = windIconsRoot;
    m_Ready = renderer != nullptr && !windIconsRoot.empty() && std::filesystem::is_directory(windIconsRoot);
    if (!m_Ready) {
        HE_ERROR("[Icons] WindIcons root not found: " + windIconsRoot.string());
        return false;
    }
    HE_INFO("[Icons] WindIcons root: " + windIconsRoot.string());
    return true;
}

void IconManager::Shutdown()
{
    std::scoped_lock lock(m_Mutex);
    for (auto& [key, texture] : m_Textures) {
        (void)key;
        DestroyTexture(texture, false);
    }
    m_Textures.clear();
    m_Renderer = nullptr;
    m_Ready = false;
}

size_t IconManager::TextureCacheEntryCount() const
{
    std::lock_guard lock(m_Mutex);
    size_t n = 0;
    for (const auto& [_, tex] : m_Textures) {
        if (tex.ready && !tex.missing) {
            ++n;
        }
    }
    return n;
}

uint64_t IconManager::EstimatedGpuBytes() const
{
    std::lock_guard lock(m_Mutex);
    uint64_t bytes = 0;
    for (const auto& [_, tex] : m_Textures) {
        if (tex.ready && !tex.missing) {
            bytes += tex.estimatedBytes;
        }
    }
    return bytes;
}

IconManagerStats IconManager::GetStats() const
{
    std::lock_guard lock(m_Mutex);
    IconManagerStats s{};
    s.cacheEntries = static_cast<uint32_t>(m_Textures.size());
    for (const auto& [_, tex] : m_Textures) {
        if (tex.missing) {
            ++s.missEntries;
        } else if (tex.ready) {
            ++s.readyEntries;
            s.estimatedGpuBytes += tex.estimatedBytes;
        }
    }
    s.resolveHits = m_ResolveHits;
    s.resolveMisses = m_ResolveMisses;
    s.loads = m_Loads;
    s.uploads = m_Uploads;
    s.evictions = m_Evictions;
    s.duplicateLoadAvoided = m_DuplicateLoadAvoided;
    return s;
}

bool IconManager::NeedsEviction() const
{
    auto& residency = UIResourceResidency::Get();
    if (!residency.ShouldRunEvictionPass()) {
        return false;
    }
    const uint64_t used = EstimatedGpuBytes();
    if (used > residency.IconGpuBudgetBytes()) {
        return true;
    }
    // Bounded idle probe (cursor window) — no full-map scan every tick.
    const uint64_t frame = residency.CurrentFrame();
    const uint32_t idle = residency.IconIdleFrames();
    std::lock_guard lock(m_Mutex);
    if (m_Textures.empty()) {
        return false;
    }
    const size_t n = m_Textures.size();
    const size_t probe = (std::min)(n, static_cast<size_t>(32));
    auto it = m_Textures.begin();
    if (m_EvictCursor < n) {
        std::advance(it, static_cast<std::ptrdiff_t>(m_EvictCursor % n));
    }
    for (size_t i = 0; i < probe; ++i) {
        if (it == m_Textures.end()) {
            it = m_Textures.begin();
        }
        const CachedTexture& tex = it->second;
        if (tex.ready && !tex.missing && !tex.loading
            && frame > tex.lastUsedFrame
            && (frame - tex.lastUsedFrame) >= idle) {
            return true;
        }
        ++it;
    }
    return false;
}

void IconManager::OnFrame(
    uint64_t /*frameNumber*/,
    OverlayRenderer* renderer,
    const std::shared_ptr<Widget>& root)
{
    auto& residency = UIResourceResidency::Get();
    if (!residency.ShouldRunEvictionPass()) {
        residency.NoteEvictionPass(false);
        return;
    }

    if (!NeedsEviction()) {
        residency.NoteEvictionPass(false);
        return;
    }

    OverlayRenderer* host = renderer ? renderer : m_Renderer;
    if (!host) {
        residency.NoteEvictionPass(false);
        return;
    }

    host->PrepareForResourceEviction(root);
    const uint64_t before = m_Evictions;
    TrimUnderBudget(
        residency.IconGpuBudgetBytes(),
        residency.IconIdleFrames(),
        residency.MaxEvictionsThisTick());
    const uint64_t evicted = m_Evictions - before;
    if (evicted > 0) {
        residency.NoteEviction(static_cast<uint32_t>(evicted));
    }
    residency.NoteEvictionPass(evicted > 0);
}

void IconManager::TrimUnderBudget(uint64_t budgetBytes, uint32_t idleFrames, uint32_t maxEvictions)
{
    auto& residency = UIResourceResidency::Get();
    const uint64_t frame = residency.CurrentFrame();
    std::scoped_lock lock(m_Mutex);
    if (m_Textures.empty() || maxEvictions == 0) {
        return;
    }

    uint64_t used = 0;
    for (const auto& [_, tex] : m_Textures) {
        if (tex.ready && !tex.missing) {
            used += tex.estimatedBytes;
        }
    }

    const bool overBudget = used > budgetBytes;
    uint32_t evicted = 0;
    const size_t n = m_Textures.size();
    size_t scanned = 0;
    auto it = m_Textures.begin();
    if (m_EvictCursor < n) {
        std::advance(it, static_cast<std::ptrdiff_t>(m_EvictCursor % n));
    }

    while (scanned < n && evicted < maxEvictions) {
        if (it == m_Textures.end()) {
            it = m_Textures.begin();
        }
        CachedTexture& tex = it->second;
        ++scanned;
        // Never evict textures touched this frame (draw / retained rebuild / pin).
        if (tex.lastUsedFrame == frame || tex.loading || !tex.ready || tex.missing) {
            ++it;
            continue;
        }
        const bool idle = frame > tex.lastUsedFrame
            && (frame - tex.lastUsedFrame) >= idleFrames;
        // Over budget: LRU among not-in-use; under budget: idle only.
        if (overBudget || idle) {
            const uint64_t bytes = tex.estimatedBytes;
            DestroyTexture(tex, true);
            ++m_Evictions;
            ++evicted;
            it = m_Textures.erase(it);
            used = used > bytes ? used - bytes : 0;
            if (overBudget && used <= budgetBytes) {
                break;
            }
            if (!overBudget) {
                // Opportunistic idle trim: one pass window, stop when under budget idle.
                break;
            }
            continue;
        }
        ++it;
    }
    const size_t remain = m_Textures.size();
    m_EvictCursor = remain == 0 ? 0 : (m_EvictCursor + scanned) % remain;
}

std::string IconManager::CacheKey(WindIconRef icon) const
{
    return std::string(icon.stem) + "_" + std::to_string(icon.sizePx);
}

std::filesystem::path IconManager::AssetPathFor(WindIconRef icon) const
{
    return m_WindIconsRoot / (CacheKey(icon) + ".png");
}

void IconManager::DestroyTexture(CachedTexture& texture, bool deferred) const
{
    if (m_Renderer && texture.descriptorSet != we::rhi::RHIDescriptorSetHandle::Invalid) {
        if (deferred) {
            m_Renderer->RetireTexture(texture.descriptorSet);
        } else {
            m_Renderer->UnregisterTexture(texture.descriptorSet);
        }
    }
    texture.descriptorSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    texture.ready = false;
    texture.loading = false;
    texture.width = 0;
    texture.height = 0;
    texture.estimatedBytes = 0;
}

void IconManager::Touch(CachedTexture& texture) const
{
    texture.lastUsedFrame = UIResourceResidency::Get().CurrentFrame();
}

IconManager::CachedTexture* IconManager::LoadTexture(WindIconRef icon) const
{
    if (!m_Renderer || !icon.IsValid()) {
        return nullptr;
    }

    auto& residency = UIResourceResidency::Get();
    const std::string key = CacheKey(icon);

    {
        std::scoped_lock lock(m_Mutex);
        auto it = m_Textures.find(key);
        if (it != m_Textures.end()) {
            if (it->second.missing) {
                Touch(it->second);
                return nullptr;
            }
            if (it->second.ready) {
                Touch(it->second);
                ++m_ResolveHits;
                residency.NoteHit();
                return &it->second;
            }
            if (it->second.loading) {
                ++m_DuplicateLoadAvoided;
                return nullptr;
            }
        }

        CachedTexture placeholder{};
        placeholder.loading = true;
        placeholder.lastUsedFrame = residency.CurrentFrame();
        auto [insertedIt, ok] = m_Textures.emplace(key, placeholder);
        if (!ok) {
            ++m_DuplicateLoadAvoided;
            return insertedIt->second.ready ? &insertedIt->second : nullptr;
        }
        ++m_Loads;
        residency.NoteLoad();
        residency.NoteMiss();
    }

    const auto path = AssetPathFor(icon);
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec) {
        std::scoped_lock lock(m_Mutex);
        auto it = m_Textures.find(key);
        if (it != m_Textures.end()) {
            it->second.loading = false;
            it->second.missing = true;
            it->second.ready = false;
            Touch(it->second);
        }
        ++m_ResolveMisses;
        return nullptr;
    }

    const auto sourceWriteTime = std::filesystem::last_write_time(path, ec);

    std::vector<uint8_t> rgba;
    uint32_t width = 0;
    uint32_t height = 0;
    if (!we::runtime::icons::LoadPngRgba(path, rgba, width, height)) {
        HE_ERROR("[Icons] Failed to decode WindIcon: " + path.string());
        std::scoped_lock lock(m_Mutex);
        auto it = m_Textures.find(key);
        if (it != m_Textures.end()) {
            it->second.loading = false;
            it->second.missing = true;
            Touch(it->second);
        }
        ++m_ResolveMisses;
        return nullptr;
    }

    const bool linearFilter = width > 32 || height > 32;
    const we::rhi::RHIDescriptorSetHandle descriptorSet =
        m_Renderer->UploadRgbaTexture(width, height, rgba, linearFilter, true);
    if (descriptorSet == we::rhi::RHIDescriptorSetHandle::Invalid) {
        HE_ERROR("[Icons] Failed to upload WindIcon: " + path.string());
        std::scoped_lock lock(m_Mutex);
        auto it = m_Textures.find(key);
        if (it != m_Textures.end()) {
            it->second.loading = false;
            it->second.missing = true;
            Touch(it->second);
        }
        ++m_ResolveMisses;
        return nullptr;
    }
    ++m_Uploads;
    residency.NoteUpload();

    std::scoped_lock lock(m_Mutex);
    auto it = m_Textures.find(key);
    if (it == m_Textures.end()) {
        if (m_Renderer) {
            m_Renderer->UnregisterTexture(descriptorSet);
        }
        return nullptr;
    }

    CachedTexture& slot = it->second;
    if (slot.ready && slot.descriptorSet != we::rhi::RHIDescriptorSetHandle::Invalid
        && slot.descriptorSet != descriptorSet) {
        if (m_Renderer) {
            m_Renderer->UnregisterTexture(descriptorSet);
        }
        ++m_DuplicateLoadAvoided;
        Touch(slot);
        ++m_ResolveHits;
        residency.NoteHit();
        return &slot;
    }

    slot.descriptorSet = descriptorSet;
    slot.width = width;
    slot.height = height;
    slot.shaderType = 4.0f;
    slot.ready = true;
    slot.loading = false;
    slot.missing = false;
    slot.estimatedBytes = static_cast<uint64_t>(width) * static_cast<uint64_t>(height) * 4ull;
    slot.sourceWriteTime = sourceWriteTime;
    Touch(slot);
    ++m_ResolveHits;
    return &slot;
}

IconDrawInfo IconManager::ResolveIcon(WindIconRef icon) const
{
    IconDrawInfo info;
    if (!m_Ready || !icon.IsValid()) {
        return info;
    }

    CachedTexture* texture = LoadTexture(icon);
    if (!texture || !texture->ready) {
        return info;
    }

    info.descriptorSet = texture->descriptorSet;
    info.uvMin[0] = 0.0f;
    info.uvMin[1] = 0.0f;
    info.uvMax[0] = 1.0f;
    info.uvMax[1] = 1.0f;
    info.shaderType = texture->shaderType;
    info.sizePx = icon.sizePx;
    info.valid = true;
    return info;
}

} // namespace we::runtime::kindui
