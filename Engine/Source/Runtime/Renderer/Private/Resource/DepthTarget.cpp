#include "Resource/DepthTarget.h"
#include "RHI/Desc.h"

namespace we::runtime::renderer {

DepthTarget::~DepthTarget() {
    Shutdown();
}

void DepthTarget::Init(we::rhi::IRHIDevice* device, uint32_t width, uint32_t height) {
    Shutdown();
    m_Device = device;
    m_Width = width;
    m_Height = height;
    if (m_Device && m_Width > 0 && m_Height > 0) {
        CreateResources();
    }
}

void DepthTarget::Shutdown() {
    if (m_Device && m_Texture != we::rhi::RHITextureHandle::Invalid) {
        (void)m_Device->DestroyTexture(m_Texture);
        m_Texture = we::rhi::RHITextureHandle::Invalid;
    }
    m_Device = nullptr;
    m_Width = 0;
    m_Height = 0;
}

void DepthTarget::Resize(uint32_t width, uint32_t height) {
    if (width == m_Width && height == m_Height) {
        return;
    }
    auto* device = m_Device;
    Shutdown();
    Init(device, width, height);
}

void DepthTarget::CreateResources() {
    if (!m_Device) {
        return;
    }
    we::rhi::TextureDesc desc{};
    desc.extent = {m_Width, m_Height, 1};
    desc.format = m_Format;
    desc.usage = we::rhi::TextureUsage::DepthStencil;
    desc.debugName = "DepthTarget";
    auto result = m_Device->CreateTexture(desc);
    if (result) {
        m_Texture = *result;
    }
}

} // namespace we::runtime::renderer
