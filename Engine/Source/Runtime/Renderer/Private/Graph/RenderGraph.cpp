#include "Renderer/Graph/RenderGraph.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"

#include <utility>

namespace we::runtime::renderer {

RenderPass::RenderPass(std::string name)
    : m_Name(std::move(name))
{
}

RenderGraph::~RenderGraph() {
    Shutdown();
}

void RenderGraph::Init(we::rhi::IRHIDevice* device) {
    m_Device = device;
    m_Initialized = device != nullptr;
}

void RenderGraph::Shutdown() {
    m_Passes.clear();
    m_Device = nullptr;
    m_Initialized = false;
}

void RenderGraph::ClearPasses() {
    m_Passes.clear();
}

void RenderGraph::AddPass(std::unique_ptr<RenderPass> pass) {
    if (pass) {
        m_Passes.push_back(std::move(pass));
    }
}

void RenderGraph::EmitBarriers(
    we::rhi::IRHICommandList& cmd,
    const std::vector<GraphTextureRef>& textures,
    const std::vector<GraphBufferRef>& buffers)
{
    std::vector<we::rhi::ResourceBarrierDesc> barriers;
    barriers.reserve(textures.size() + buffers.size());

    for (const auto& tex : textures) {
        if (tex.handle == we::rhi::RHITextureHandle::Invalid) {
            continue;
        }
        we::rhi::ResourceBarrierDesc b{};
        b.isTexture = true;
        b.texture.texture = tex.handle;
        b.texture.before = we::rhi::ResourceState::Undefined;
        b.texture.after = tex.desiredState;
        barriers.push_back(b);
    }
    for (const auto& buf : buffers) {
        if (buf.handle == we::rhi::RHIBufferHandle::Invalid) {
            continue;
        }
        we::rhi::ResourceBarrierDesc b{};
        b.isTexture = false;
        b.buffer.buffer = buf.handle;
        b.buffer.before = we::rhi::ResourceState::Undefined;
        b.buffer.after = buf.desiredState;
        barriers.push_back(b);
    }
    if (!barriers.empty()) {
        cmd.ResourceBarrier(barriers);
    }
}

void RenderGraph::Execute(we::rhi::IRHICommandList& cmd, uint32_t frameIndex) {
    if (!m_Initialized || !m_Device) {
        return;
    }

    GraphPassContext ctx{};
    ctx.device = m_Device;
    ctx.commandList = &cmd;
    ctx.frameIndex = frameIndex;

    for (auto& pass : m_Passes) {
        if (!pass) {
            continue;
        }
        std::vector<GraphTextureRef> textures;
        std::vector<GraphBufferRef> buffers;
        pass->Setup(textures, buffers);
        EmitBarriers(cmd, textures, buffers);
        cmd.PushDebugGroup(pass->GetName());
        pass->Execute(ctx);
        cmd.PopDebugGroup();
    }
}

} // namespace we::runtime::renderer
