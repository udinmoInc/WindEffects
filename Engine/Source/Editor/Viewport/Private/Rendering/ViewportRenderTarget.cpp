#include "Rendering/ViewportRenderTarget.h"
#include "RHI/Desc.h"

namespace we::editor::viewport {

ViewportRenderTarget::~ViewportRenderTarget() {
    Shutdown();
}

bool ViewportRenderTarget::Init(we::rhi::IRHIDevice* device, we::rhi::Format colorFormat) {
    m_Device = device;
    m_ColorFormat = colorFormat;
    return m_Device != nullptr;
}

void ViewportRenderTarget::Shutdown() {
    if (m_Device) {
        if (m_ColorTexture != we::rhi::RHITextureHandle::Invalid) {
            (void)m_Device->DestroyTexture(m_ColorTexture);
            m_ColorTexture = we::rhi::RHITextureHandle::Invalid;
        }
        if (m_DepthTexture != we::rhi::RHITextureHandle::Invalid) {
            (void)m_Device->DestroyTexture(m_DepthTexture);
            m_DepthTexture = we::rhi::RHITextureHandle::Invalid;
        }
    }
    m_Width = 0;
    m_Height = 0;
}

void ViewportRenderTarget::Resize(uint32_t width, uint32_t height) {
    if (width == m_Width && height == m_Height) {
        return;
    }
    m_Width = width;
    m_Height = height;
    Recreate();
}

void ViewportRenderTarget::Recreate() {
    if (!m_Device || m_Width == 0 || m_Height == 0) {
        return;
    }
    if (m_ColorTexture != we::rhi::RHITextureHandle::Invalid) {
        (void)m_Device->DestroyTexture(m_ColorTexture);
        m_ColorTexture = we::rhi::RHITextureHandle::Invalid;
    }
    if (m_DepthTexture != we::rhi::RHITextureHandle::Invalid) {
        (void)m_Device->DestroyTexture(m_DepthTexture);
        m_DepthTexture = we::rhi::RHITextureHandle::Invalid;
    }

    we::rhi::TextureDesc colorDesc{};
    colorDesc.extent = {m_Width, m_Height, 1};
    colorDesc.format = m_ColorFormat;
    colorDesc.usage = we::rhi::TextureUsage::ColorAttachment | we::rhi::TextureUsage::Sampled
        | we::rhi::TextureUsage::TransferSrc;
    colorDesc.debugName = "ViewportColor";
    if (auto r = m_Device->CreateTexture(colorDesc)) {
        m_ColorTexture = *r;
    }

    we::rhi::TextureDesc depthDesc{};
    depthDesc.extent = {m_Width, m_Height, 1};
    depthDesc.format = we::rhi::Format::D32_SFLOAT;
    depthDesc.usage = we::rhi::TextureUsage::DepthStencil;
    depthDesc.debugName = "ViewportDepth";
    if (auto r = m_Device->CreateTexture(depthDesc)) {
        m_DepthTexture = *r;
    }
}

} // namespace we::editor::viewport
