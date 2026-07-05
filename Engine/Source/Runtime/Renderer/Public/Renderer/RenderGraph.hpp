#pragma once

#include "Renderer/Renderer.hpp"
#if WE_HAS_VULKAN
#include <volk.h>
#endif
#include <memory>

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

class RenderGraph {
public:
    RENDERER_API RenderGraph(const std::shared_ptr<Renderer>& renderer);
    RENDERER_API ~RenderGraph() = default;

    // Prevent copying
    RenderGraph(const RenderGraph&) = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;

    RENDERER_API void BeginOffscreenPass(VkCommandBuffer cmd) const;
    RENDERER_API void EndOffscreenPass(VkCommandBuffer cmd) const;

    RENDERER_API void BeginSwapchainPass(VkCommandBuffer cmd) const;
    RENDERER_API void EndSwapchainPass(VkCommandBuffer cmd) const;

private:
    std::shared_ptr<Renderer> m_Renderer;
};

#endif // WE_HAS_VULKAN

} // namespace we::runtime::renderer
