// ==============================================================================
// WindEffects — KindUI — IconManager
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include "KindUI/Core/Types.h"
#include "KindUI/Core/WindIcon.h"

#include "RHI/Types.h"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace we::runtime::kindui {

class OverlayRenderer;
class Widget;

struct IconDrawInfo {
    we::rhi::RHIDescriptorSetHandle descriptorSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    float uvMin[2] = {0.0f, 0.0f};
    float uvMax[2] = {1.0f, 1.0f};
    float shaderType = 4.0f; // WindIcons are authored full-color (no mono tint)
    uint32_t sizePx = 0;
    bool valid = false;
};

struct IconManagerStats {
    uint32_t cacheEntries = 0;
    uint32_t readyEntries = 0;
    uint32_t missEntries = 0;
    uint64_t estimatedGpuBytes = 0;
    uint64_t resolveHits = 0;
    uint64_t resolveMisses = 0;
    uint64_t loads = 0;
    uint64_t uploads = 0;
    uint64_t evictions = 0;
    uint64_t duplicateLoadAvoided = 0;
};

class KINDUI_API IconManager {
public:
    IconManager();
    ~IconManager();

    bool Init(OverlayRenderer* renderer, const std::filesystem::path& windIconsRoot);
    void Shutdown();

    /// Lazy resolve: loads PNG + uploads GPU texture on first use, then reuses globally.
    [[nodiscard]] IconDrawInfo ResolveIcon(WindIconRef icon) const;

    /// Residency tick: bounded eviction under UIResourceResidency budgets.
    /// Safe path: OverlayRenderer::PrepareForResourceEviction then deferred RetireTexture.
    void OnFrame(uint64_t frameNumber, OverlayRenderer* renderer, const std::shared_ptr<Widget>& root);

    [[nodiscard]] bool NeedsEviction() const;

    /// Release idle GPU textures (bounded). Prefer OnFrame which prepares caches first.
    void TrimUnderBudget(uint64_t budgetBytes, uint32_t idleFrames, uint32_t maxEvictions);

    [[nodiscard]] bool IsReady() const { return m_Ready; }
    [[nodiscard]] size_t TextureCacheEntryCount() const;
    [[nodiscard]] uint64_t EstimatedGpuBytes() const;
    [[nodiscard]] IconManagerStats GetStats() const;

private:
    struct CachedTexture {
        we::rhi::RHIDescriptorSetHandle descriptorSet = we::rhi::RHIDescriptorSetHandle::Invalid;
        uint32_t width = 0;
        uint32_t height = 0;
        float shaderType = 4.0f;
        bool ready = false;
        bool loading = false;
        bool missing = false;
        uint64_t lastUsedFrame = 0;
        uint64_t estimatedBytes = 0;
        std::filesystem::file_time_type sourceWriteTime{};
    };

    [[nodiscard]] std::filesystem::path AssetPathFor(WindIconRef icon) const;
    [[nodiscard]] std::string CacheKey(WindIconRef icon) const;
    CachedTexture* LoadTexture(WindIconRef icon) const;
    void DestroyTexture(CachedTexture& texture, bool deferred) const;
    void Touch(CachedTexture& texture) const;

    OverlayRenderer* m_Renderer = nullptr;
    std::filesystem::path m_WindIconsRoot;
    bool m_Ready = false;
    mutable size_t m_EvictCursor = 0;

    mutable std::mutex m_Mutex;
    mutable std::unordered_map<std::string, CachedTexture> m_Textures;
    mutable uint64_t m_ResolveHits = 0;
    mutable uint64_t m_ResolveMisses = 0;
    mutable uint64_t m_Loads = 0;
    mutable uint64_t m_Uploads = 0;
    mutable uint64_t m_Evictions = 0;
    mutable uint64_t m_DuplicateLoadAvoided = 0;
};

} // namespace we::runtime::kindui
