#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <memory>
#include <unordered_map>
#include <vector>
#include "WindEffects/Runtime/UI/Core/Geometry.h"
#include "WindEffects/Runtime/UI/Core/Icon.h"
#include "WindEffects/Runtime/UI/Rendering/Icons/IconManager.h"

namespace WindEffects::Editor::UI {

class UiGpuUpload;

struct IconTexture {
    we::rhi::RHITextureHandle texture = we::rhi::RHITextureHandle::Invalid;
    we::rhi::RHITextureViewHandle view = we::rhi::RHITextureViewHandle::Invalid;
    we::rhi::RHISamplerHandle sampler = we::rhi::RHISamplerHandle::Invalid;
    we::rhi::RHIDescriptorSetHandle descriptorSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    uint32_t width = 0;
    uint32_t height = 0;
};

class UI_API IconRenderer {
public:
    IconRenderer();
    ~IconRenderer();

    bool Init(we::rhi::IRHIDevice* device, UiGpuUpload* gpuUpload);
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

    we::rhi::IRHIDevice* m_Device = nullptr;
    UiGpuUpload* m_GpuUpload = nullptr;
    IconManager* m_IconManager = nullptr;

    std::unordered_map<std::string, IconTexture> m_ThumbnailCache;
    Color m_DefaultColor = Color::White();
};

} // namespace WindEffects::Editor::UI
