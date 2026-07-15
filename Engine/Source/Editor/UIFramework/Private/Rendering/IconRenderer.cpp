#include "Rendering/IconRenderer.h"

#include "Rendering/IconMetrics.h"
#include "Core/Logger.h"

namespace WindEffects::Editor::UI {

IconRenderer::IconRenderer() = default;

IconRenderer::~IconRenderer() {
    Shutdown();
}

bool IconRenderer::Init(we::rhi::IRHIDevice* device, UiGpuUpload* gpuUpload) {
    m_Device = device;
    m_GpuUpload = gpuUpload;
    return true;
}

void IconRenderer::Shutdown() {
    ClearCache();
    m_Device = nullptr;
    m_GpuUpload = nullptr;
}

IconDrawInfo IconRenderer::GetLucideIconDrawInfo(
    const std::string& iconName,
    const uint32_t size,
    const Color& color,
    const float strokeWidth) const {
    (void)strokeWidth;
    if (!m_IconManager) {
        return {};
    }
    return m_IconManager->ResolveIcon(iconName, size, color);
}

we::rhi::RHIDescriptorSetHandle IconRenderer::GetLucideIcon(
    const std::string& iconName,
    const uint32_t size,
    const Color& color,
    const float strokeWidth) {
    const IconDrawInfo info = GetLucideIconDrawInfo(iconName, size, color, strokeWidth);
    return info.valid ? info.descriptorSet : we::rhi::RHIDescriptorSetHandle::Invalid;
}

we::rhi::RHIDescriptorSetHandle IconRenderer::GetIcon(const std::string& iconName, const uint32_t size) {
    if (!m_IconManager) {
        return we::rhi::RHIDescriptorSetHandle::Invalid;
    }

    const uint32_t tierPx = IconMetrics::SnapToAtlasTier(size);
    if (iconName.find('/') != std::string::npos || iconName.find('\\') != std::string::npos) {
        const IconDrawInfo info = m_IconManager->ResolvePathIcon(iconName, tierPx, m_DefaultColor);
        return info.valid ? info.descriptorSet : we::rhi::RHIDescriptorSetHandle::Invalid;
    }

    return GetLucideIcon(iconName, tierPx, m_DefaultColor);
}

we::rhi::RHIDescriptorSetHandle IconRenderer::CreateTextureFromBitmap(
    const std::vector<uint8_t>& bitmap,
    const uint32_t width,
    const uint32_t height) {
    (void)bitmap;
    (void)width;
    (void)height;
    return we::rhi::RHIDescriptorSetHandle::Invalid;
}

void IconRenderer::ClearCache() {
    m_ThumbnailCache.clear();
}

bool IconRenderer::CreateTexture(
    const std::vector<uint8_t>&,
    const uint32_t,
    const uint32_t,
    IconTexture&) {
    return false;
}

void IconRenderer::DestroyTexture(IconTexture&) {}

} // namespace WindEffects::Editor::UI
