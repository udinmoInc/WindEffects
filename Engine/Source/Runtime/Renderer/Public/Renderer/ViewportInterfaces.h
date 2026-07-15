#pragma once

#include "Renderer/Export.h"
#include "RHI/Types.h"

#include <cstdint>

namespace we::runtime::renderer {

class RENDERER_API ISceneViewportController {
public:
    virtual ~ISceneViewportController() = default;

    virtual void SetViewportRenderTargetSize(uint32_t width, uint32_t height) = 0;
    virtual void SetViewportBlitRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
    virtual void SetViewportRenderTargetColor(we::rhi::RHITextureHandle colorTexture) = 0;
    virtual void SetViewportDepthTarget(we::rhi::RHITextureHandle depthTexture) = 0;
    [[nodiscard]] virtual we::rhi::RHITextureViewHandle GetViewportColorView() const = 0;
    [[nodiscard]] virtual we::rhi::RHISamplerHandle GetViewportColorSampler() const = 0;
};

} // namespace we::runtime::renderer
