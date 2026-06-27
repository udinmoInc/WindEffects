#pragma once

#include "Renderer.hpp"
#include <volk.h>
#include <memory>

namespace HouseEngine {

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

} // namespace HouseEngine
