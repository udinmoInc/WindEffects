#pragma once

#include "Renderer/Export.h"
#include "Resource/DepthTarget.h"
#include <cstdint>
#include <volk.h>

namespace we::runtime::renderer {

class RENDERER_API ISceneViewportController {
public:
    virtual ~ISceneViewportController() = default;

    // Offscreen viewport render target size (in pixels).
    virtual void SetViewportRenderTargetSize(uint32_t width, uint32_t height) = 0;

    // Screen-space rect where the offscreen viewport is copied into the swapchain.
    virtual void SetViewportBlitRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

    // Offscreen viewport color target handles.
    // Ownership is external (viewport module). Renderer only uses them during rendering.
    virtual void SetViewportRenderTargetColor(VkImage colorImage, VkImageView colorImageView, VkFormat colorFormat) = 0;
    virtual void SetViewportDepthTarget(DepthTarget* depthTarget) = 0;
};

} // namespace we::runtime::renderer

