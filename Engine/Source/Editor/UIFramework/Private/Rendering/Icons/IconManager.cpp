#include "Rendering/Icons/IconManager.h"

#include "Core/Icon.h"
#include "Core/Logger.h"
#include "Icons/Assets/IconAsset.h"
#include "Rendering/IconMetrics.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <unordered_set>

namespace WindEffects::Editor::UI {

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
    we::runtime::renderer::DeviceContext* context,
    we::runtime::renderer::ResourceManager* resources,
    UiGpuUpload* gpuUpload,
    VkDescriptorPool descriptorPool,
    VkDescriptorSetLayout textureLayout,
    const std::filesystem::path& metaPath,
    const std::filesystem::path& atlasRoot)
{
    m_MetaPath = metaPath;
    m_AtlasRoot = atlasRoot;

    m_Cache = std::make_unique<AtlasCache>();
    m_AtlasManager = std::make_unique<AtlasManager>();
    if (!m_AtlasManager->Init(context, resources, gpuUpload, descriptorPool, textureLayout)) {
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
    m_Ready = false;
}

void IconManager::LoadMeta(const std::filesystem::path& metaPath)
{
    const auto loaded = we::runtime::icons::assets::IconMetaReader::LoadFromFile(metaPath);
    if (!loaded.ok) {
        HE_ERROR("[Icons] Failed to load icon meta: " + metaPath.string() + " (" + loaded.error.message + ")");
        return;
    }
    std::vector<IconMetaEntryInput> inputs;
    inputs.reserve(loaded.value.entries.size());
    for (const auto& entry : loaded.value.entries) {
        IconMetaEntryInput input;
        input.nameHash = entry.nameHash;
        input.name = entry.name;
        input.tierPx = entry.tierPx;
        input.uv = {entry.uv.u0, entry.uv.v0, entry.uv.u1, entry.uv.v1};
        input.flags = we::runtime::icons::HasFlag(entry.flags, we::runtime::icons::IconEntryFlags::FullColor)
            ? IconEntryFlags::FullColor
            : IconEntryFlags::None;
        inputs.push_back(std::move(input));
    }
    m_Cache->LoadMeta(inputs);
    HE_INFO("[Icons] Loaded " + std::to_string(loaded.value.entries.size()) + " icon atlas entries");
}

void IconManager::PreloadAtlases()
{
    for (const uint32_t tierPx : IconMetrics::kAtlasTiers) {
        RequestTierLoad(tierPx);
    }
}

void IconManager::RequestTierLoad(const uint32_t tierPx)
{
    std::scoped_lock lock(m_LoadMutex);
    if (m_LoadedTiers.contains(tierPx) || m_PendingTiers.contains(tierPx)) {
        return;
    }
    m_PendingTiers.insert(tierPx);

    const auto atlasPath = AtlasPathForTier(tierPx);
    if (!std::filesystem::exists(atlasPath)) {
        HE_ERROR("[Icons] Missing atlas tier asset: " + atlasPath.string());
        m_PendingTiers.erase(tierPx);
        return;
    }

    if (m_AtlasManager->LoadTierFromFile(tierPx, atlasPath)) {
        m_LoadedTiers.insert(tierPx);
    } else {
        HE_ERROR("[Icons] Failed to upload atlas tier: " + atlasPath.string());
    }
    m_PendingTiers.erase(tierPx);
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

    const uint32_t tierPx = IconMetrics::SnapToAtlasTier(requestedTierPx);
    const std::string logicalName = ResolveLogicalName(iconName);
    const CachedIconEntry* entry = m_Cache->Find(logicalName, tierPx);
    if (!entry) {
        static thread_local std::unordered_set<std::string> s_ReportedMissing;
        const std::string key = logicalName + "_" + std::to_string(tierPx);
        if (s_ReportedMissing.insert(key).second) {
            HE_ERROR("[Icons] Icon not found in atlas meta: " + logicalName + " @" + std::to_string(tierPx));
        }
        return info;
    }

    const AtlasGpuResource* atlas = m_AtlasManager->GetTier(tierPx);
    if (!atlas) {
        const_cast<IconManager*>(this)->RequestTierLoad(tierPx);
        atlas = m_AtlasManager->GetTier(tierPx);
        if (!atlas) {
            return info;
        }
    }

    info.descriptorSet = atlas->descriptorSet;
    info.uvMin[0] = entry->uv.u0;
    info.uvMin[1] = entry->uv.v0;
    info.uvMax[0] = entry->uv.u1;
    info.uvMax[1] = entry->uv.v1;
    info.shaderType = HasIconFlag(entry->flags, IconEntryFlags::FullColor) ? 4.0f : 0.0f;
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
                + " descriptor=0x" + std::to_string(reinterpret_cast<uintptr_t>(info.descriptorSet))
                + " format=VK_FORMAT_R8G8B8A8_UNORM"
                + " shaderType=" + std::to_string(info.shaderType)
                + " fullColor=" + (HasIconFlag(entry->flags, IconEntryFlags::FullColor) ? "yes" : "no"));
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

} // namespace WindEffects::Editor::UI
