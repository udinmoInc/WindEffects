#include "KindUI/Rendering/Icons/IconManager.h"

#include "KindUI/Core/Icon.h"
#include "Core/Logger.h"
#include "Icons/Assets/IconAsset.h"
#include "Icons/Core/IconTypes.h"
#include "KindUI/Rendering/IconMetrics.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace we::runtime::kindui {

namespace {

bool IsAtlasAuditEnabled()
{
    const char* value = std::getenv("WE_ICONS_ATLAS_AUDIT");
    return value != nullptr && value[0] != '\0' && value[0] != '0';
}

std::string StemFromAssetPath(const std::string& assetPath)
{
    const std::filesystem::path path(assetPath);
    std::string stem = path.stem().string();
    if (stem == "WindEffects") {
        return "windeffects";
    }
    if (stem.starts_with("Ic_")) {
        stem = stem.substr(3);
    }
    std::transform(stem.begin(), stem.end(), stem.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return stem;
}

} // namespace

IconManager::IconManager() = default;

IconManager::~IconManager()
{
    Shutdown();
}

bool IconManager::Init(
    OverlayRenderer* renderer,
    const std::filesystem::path& metaPath,
    const std::filesystem::path& atlasRoot)
{
    m_MetaPath = metaPath;
    m_AtlasRoot = atlasRoot;

    m_Cache = std::make_unique<AtlasCache>();
    m_AtlasManager = std::make_unique<AtlasManager>();
    if (!m_AtlasManager->Init(renderer)) {
        return false;
    }

    IconRegistry::Get().InitializeDefaultIcons();
    LoadMeta(metaPath);
    PreloadAtlases();
    m_Ready = true;
    return true;
}

void IconManager::Shutdown()
{
    if (m_AtlasManager) {
        m_AtlasManager->Shutdown();
        m_AtlasManager.reset();
    }
    if (m_Cache) {
        m_Cache->Clear();
        m_Cache.reset();
    }
    m_LoadedTiers.clear();
    m_PendingTiers.clear();
    m_FailedTiers.clear();
    m_CompactTierLoadRequested = false;
    m_Ready = false;
}

void IconManager::LoadMeta(const std::filesystem::path& metaPath)
{
    we::runtime::icons::assets::IconMetaFlatList flat{};
    char errorBuf[256] = {};
    if (!we::runtime::icons::assets::LoadFlatList(metaPath.string().c_str(), flat, errorBuf, sizeof(errorBuf))) {
        HE_ERROR(
            "[Icons] Failed to load icon meta: " + metaPath.string()
            + (errorBuf[0] != '\0' ? std::string(" (") + errorBuf + ")" : std::string{}));
        we::runtime::icons::assets::FreeFlatList(flat);
        return;
    }

    std::vector<IconMetaEntryInput> inputs;
    inputs.reserve(flat.count);
    std::vector<uint32_t> tiers;
    tiers.reserve(flat.count);
    std::unordered_map<std::string, std::vector<uint32_t>> iconTiers;
    iconTiers.reserve(static_cast<size_t>(flat.count));
    for (uint32_t i = 0; i < flat.count; ++i) {
        const auto& entry = flat.entries[i];
        IconMetaEntryInput input;
        input.nameHash = entry.nameHash;
        input.name = entry.name;
        input.tierPx = entry.tierPx;
        input.uv = {entry.u0, entry.v0, entry.u1, entry.v1};
        input.flags = (entry.flags & static_cast<uint8_t>(we::runtime::icons::IconEntryFlags::FullColor)) != 0
            ? IconEntryFlags::FullColor
            : IconEntryFlags::None;
        if (entry.tierPx != 0) {
            tiers.push_back(entry.tierPx);
            if (entry.name[0] != '\0') {
                iconTiers[entry.name].push_back(entry.tierPx);
            }
        }
        inputs.push_back(std::move(input));
    }
    we::runtime::icons::assets::FreeFlatList(flat);

    // Prefer filesystem discovery (includes folder-only packs like 13/19/101/202).
    std::uint32_t* discovered = nullptr;
    std::uint32_t discoveredCount = 0;
    if (!we::runtime::icons::DiscoverAtlasTiersFlat(m_AtlasRoot.string().c_str(), discovered, discoveredCount)
        || discoveredCount == 0) {
        we::runtime::icons::FreeAtlasTiers(discovered);
        discovered = nullptr;
        discoveredCount = 0;
        (void)we::runtime::icons::DiscoverAtlasTiersFlat(
            metaPath.parent_path().string().c_str(), discovered, discoveredCount);
    }
    for (std::uint32_t i = 0; i < discoveredCount; ++i) {
        tiers.push_back(discovered[i]);
    }
    we::runtime::icons::FreeAtlasTiers(discovered);
    IconMetrics::SetRegisteredAtlasTiers(tiers);

    // Per-icon tables so folder-only packs cannot steal snaps for square UI glyphs.
    IconMetrics::ClearIconAtlasTiers();
    for (const auto& [name, nameTiers] : iconTiers) {
        IconMetrics::SetIconAtlasTiers(name, nameTiers);
    }

    m_Cache->LoadMeta(inputs);
    HE_INFO(
        "[Icons] Loaded " + std::to_string(inputs.size())
        + " icon atlas entries across " + std::to_string(IconMetrics::GetRegisteredAtlasTiers().size())
        + " tiers");
}

void IconManager::PreloadAtlases()
{
    const auto tiers = IconMetrics::GetRegisteredAtlasTiers();
    // Startup: load the primary UI atlas only. Other tiers (incl. folder packs) load on demand
    // so we don't flood the Vulkan queue with sync uploads before the first swapchain frame.
    for (const uint32_t tierPx : tiers) {
        if (tierPx != 16) {
            continue;
        }
        HE_INFO("[Icons] Preloading atlas tier " + std::to_string(tierPx));
        RequestTierLoad(tierPx);
    }
}

void IconManager::EnsureCompactTierLoaded()
{
    if (!m_AtlasManager || m_AtlasManager->IsTierReady(IconMetrics::kCompactTierPx)) {
        return;
    }

    {
        std::scoped_lock lock(m_LoadMutex);
        if (m_FailedTiers.contains(IconMetrics::kCompactTierPx)
            || m_PendingTiers.contains(IconMetrics::kCompactTierPx)
            || m_LoadedTiers.contains(IconMetrics::kCompactTierPx)) {
            return;
        }
        m_CompactTierLoadRequested = true;
    }

    m_AtlasManager->WaitDeviceIdle();
    HE_INFO("[Icons] Loading compact atlas tier " + std::to_string(IconMetrics::kCompactTierPx));
    RequestTierLoad(IconMetrics::kCompactTierPx);
}

void IconManager::RequestTierLoad(const uint32_t tierPx)
{
    {
        std::scoped_lock lock(m_LoadMutex);
        if (m_LoadedTiers.contains(tierPx) || m_PendingTiers.contains(tierPx) || m_FailedTiers.contains(tierPx)) {
            return;
        }
        m_PendingTiers.insert(tierPx);
    }

    const auto atlasPath = AtlasPathForTier(tierPx);
    if (!std::filesystem::exists(atlasPath)) {
        HE_ERROR("[Icons] Missing atlas tier asset: " + atlasPath.string());
        std::scoped_lock lock(m_LoadMutex);
        m_PendingTiers.erase(tierPx);
        m_FailedTiers.insert(tierPx);
        return;
    }

    const bool uploaded = m_AtlasManager->LoadTierFromFile(tierPx, atlasPath);

    std::scoped_lock lock(m_LoadMutex);
    m_PendingTiers.erase(tierPx);
    if (uploaded) {
        m_LoadedTiers.insert(tierPx);
        HE_INFO("[Icons] Atlas tier ready: " + std::to_string(tierPx));
    } else {
        m_FailedTiers.insert(tierPx);
        HE_ERROR("[Icons] Failed to upload atlas tier: " + atlasPath.string());
    }
}

std::filesystem::path IconManager::AtlasPathForTier(const uint32_t tierPx) const
{
    return m_AtlasRoot / ("ui_Atlas_" + std::to_string(tierPx) + ".weiconatlas");
}

std::string IconManager::ResolveLogicalName(const std::string& iconName) const
{
    return Icons::ResolveLucideName(iconName);
}

IconDrawInfo IconManager::ResolveIcon(
    const std::string& iconName,
    const uint32_t requestedTierPx,
    const Color& color) const
{
    (void)color;
    IconDrawInfo info;
    if (!m_Ready || !m_Cache || !m_AtlasManager) {
        return info;
    }

    const std::string logicalName = ResolveLogicalName(iconName);
    const uint32_t tierPx = IconMetrics::TierPxForIcon(
        logicalName,
        static_cast<float>(requestedTierPx));
    const CachedIconEntry* entry = m_Cache->FindWithTierFallback(logicalName, tierPx);
    if (!entry) {
        static thread_local std::unordered_set<std::string> s_ReportedMissing;
        const std::string key = logicalName + "_" + std::to_string(tierPx);
        if (s_ReportedMissing.insert(key).second) {
            HE_ERROR("[Icons] Icon not found in atlas meta: " + logicalName + " @" + std::to_string(tierPx));
        }
        return info;
    }

    const AtlasGpuResource* atlas = m_AtlasManager->GetTier(entry->tierPx);
    const CachedIconEntry* drawEntry = entry;

    if (!atlas) {
        const_cast<IconManager*>(this)->RequestTierLoad(entry->tierPx);
        atlas = m_AtlasManager->GetTier(entry->tierPx);
    }

    if (!atlas) {
        return info;
    }

    info.descriptorSet = atlas->descriptorSet;
    info.uvMin[0] = drawEntry->uv.u0;
    info.uvMin[1] = drawEntry->uv.v0;
    info.uvMax[0] = drawEntry->uv.u1;
    info.uvMax[1] = drawEntry->uv.v1;
    info.shaderType = HasIconFlag(drawEntry->flags, IconEntryFlags::FullColor) ? 4.0f : 0.0f;
    info.valid = true;

    if (IsAtlasAuditEnabled()) {
        static thread_local std::unordered_set<std::string> s_AuditLogged;
        const std::string auditKey = logicalName + "_" + std::to_string(tierPx);
        if (s_AuditLogged.insert(auditKey).second) {
            HE_INFO(
                "[Icons][Audit] name=" + logicalName
                + " tier=" + std::to_string(tierPx)
                + " uv=(" + std::to_string(info.uvMin[0]) + "," + std::to_string(info.uvMin[1])
                + ")-(" + std::to_string(info.uvMax[0]) + "," + std::to_string(info.uvMax[1]) + ")"
                + " atlas=" + std::to_string(atlas->width) + "x" + std::to_string(atlas->height)
                + " descriptor=0x" + std::to_string(static_cast<uint64_t>(info.descriptorSet))
                + " format=R8G8B8A8_UNORM"
                + " shaderType=" + std::to_string(info.shaderType)
                + " fullColor=" + (HasIconFlag(drawEntry->flags, IconEntryFlags::FullColor) ? "yes" : "no"));
        }
    }

    return info;
}

IconDrawInfo IconManager::ResolvePathIcon(
    const std::string& assetPath,
    const uint32_t requestedTierPx,
    const Color& color) const
{
    return ResolveIcon(StemFromAssetPath(assetPath), requestedTierPx, color);
}

} // namespace we::runtime::kindui
