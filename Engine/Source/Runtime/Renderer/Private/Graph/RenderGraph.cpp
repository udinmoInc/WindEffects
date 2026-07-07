#include "Graph/RenderGraph.h"

#include "Core/Validation.h"
#include "Graph/FrameContext.h"

namespace we::runtime::renderer {

RenderGraph::~RenderGraph() {
    Shutdown();
}

void RenderGraph::Init(Renderer* renderer) {
    WE_VALIDATE_INIT(!m_Initialized, "RenderGraph", "Already initialized.");
    WE_VALIDATE_INIT(renderer != nullptr, "RenderGraph", "Renderer is null.");
    
    m_Renderer = renderer;
    m_Initialized = true;
}

void RenderGraph::Shutdown() {
    if (!m_Initialized) return;
    
    m_Passes.clear();
    m_Compiled = false;
    m_Initialized = false;
}

void RenderGraph::AddPass(std::unique_ptr<RenderPass> pass) {
    WE_VALIDATE_INIT(m_Initialized, "RenderGraph", "Cannot add pass to uninitialized RenderGraph.");
    WE_VALIDATE_INIT(!m_Compiled, "RenderGraph", "Cannot add pass to a compiled graph.");
    
    m_Passes.push_back(std::move(pass));
}

void RenderGraph::Compile() {
    WE_VALIDATE_INIT(m_Initialized, "RenderGraph", "Cannot compile uninitialized RenderGraph.");
    
    for (auto& pass : m_Passes) {
        pass->Validate();
    }
    
    m_Compiled = true;
}

void RenderGraph::Execute(const FrameContext& frame) {
    WE_VALIDATE_RENDER(m_Initialized, "RenderGraph::Execute", "RenderGraph not initialized.");
    WE_VALIDATE_RENDER(m_Compiled, "RenderGraph::Execute", "RenderGraph not compiled.");

    for (auto& pass : m_Passes) {
        pass->Execute(frame);
    }
}

} // namespace we::runtime::renderer
