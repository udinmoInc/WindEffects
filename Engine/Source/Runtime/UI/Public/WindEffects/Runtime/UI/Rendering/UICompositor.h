#pragma once

#include "RHI/IRHI.h"
#include "RHI/Types.h"

namespace WindEffects::Editor::UI {

class UICompositor {
public:
    bool Initialize(we::rhi::IRHIDevice* device, we::rhi::Format format);
    void Shutdown();

    void BeginComposite(we::rhi::IRHICommandList* cmd, we::rhi::RHITextureViewHandle targetView,
                        we::rhi::Extent2D extent);
    void EndComposite(we::rhi::IRHICommandList* cmd);

private:
    we::rhi::IRHIDevice* m_Device = nullptr;
    we::rhi::Format m_Format = we::rhi::Format::Unknown;
};

} // namespace WindEffects::Editor::UI
