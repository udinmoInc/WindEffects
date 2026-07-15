#pragma once

#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <cstdint>
#include <memory>

namespace we::editor::viewport {

class ViewportRenderTarget {
public:
    ViewportRenderTarget() = default;
    ~ViewportRenderTarget();

    bool Init(we::rhi::IRHIDevice* device, we::rhi::Format colorFormat);
    void Shutdown();
    void Resize(uint32_t width, uint32_t height);

    [[nodiscard]] we::rhi::RHITextureHandle GetColorTexture() const { return m_ColorTexture; }
    [[nodiscard]] we::rhi::RHITextureHandle GetDepthTexture() const { return m_DepthTexture; }
    [[nodiscard]] we::rhi::Format GetColorFormat() const { return m_ColorFormat; }
    [[nodiscard]] uint32_t GetWidth() const { return m_Width; }
    [[nodiscard]] uint32_t GetHeight() const { return m_Height; }

private:
    void Recreate();

    we::rhi::IRHIDevice* m_Device = nullptr;
    we::rhi::RHITextureHandle m_ColorTexture = we::rhi::RHITextureHandle::Invalid;
    we::rhi::RHITextureHandle m_DepthTexture = we::rhi::RHITextureHandle::Invalid;
    we::rhi::Format m_ColorFormat = we::rhi::Format::R8G8B8A8_UNORM;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
};

} // namespace we::editor::viewport
