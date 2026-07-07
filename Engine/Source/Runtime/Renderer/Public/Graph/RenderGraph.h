#pragma once

#include <memory>

#include "Graph/RenderPass.h"
#include <vector>
#include <memory>
#include <volk.h>

namespace we::runtime::renderer {

class Renderer;

class RenderGraph {
public:
    RenderGraph() = default;
    ~RenderGraph();

    void Init(Renderer* renderer);
    void Shutdown();

    void AddPass(std::unique_ptr<RenderPass> pass);
    void Compile();
    void Execute(const FrameContext& frame);

private:
    Renderer* m_Renderer = nullptr;
    std::vector<std::unique_ptr<RenderPass>> m_Passes;
    bool m_Compiled = false;
    bool m_Initialized = false;
};

} // namespace we::runtime::renderer
