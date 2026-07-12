#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include <volk.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include "Core/Geometry.h"
#include "Core/Icon.h"
#include "Rendering/Icons/IconManager.h"

#include "Core/DeviceContext.h"
#include "Rendering/UiGpuUpload.h"

namespace we::runtime::renderer {
class ResourceManager;
}

namespace WindEffects::Editor::UI {

struct IconTexture {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
};

class UIFRAMEWORK_API IconRenderer {
public:
    IconRenderer();
    ~IconRenderer();

    bool Init(we::runtime::renderer::DeviceContext* context,
              we::runtime::renderer::ResourceManager* resources,
              UiGpuUpload* gpuUpload,
              VkDescriptorPool descriptorPool,
              VkDescriptorSetLayout textureLayout);
    void Shutdown();

    void SetIconManager(IconManager* iconManager) { m_IconManager = iconManager; }

    [[nodiscard]] IconDrawInfo GetLucideIconDrawInfo(
        const std::string& iconName,
        uint32_t size,
        const Color& color,
        float strokeWidth = 0.0f) const;

    VkDescriptorSet GetLucideIcon(const std::string& iconName, uint32_t size, const Color& color, float strokeWidth = 0.0f);
    VkDescriptorSet GetIcon(const std::string& iconName, uint32_t size);
    VkDescriptorSet CreateTextureFromBitmap(const std::vector<uint8_t>& bitmap, uint32_t width, uint32_t height);

    void ClearCache();

private:
    bool CreateTexture(const std::vector<uint8_t>& bitmap, uint32_t width, uint32_t height, IconTexture& outTexture);
    void DestroyTexture(IconTexture& texture);

    we::runtime::renderer::DeviceContext* m_Context = nullptr;
    we::runtime::renderer::ResourceManager* m_Resources = nullptr;
    UiGpuUpload* m_GpuUpload = nullptr;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_TextureLayout = VK_NULL_HANDLE;
    IconManager* m_IconManager = nullptr;

    std::unordered_map<std::string, IconTexture> m_ThumbnailCache;
    Color m_DefaultColor = Color::White();
};

} // namespace WindEffects::Editor::UI
