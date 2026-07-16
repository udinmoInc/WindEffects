#pragma once

#pragma warning(push)
#pragma warning(disable : 4251)

#include "Renderer/Export.h"
#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::runtime::renderer {

enum class GraphResourceAccess : uint8_t {
    None = 0,
    Read,
    Write,
    ReadWrite
};

enum class GraphPassFlags : uint8_t {
    None = 0,
    SideEffects = 1u << 0,   // UI / Present — never cull
    KeepAlive = 1u << 1,     // Named stubs — stay visible even if unused
    AsyncCapable = 1u << 2   // Eligible for planned compute overlap
};

[[nodiscard]] inline GraphPassFlags operator|(GraphPassFlags a, GraphPassFlags b) {
    return static_cast<GraphPassFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
[[nodiscard]] inline GraphPassFlags operator&(GraphPassFlags a, GraphPassFlags b) {
    return static_cast<GraphPassFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
[[nodiscard]] inline bool HasFlag(GraphPassFlags flags, GraphPassFlags bit) {
    return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(bit)) != 0;
}

enum class RGScheduleMode : uint8_t {
    SingleQueue = 0,  // Flatten planned schedule onto graphics (current RHI)
    AsyncPlanned = 1  // Same execution today; dump shows overlap plan
};

constexpr uint32_t kInvalidGraphResourceId = ~0u;

struct TransientTextureDesc {
    we::rhi::Extent3D extent{1, 1, 1};
    we::rhi::Format format = we::rhi::Format::R8G8B8A8_UNORM;
    we::rhi::TextureUsage usage = we::rhi::TextureUsage::ColorAttachment;
    const char* debugName = "RG.TransientTexture";
    bool neverRealize = false; // Stub placeholder — tracked but not allocated
};

struct TransientBufferDesc {
    uint64_t size = 0;
    we::rhi::BufferUsage usage = we::rhi::BufferUsage::Uniform;
    const char* debugName = "RG.TransientBuffer";
    bool neverRealize = false;
};

struct GraphTextureRef {
    uint32_t resourceId = kInvalidGraphResourceId;
    we::rhi::RHITextureHandle handle = we::rhi::RHITextureHandle::Invalid;
    we::rhi::ResourceState desiredState = we::rhi::ResourceState::Undefined;
    GraphResourceAccess access = GraphResourceAccess::Write;
};

struct GraphBufferRef {
    uint32_t resourceId = kInvalidGraphResourceId;
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
    explicit RenderPass(
        std::string name,
        we::rhi::QueueType queue = we::rhi::QueueType::Graphics,
        GraphPassFlags flags = GraphPassFlags::None);
    virtual ~RenderPass() = default;

    [[nodiscard]] const std::string& GetName() const { return m_Name; }
    [[nodiscard]] we::rhi::QueueType GetQueue() const { return m_Queue; }
    [[nodiscard]] GraphPassFlags GetFlags() const { return m_Flags; }

    virtual void Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>& buffers) = 0;
    virtual void Execute(const GraphPassContext& ctx) = 0;

protected:
    void SetQueue(we::rhi::QueueType queue) { m_Queue = queue; }
    void SetFlags(GraphPassFlags flags) { m_Flags = flags; }

private:
    std::string m_Name;
    we::rhi::QueueType m_Queue = we::rhi::QueueType::Graphics;
    GraphPassFlags m_Flags = GraphPassFlags::None;
};

struct CompiledPassInfo {
    uint32_t passIndex = 0;
    std::string name;
    we::rhi::QueueType queue = we::rhi::QueueType::Graphics;
    GraphPassFlags flags = GraphPassFlags::None;
    bool culled = false;
    int plannedWave = 0; // Overlap wave in AsyncPlanned dump
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

    [[nodiscard]] uint32_t ImportTexture(
        we::rhi::RHITextureHandle handle,
        we::rhi::ResourceState initialState,
        const char* debugName = "RG.ImportTexture");
    [[nodiscard]] uint32_t ImportBuffer(
        we::rhi::RHIBufferHandle handle,
        we::rhi::ResourceState initialState,
        const char* debugName = "RG.ImportBuffer");
    [[nodiscard]] uint32_t CreateTexture(const TransientTextureDesc& desc);
    [[nodiscard]] uint32_t CreateBuffer(const TransientBufferDesc& desc);

    [[nodiscard]] we::rhi::RHITextureHandle ResolveTexture(uint32_t resourceId) const;
    [[nodiscard]] we::rhi::RHIBufferHandle ResolveBuffer(uint32_t resourceId) const;

    void SetScheduleMode(RGScheduleMode mode) { m_ScheduleMode = mode; }
    [[nodiscard]] RGScheduleMode GetScheduleMode() const { return m_ScheduleMode; }

    // Compiles dependency order, culls, plans queue overlap, emits barriers, executes.
    void Execute(we::rhi::IRHICommandList& cmd, uint32_t frameIndex);

    [[nodiscard]] std::string Dump() const;
    [[nodiscard]] const std::vector<CompiledPassInfo>& GetCompiledPasses() const { return m_Compiled; }
    [[nodiscard]] bool IsInitialized() const { return m_Initialized; }

private:
    enum class ResourceKind : uint8_t { Texture, Buffer };

    struct ResourceRecord {
        ResourceKind kind = ResourceKind::Texture;
        std::string debugName;
        bool imported = false;
        bool neverRealize = false;
        we::rhi::RHITextureHandle texture = we::rhi::RHITextureHandle::Invalid;
        we::rhi::RHIBufferHandle buffer = we::rhi::RHIBufferHandle::Invalid;
        we::rhi::ResourceState state = we::rhi::ResourceState::Undefined;
        TransientTextureDesc textureDesc{};
        TransientBufferDesc bufferDesc{};
        int firstUse = -1;
        int lastUse = -1;
        bool realized = false;
        bool owned = false;
    };

    struct PassBuildInfo {
        std::vector<GraphTextureRef> textures;
        std::vector<GraphBufferRef> buffers;
        std::vector<uint32_t> reads;
        std::vector<uint32_t> writes;
    };

    void Compile();
    void CullPasses(std::vector<PassBuildInfo>& builds, std::vector<char>& keep);
    void PlanWaves(const std::vector<char>& keep, const std::vector<PassBuildInfo>& builds);
    void RealizeResources(const std::vector<char>& keep, const std::vector<PassBuildInfo>& builds);
    void DestroyOwnedResources();
    void EmitBarriers(
        we::rhi::IRHICommandList& cmd,
        const std::vector<GraphTextureRef>& textures,
        const std::vector<GraphBufferRef>& buffers);
    void ResolveRefs(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>& buffers) const;
    [[nodiscard]] uint32_t FindOrImportTexture(we::rhi::RHITextureHandle handle, const char* debugName);
    [[nodiscard]] uint32_t FindOrImportBuffer(we::rhi::RHIBufferHandle handle, const char* debugName);
    [[nodiscard]] static const char* QueueName(we::rhi::QueueType queue);

    we::rhi::IRHIDevice* m_Device = nullptr;
    std::vector<std::unique_ptr<RenderPass>> m_Passes;
    std::vector<ResourceRecord> m_Resources;
    std::vector<CompiledPassInfo> m_Compiled;
    std::vector<uint32_t> m_ExecuteOrder;
    std::unordered_map<uint64_t, we::rhi::ResourceState> m_TextureStates;
    std::unordered_map<uint64_t, we::rhi::ResourceState> m_BufferStates;
    RGScheduleMode m_ScheduleMode = RGScheduleMode::SingleQueue;
    bool m_AsyncComputeCapable = false;
    bool m_Initialized = false;
};

} // namespace we::runtime::renderer

#pragma warning(pop)
