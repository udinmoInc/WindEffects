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
    RenderGraph(const std::shared_ptr<Renderer>& renderer);
    ~RenderGraph() = default;

    // Prevent copying
    RenderGraph(const RenderGraph&) = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;

    void BeginOffscreenPass(VkCommandBuffer cmd) const;
    void EndOffscreenPass(VkCommandBuffer cmd) const;

    void BeginSwapchainPass(VkCommandBuffer cmd) const;
    void EndSwapchainPass(VkCommandBuffer cmd) const;

private:
    std::shared_ptr<Renderer> m_Renderer;
};

#endif // WE_HAS_VULKAN

} // namespace we::runtime::renderer
