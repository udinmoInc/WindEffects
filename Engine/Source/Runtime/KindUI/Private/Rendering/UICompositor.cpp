#include "KindUI/Rendering/UICompositor.h"

namespace we::runtime::kindui {

bool UICompositor::Initialize(we::rhi::IRHIDevice* device, we::rhi::Format format) {
    m_Device = device;
    m_Format = format;
    return m_Device != nullptr;
}

void UICompositor::Shutdown() {
    m_Device = nullptr;
}

void UICompositor::BeginComposite(we::rhi::IRHICommandList*, we::rhi::RHITextureViewHandle, we::rhi::Extent2D) {}
void UICompositor::EndComposite(we::rhi::IRHICommandList*) {}

} // namespace we::runtime::kindui
