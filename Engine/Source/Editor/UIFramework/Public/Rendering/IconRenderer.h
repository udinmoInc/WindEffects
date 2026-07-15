#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "RHI/Types.h"

#include <memory>
#include <unordered_map>
#include <vector>
#include "Core/Geometry.h"
#include "Core/Icon.h"
#include "Rendering/Icons/IconManager.h"

namespace we::runtime::renderer {
class DeviceContext;
class ResourceManager;
}

namespace WindEffects::Editor::UI {

class UiGpuUpload;

struct IconTexture {
    // TODO(migr to VulkanRHI): opaque native handles
    uint64_t image = 0;
    uint64_t memory = 0;
    uint64_t view = 0;
    uint64_t sampler = 0;
    we::rhi::RHIDescriptorSetHandle descriptorSet = we::rhi::RHIDescriptorSetHandle::Invalid;
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
              uint64_t descriptorPool,
              uint64_t textureLayout);
    void Shutdown();

    void SetIconManager(IconManager* iconManager) { m_IconManager = iconManager; }

    [[nodiscard]] IconDrawInfo GetLucideIconDrawInfo(
        const std::string& iconName,
        uint32_t size,
        const Color& color,
        float strokeWidth = 0.0f) const;

    we::rhi::RHIDescriptorSetHandle GetLucideIcon(const std::string& iconName, uint32_t size, const Color& color, float strokeWidth = 0.0f);
    we::rhi::RHIDescriptorSetHandle GetIcon(const std::string& iconName, uint32_t size);
    we::rhi::RHIDescriptorSetHandle CreateTextureFromBitmap(const std::vector<uint8_t>& bitmap, uint32_t width, uint32_t height);

    void ClearCache();

private:
    bool CreateTexture(const std::vector<uint8_t>& bitmap, uint32_t width, uint32_t height, IconTexture& outTexture);
    void DestroyTexture(IconTexture& texture);

    we::runtime::renderer::DeviceContext* m_Context = nullptr;
    we::runtime::renderer::ResourceManager* m_Resources = nullptr;
    UiGpuUpload* m_GpuUpload = nullptr;
    uint64_t m_DescriptorPool = 0;
    uint64_t m_TextureLayout = 0;
    IconManager* m_IconManager = nullptr;

    std::unordered_map<std::string, IconTexture> m_ThumbnailCache;
    Color m_DefaultColor = Color::White();
};

} // namespace WindEffects::Editor::UI
