#include "Rendering/Icons/AtlasManager.h"

#include "Icons/Assets/IconAsset.h"
#include "Rendering/IconMetrics.h"
#include "Rendering/OverlayRenderer.h"
#include "Core/Logger.h"

namespace WindEffects::Editor::UI {

namespace {

uint32_t TierIndex(uint32_t tierPx) {
    for (uint32_t i = 0; i < IconMetrics::kAtlasTierCount; ++i) {
        if (IconMetrics::kAtlasTiers[i] == tierPx) {
            return i;
        }
    }
    return UINT32_MAX;
}

} // namespace

AtlasManager::~AtlasManager() {
    Shutdown();
}

bool AtlasManager::Init(OverlayRenderer* renderer) {
    m_Renderer = renderer;
    return m_Renderer != nullptr;
}

void AtlasManager::Shutdown() {
    std::scoped_lock lock(m_Mutex);
    for (auto& tier : m_Tiers) {
        DestroyTier(tier);
    }
    for (auto& path : m_TierPaths) {
        path.clear();
    }
    m_Renderer = nullptr;
}

bool AtlasManager::LoadTierFromFile(uint32_t tierPx, const std::filesystem::path& atlasPath) {
    const auto loaded = we::runtime::icons::assets::IconAtlasReader::LoadFromFile(atlasPath);
    if (!loaded.ok) {
        HE_ERROR("[AtlasManager] Failed to load atlas: " + atlasPath.string());
        return false;
    }
    const uint32_t index = TierIndex(tierPx);
    if (index >= IconMetrics::kAtlasTierCount) {
        return false;
    }
    std::scoped_lock lock(m_Mutex);
    DestroyTier(m_Tiers[index]);
    m_TierPaths[index] = atlasPath;
    m_Tiers[index].tierPx = tierPx;
    const bool uploaded = UploadAtlasPage(
        m_Tiers[index], loaded.value.page.rgba, loaded.value.page.width, loaded.value.page.height);
    m_Tiers[index].ready = uploaded;
    return uploaded;
}

bool AtlasManager::EnsureTierLoaded(uint32_t tierPx, const std::filesystem::path& atlasPath) {
    const uint32_t index = TierIndex(tierPx);
    if (index >= IconMetrics::kAtlasTierCount) {
        return false;
    }
    {
        std::scoped_lock lock(m_Mutex);
        if (m_Tiers[index].ready) {
            return true;
        }
        m_TierPaths[index] = atlasPath;
    }
    return LoadTierFromFile(tierPx, atlasPath);
}

const AtlasGpuResource* AtlasManager::GetTier(uint32_t tierPx) const {
    const uint32_t index = TierIndex(tierPx);
    if (index >= IconMetrics::kAtlasTierCount) {
        return nullptr;
    }
    std::scoped_lock lock(m_Mutex);
    return m_Tiers[index].ready ? &m_Tiers[index] : nullptr;
}

bool AtlasManager::IsTierReady(uint32_t tierPx) const {
    return GetTier(tierPx) != nullptr;
}

void AtlasManager::WaitDeviceIdle() const {
}

bool AtlasManager::UploadAtlasPage(
    AtlasGpuResource& tier, const std::vector<uint8_t>& rgba, uint32_t width, uint32_t height) {
    if (!m_Renderer || rgba.empty() || width == 0 || height == 0) {
        return false;
    }
    tier.width = width;
    tier.height = height;
    tier.descriptorSet = m_Renderer->UploadRgbaTexture(width, height, rgba, false);
    return tier.descriptorSet != we::rhi::RHIDescriptorSetHandle::Invalid;
}

void AtlasManager::DestroyTier(AtlasGpuResource& tier) {
    if (m_Renderer && tier.descriptorSet != we::rhi::RHIDescriptorSetHandle::Invalid) {
        m_Renderer->UnregisterTexture(tier.descriptorSet);
    }
    tier = {};
}

} // namespace WindEffects::Editor::UI
