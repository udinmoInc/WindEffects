#pragma once

#pragma warning(push)
#pragma warning(disable : 4251)

#include "Renderer/Export.h"
#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace we::runtime::renderer {

enum class GraphResourceAccess : uint8_t {
    None = 0,
    Read,
    Write,
    ReadWrite
};

struct GraphTextureRef {
    we::rhi::RHITextureHandle handle = we::rhi::RHITextureHandle::Invalid;
    we::rhi::ResourceState desiredState = we::rhi::ResourceState::Undefined;
    GraphResourceAccess access = GraphResourceAccess::Write;
};

struct GraphBufferRef {
    we::rhi::RHIBufferHandle handle = we::rhi::RHIBufferHandle::Invalid;
    we::rhi::ResourceState desiredState = we::rhi::ResourceState::Undefined;
    GraphResourceAccess access = GraphResourceAccess::Write;
};

struct GraphPassContext {
    we::rhi::IRHIDevice* device = nullptr;
    we::rhi::IRHICommandList* commandList = nullptr;
    uint32_t frameIndex = 0;
};

class RENDERER_API RenderPass {
public:
    explicit RenderPass(std::string name);
    virtual ~RenderPass() = default;

    [[nodiscard]] const std::string& GetName() const { return m_Name; }

    virtual void Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>& buffers) = 0;
    virtual void Execute(const GraphPassContext& ctx) = 0;

private:
    std::string m_Name;
};

class RENDERER_API RenderGraph {
public:
    RenderGraph() = default;
    ~RenderGraph();

    RenderGraph(const RenderGraph&) = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;

    void Init(we::rhi::IRHIDevice* device);
    void Shutdown();

    void ClearPasses();
    void AddPass(std::unique_ptr<RenderPass> pass);

    // Compiles dependency order and records resource transitions, then executes.
    void Execute(we::rhi::IRHICommandList& cmd, uint32_t frameIndex);

    [[nodiscard]] bool IsInitialized() const { return m_Initialized; }

private:
    void EmitBarriers(we::rhi::IRHICommandList& cmd, const std::vector<GraphTextureRef>& textures,
        const std::vector<GraphBufferRef>& buffers);

    we::rhi::IRHIDevice* m_Device = nullptr;
    std::vector<std::unique_ptr<RenderPass>> m_Passes;
    bool m_Initialized = false;
};

} // namespace we::runtime::renderer

#pragma warning(pop)
