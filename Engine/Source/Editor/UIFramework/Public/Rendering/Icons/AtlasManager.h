#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include "Rendering/IconMetrics.h"

#include <volk.h>
#include <cstdint>
#include <filesystem>
#include <vector>
#include <mutex>
#include <string>

namespace we::runtime::renderer {
class DeviceContext;
class ResourceManager;
}

namespace WindEffects::Editor::UI {

class UiGpuUpload;

struct AtlasGpuResource {
    uint32_t tierPx = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    bool ready = false;
};

class UIFRAMEWORK_API AtlasManager {
public:
    AtlasManager() = default;
    ~AtlasManager();

    bool Init(
        we::runtime::renderer::DeviceContext* context,
        we::runtime::renderer::ResourceManager* resources,
        UiGpuUpload* gpuUpload,
        VkDescriptorPool descriptorPool,
        VkDescriptorSetLayout textureLayout);

    void Shutdown();

    bool LoadTierFromFile(uint32_t tierPx, const std::filesystem::path& atlasPath);
    bool EnsureTierLoaded(uint32_t tierPx, const std::filesystem::path& atlasPath);

    [[nodiscard]] const AtlasGpuResource* GetTier(uint32_t tierPx) const;
    [[nodiscard]] bool IsTierReady(uint32_t tierPx) const;
    void WaitDeviceIdle() const;

private:
    bool UploadAtlasPage(AtlasGpuResource& tier, const std::vector<uint8_t>& rgba, uint32_t width, uint32_t height);
    void DestroyTier(AtlasGpuResource& tier);

    we::runtime::renderer::DeviceContext* m_Context = nullptr;
    we::runtime::renderer::ResourceManager* m_Resources = nullptr;
    UiGpuUpload* m_GpuUpload = nullptr;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_TextureLayout = VK_NULL_HANDLE;

    AtlasGpuResource m_Tiers[IconMetrics::kAtlasTierCount]{};
    std::filesystem::path m_TierPaths[IconMetrics::kAtlasTierCount]{};
    mutable std::mutex m_Mutex;
};

} // namespace WindEffects::Editor::UI
