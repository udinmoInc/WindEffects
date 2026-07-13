#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include "Core/Geometry.h"
#include "Rendering/Icons/AtlasCache.h"
#include "Rendering/Icons/AtlasManager.h"

#include <volk.h>
#include <cstdint>
#include <filesystem>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

namespace we::runtime::renderer {
class DeviceContext;
class ResourceManager;
}

namespace WindEffects::Editor::UI {

class UiGpuUpload;

struct IconDrawInfo {
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    float uvMin[2] = {0.0f, 0.0f};
    float uvMax[2] = {1.0f, 1.0f};
    float shaderType = 0.0f;
    bool valid = false;
};

class UIFRAMEWORK_API IconManager {
public:
    IconManager();
    ~IconManager();

    bool Init(
        we::runtime::renderer::DeviceContext* context,
        we::runtime::renderer::ResourceManager* resources,
        UiGpuUpload* gpuUpload,
        VkDescriptorPool descriptorPool,
        VkDescriptorSetLayout textureLayout,
        const std::filesystem::path& metaPath,
        const std::filesystem::path& atlasRoot);

    void Shutdown();

    [[nodiscard]] IconDrawInfo ResolveIcon(
        const std::string& iconName,
        uint32_t tierPx,
        const Color& color) const;

    [[nodiscard]] IconDrawInfo ResolvePathIcon(
        const std::string& assetPath,
        uint32_t tierPx,
        const Color& color) const;

    [[nodiscard]] bool IsReady() const { return m_Ready; }

    void EnsureCompactTierLoaded();

private:
    void LoadMeta(const std::filesystem::path& metaPath);
    void PreloadAtlases();
    void RequestTierLoad(uint32_t tierPx);
    [[nodiscard]] std::filesystem::path AtlasPathForTier(uint32_t tierPx) const;
    [[nodiscard]] std::string ResolveLogicalName(const std::string& iconName) const;

    std::unique_ptr<AtlasCache> m_Cache;
    std::unique_ptr<AtlasManager> m_AtlasManager;

    std::filesystem::path m_MetaPath;
    std::filesystem::path m_AtlasRoot;
    bool m_Ready = false;

    mutable std::mutex m_LoadMutex;
    std::unordered_set<uint32_t> m_LoadedTiers;
    std::unordered_set<uint32_t> m_PendingTiers;
    std::unordered_set<uint32_t> m_FailedTiers;
    bool m_CompactTierLoadRequested = false;
};

} // namespace WindEffects::Editor::UI
