#include "Renderer/Graph/RenderGraph.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "RHI/Desc.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <utility>

namespace we::runtime::renderer {
namespace {

[[nodiscard]] bool IsWriteAccess(GraphResourceAccess access) {
    return access == GraphResourceAccess::Write || access == GraphResourceAccess::ReadWrite;
}

[[nodiscard]] bool IsReadAccess(GraphResourceAccess access) {
    return access == GraphResourceAccess::Read || access == GraphResourceAccess::ReadWrite;
}

} // namespace

RenderPass::RenderPass(std::string name, we::rhi::QueueType queue, GraphPassFlags flags)
    : m_Name(std::move(name))
    , m_Queue(queue)
    , m_Flags(flags)
{
}

RenderGraph::~RenderGraph() {
    Shutdown();
}

void RenderGraph::Init(we::rhi::IRHIDevice* device) {
    m_Device = device;
    m_Initialized = device != nullptr;
    m_AsyncComputeCapable = device && device->GetCapabilities().SupportsAsyncCompute();
    m_TextureStates.clear();
    m_BufferStates.clear();
    m_Resources.clear();
    m_Compiled.clear();
    m_ExecuteOrder.clear();
}

void RenderGraph::Shutdown() {
    DestroyOwnedResources();
    m_Passes.clear();
    m_Resources.clear();
    m_Compiled.clear();
    m_ExecuteOrder.clear();
    m_TextureStates.clear();
    m_BufferStates.clear();
    m_Device = nullptr;
    m_Initialized = false;
}

void RenderGraph::ClearPasses() {
    DestroyOwnedResources();
    m_Passes.clear();
    m_Resources.clear();
    m_Compiled.clear();
    m_ExecuteOrder.clear();
    m_TextureStates.clear();
    m_BufferStates.clear();
}

void RenderGraph::AddPass(std::unique_ptr<RenderPass> pass) {
    if (pass) {
        m_Passes.push_back(std::move(pass));
    }
}

uint32_t RenderGraph::ImportTexture(
    we::rhi::RHITextureHandle handle,
    we::rhi::ResourceState initialState,
    const char* debugName)
{
    ResourceRecord rec{};
    rec.kind = ResourceKind::Texture;
    rec.debugName = debugName ? debugName : "RG.ImportTexture";
    rec.imported = true;
    rec.texture = handle;
    rec.state = initialState;
    rec.realized = handle != we::rhi::RHITextureHandle::Invalid;
    const uint32_t id = static_cast<uint32_t>(m_Resources.size());
    m_Resources.push_back(std::move(rec));
    if (handle != we::rhi::RHITextureHandle::Invalid) {
        m_TextureStates[static_cast<uint64_t>(handle)] = initialState;
    }
    return id;
}

uint32_t RenderGraph::ImportBuffer(
    we::rhi::RHIBufferHandle handle,
    we::rhi::ResourceState initialState,
    const char* debugName)
{
    ResourceRecord rec{};
    rec.kind = ResourceKind::Buffer;
    rec.debugName = debugName ? debugName : "RG.ImportBuffer";
    rec.imported = true;
    rec.buffer = handle;
    rec.state = initialState;
    rec.realized = handle != we::rhi::RHIBufferHandle::Invalid;
    const uint32_t id = static_cast<uint32_t>(m_Resources.size());
    m_Resources.push_back(std::move(rec));
    if (handle != we::rhi::RHIBufferHandle::Invalid) {
        m_BufferStates[static_cast<uint64_t>(handle)] = initialState;
    }
    return id;
}

uint32_t RenderGraph::CreateTexture(const TransientTextureDesc& desc) {
    ResourceRecord rec{};
    rec.kind = ResourceKind::Texture;
    rec.debugName = desc.debugName ? desc.debugName : "RG.TransientTexture";
    rec.imported = false;
    rec.neverRealize = desc.neverRealize;
    rec.textureDesc = desc;
    rec.owned = !desc.neverRealize;
    const uint32_t id = static_cast<uint32_t>(m_Resources.size());
    m_Resources.push_back(std::move(rec));
    return id;
}

uint32_t RenderGraph::CreateBuffer(const TransientBufferDesc& desc) {
    ResourceRecord rec{};
    rec.kind = ResourceKind::Buffer;
    rec.debugName = desc.debugName ? desc.debugName : "RG.TransientBuffer";
    rec.imported = false;
    rec.neverRealize = desc.neverRealize;
    rec.bufferDesc = desc;
    rec.owned = !desc.neverRealize;
    const uint32_t id = static_cast<uint32_t>(m_Resources.size());
    m_Resources.push_back(std::move(rec));
    return id;
}

we::rhi::RHITextureHandle RenderGraph::ResolveTexture(uint32_t resourceId) const {
    if (resourceId >= m_Resources.size()) {
        return we::rhi::RHITextureHandle::Invalid;
    }
    return m_Resources[resourceId].texture;
}

we::rhi::RHIBufferHandle RenderGraph::ResolveBuffer(uint32_t resourceId) const {
    if (resourceId >= m_Resources.size()) {
        return we::rhi::RHIBufferHandle::Invalid;
    }
    return m_Resources[resourceId].buffer;
}

const char* RenderGraph::QueueName(we::rhi::QueueType queue) {
    switch (queue) {
    case we::rhi::QueueType::Graphics: return "Graphics";
    case we::rhi::QueueType::Compute: return "Compute";
    case we::rhi::QueueType::Transfer: return "Transfer";
    case we::rhi::QueueType::Present: return "Present";
    default: return "Unknown";
    }
}

void RenderGraph::ResolveRefs(
    std::vector<GraphTextureRef>& textures,
    std::vector<GraphBufferRef>& buffers) const
{
    for (auto& tex : textures) {
        if (tex.resourceId != kInvalidGraphResourceId && tex.resourceId < m_Resources.size()) {
            tex.handle = m_Resources[tex.resourceId].texture;
        }
    }
    for (auto& buf : buffers) {
        if (buf.resourceId != kInvalidGraphResourceId && buf.resourceId < m_Resources.size()) {
            buf.handle = m_Resources[buf.resourceId].buffer;
        }
    }
}

void RenderGraph::DestroyOwnedResources() {
    if (!m_Device) {
        m_Resources.clear();
        return;
    }
    for (auto& rec : m_Resources) {
        if (!rec.owned || !rec.realized) {
            continue;
        }
        if (rec.kind == ResourceKind::Texture
            && rec.texture != we::rhi::RHITextureHandle::Invalid) {
            (void)m_Device->DestroyTexture(rec.texture);
            rec.texture = we::rhi::RHITextureHandle::Invalid;
        }
        if (rec.kind == ResourceKind::Buffer
            && rec.buffer != we::rhi::RHIBufferHandle::Invalid) {
            (void)m_Device->DestroyBuffer(rec.buffer);
            rec.buffer = we::rhi::RHIBufferHandle::Invalid;
        }
        rec.realized = false;
    }
}

void RenderGraph::CullPasses(std::vector<PassBuildInfo>& builds, std::vector<char>& keep) {
    const size_t n = m_Passes.size();
    keep.assign(n, 0);

    // Seed: SideEffects / KeepAlive, plus any pass writing imported present/swapchain-style resources
    // that are also read with SideEffects later — simple: keep flagged passes.
    for (size_t i = 0; i < n; ++i) {
        const auto flags = m_Passes[i]->GetFlags();
        if (HasFlag(flags, GraphPassFlags::SideEffects) || HasFlag(flags, GraphPassFlags::KeepAlive)) {
            keep[i] = 1;
        }
    }

    // Backward walk: keep producers of resources read by kept consumers.
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t consumer = 0; consumer < n; ++consumer) {
            if (!keep[consumer]) {
                continue;
            }
            for (uint32_t resId : builds[consumer].reads) {
                for (size_t producer = 0; producer < n; ++producer) {
                    if (keep[producer]) {
                        continue;
                    }
                    const auto& writes = builds[producer].writes;
                    if (std::find(writes.begin(), writes.end(), resId) != writes.end()) {
                        keep[producer] = 1;
                        changed = true;
                    }
                }
            }
        }
    }
}

void RenderGraph::PlanWaves(const std::vector<char>& keep, const std::vector<PassBuildInfo>& builds) {
    const size_t n = m_Passes.size();

    // Honest single-queue execution: registration order among kept passes.
    m_ExecuteOrder.clear();
    m_ExecuteOrder.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        if (keep[i]) {
            m_ExecuteOrder.push_back(static_cast<uint32_t>(i));
        }
    }

    // Planned overlap waves from resource deps only (dump / future multi-queue).
    std::vector<int> wave(n, 0);
    for (size_t consumer = 0; consumer < n; ++consumer) {
        if (!keep[consumer]) {
            continue;
        }
        int maxProd = 0;
        bool hasProd = false;
        for (uint32_t resId : builds[consumer].reads) {
            for (size_t producer = 0; producer < consumer; ++producer) {
                if (!keep[producer]) {
                    continue;
                }
                const auto& writes = builds[producer].writes;
                if (std::find(writes.begin(), writes.end(), resId) == writes.end()) {
                    continue;
                }
                hasProd = true;
                maxProd = std::max(maxProd, wave[producer]);
            }
        }
        wave[consumer] = hasProd ? maxProd + 1 : 0;

        if (m_ScheduleMode == RGScheduleMode::AsyncPlanned
            && HasFlag(m_Passes[consumer]->GetFlags(), GraphPassFlags::AsyncCapable)
            && m_Passes[consumer]->GetQueue() == we::rhi::QueueType::Compute) {
            // Eligible to share the producer wave once RHI asyncCompute is true.
            wave[consumer] = hasProd ? maxProd : 0;
        }
    }

    for (auto& info : m_Compiled) {
        if (!info.culled && info.passIndex < wave.size()) {
            info.plannedWave = wave[info.passIndex];
        }
    }
}

void RenderGraph::RealizeResources(
    const std::vector<char>& keep,
    const std::vector<PassBuildInfo>& builds)
{
    if (!m_Device) {
        return;
    }

    for (auto& rec : m_Resources) {
        rec.firstUse = -1;
        rec.lastUse = -1;
    }

    for (uint32_t orderIdx = 0; orderIdx < m_ExecuteOrder.size(); ++orderIdx) {
        const uint32_t passIdx = m_ExecuteOrder[orderIdx];
        auto touch = [&](uint32_t resId) {
            if (resId >= m_Resources.size()) {
                return;
            }
            auto& rec = m_Resources[resId];
            if (rec.firstUse < 0) {
                rec.firstUse = static_cast<int>(orderIdx);
            }
            rec.lastUse = static_cast<int>(orderIdx);
        };
        for (uint32_t id : builds[passIdx].reads) {
            touch(id);
        }
        for (uint32_t id : builds[passIdx].writes) {
            touch(id);
        }
    }

    // Simple aliasing: reuse destroyed transient textures of matching desc when lifetimes don't overlap.
    struct AliasSlot {
        TransientTextureDesc desc{};
        we::rhi::RHITextureHandle handle = we::rhi::RHITextureHandle::Invalid;
        int freeAfter = -1;
    };
    std::vector<AliasSlot> freeTextures;

    for (uint32_t orderIdx = 0; orderIdx < m_ExecuteOrder.size(); ++orderIdx) {
        // Return aliases whose last use has passed
        for (auto& slot : freeTextures) {
            (void)slot;
        }

        for (size_t resId = 0; resId < m_Resources.size(); ++resId) {
            auto& rec = m_Resources[resId];
            if (rec.imported || rec.neverRealize || rec.realized) {
                continue;
            }
            if (rec.firstUse != static_cast<int>(orderIdx)) {
                continue;
            }

            if (rec.kind == ResourceKind::Texture) {
                we::rhi::RHITextureHandle aliased = we::rhi::RHITextureHandle::Invalid;
                for (size_t i = 0; i < freeTextures.size(); ++i) {
                    auto& slot = freeTextures[i];
                    if (slot.freeAfter < rec.firstUse
                        && slot.desc.extent.width == rec.textureDesc.extent.width
                        && slot.desc.extent.height == rec.textureDesc.extent.height
                        && slot.desc.extent.depth == rec.textureDesc.extent.depth
                        && slot.desc.format == rec.textureDesc.format
                        && slot.desc.usage == rec.textureDesc.usage) {
                        aliased = slot.handle;
                        freeTextures.erase(freeTextures.begin() + static_cast<std::ptrdiff_t>(i));
                        break;
                    }
                }
                if (aliased != we::rhi::RHITextureHandle::Invalid) {
                    rec.texture = aliased;
                    rec.realized = true;
                    rec.owned = true;
                    continue;
                }

                we::rhi::TextureDesc desc{};
                desc.extent = rec.textureDesc.extent;
                desc.format = rec.textureDesc.format;
                desc.usage = rec.textureDesc.usage;
                desc.debugName = rec.debugName.c_str();
                auto created = m_Device->CreateTexture(desc);
                if (created) {
                    rec.texture = *created;
                    rec.realized = true;
                    rec.owned = true;
                    m_TextureStates[static_cast<uint64_t>(rec.texture)] = we::rhi::ResourceState::Undefined;
                } else {
                    WE_LOG_WARN(we::LogCategory::Renderer.data(),
                        std::string("RG failed to realize transient texture: ") + rec.debugName);
                }
            } else if (rec.kind == ResourceKind::Buffer && rec.bufferDesc.size > 0) {
                we::rhi::BufferDesc desc{};
                desc.size = rec.bufferDesc.size;
                desc.usage = rec.bufferDesc.usage;
                desc.debugName = rec.debugName.c_str();
                auto created = m_Device->CreateBuffer(desc);
                if (created) {
                    rec.buffer = *created;
                    rec.realized = true;
                    rec.owned = true;
                    m_BufferStates[static_cast<uint64_t>(rec.buffer)] = we::rhi::ResourceState::Undefined;
                }
            }
        }

        // Free resources whose last use is this pass into alias pool
        for (auto& rec : m_Resources) {
            if (!rec.owned || rec.neverRealize || !rec.realized || rec.imported) {
                continue;
            }
            if (rec.lastUse != static_cast<int>(orderIdx)) {
                continue;
            }
            if (rec.kind == ResourceKind::Texture
                && rec.texture != we::rhi::RHITextureHandle::Invalid) {
                AliasSlot slot{};
                slot.desc = rec.textureDesc;
                slot.handle = rec.texture;
                slot.freeAfter = static_cast<int>(orderIdx);
                freeTextures.push_back(slot);
                // Keep handle on record until frame end destroy — alias pool holds same handle.
                // Mark as pooled so DestroyOwnedResources still cleans once.
            }
        }
    }

    (void)keep;
}

uint32_t RenderGraph::FindOrImportTexture(
    we::rhi::RHITextureHandle handle,
    const char* debugName)
{
    for (uint32_t i = 0; i < m_Resources.size(); ++i) {
        if (m_Resources[i].kind == ResourceKind::Texture && m_Resources[i].texture == handle) {
            return i;
        }
    }
    return ImportTexture(handle, we::rhi::ResourceState::Undefined, debugName);
}

uint32_t RenderGraph::FindOrImportBuffer(
    we::rhi::RHIBufferHandle handle,
    const char* debugName)
{
    for (uint32_t i = 0; i < m_Resources.size(); ++i) {
        if (m_Resources[i].kind == ResourceKind::Buffer && m_Resources[i].buffer == handle) {
            return i;
        }
    }
    return ImportBuffer(handle, we::rhi::ResourceState::Undefined, debugName);
}

void RenderGraph::Compile() {
    const size_t n = m_Passes.size();
    std::vector<PassBuildInfo> builds(n);
    m_Compiled.clear();
    m_Compiled.resize(n);

    for (size_t i = 0; i < n; ++i) {
        auto& pass = m_Passes[i];
        auto& build = builds[i];
        pass->Setup(build.textures, build.buffers);

        auto addRes = [&](uint32_t id, GraphResourceAccess access) {
            if (id == kInvalidGraphResourceId) {
                return;
            }
            if (IsReadAccess(access)) {
                build.reads.push_back(id);
            }
            if (IsWriteAccess(access)) {
                build.writes.push_back(id);
            }
        };

        for (auto& tex : build.textures) {
            uint32_t id = tex.resourceId;
            if (id == kInvalidGraphResourceId
                && tex.handle != we::rhi::RHITextureHandle::Invalid) {
                id = FindOrImportTexture(tex.handle, pass->GetName().c_str());
                tex.resourceId = id;
            }
            addRes(id, tex.access);
        }
        for (auto& buf : build.buffers) {
            uint32_t id = buf.resourceId;
            if (id == kInvalidGraphResourceId
                && buf.handle != we::rhi::RHIBufferHandle::Invalid) {
                id = FindOrImportBuffer(buf.handle, pass->GetName().c_str());
                buf.resourceId = id;
            }
            addRes(id, buf.access);
        }

        m_Compiled[i].passIndex = static_cast<uint32_t>(i);
        m_Compiled[i].name = pass->GetName();
        m_Compiled[i].queue = pass->GetQueue();
        m_Compiled[i].flags = pass->GetFlags();
        m_Compiled[i].culled = false;
    }

    std::vector<char> keep;
    CullPasses(builds, keep);
    for (size_t i = 0; i < n; ++i) {
        m_Compiled[i].culled = keep[i] == 0;
    }

    PlanWaves(keep, builds);
    RealizeResources(keep, builds);
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
        const uint64_t key = static_cast<uint64_t>(tex.handle);
        we::rhi::ResourceState before = we::rhi::ResourceState::Undefined;
        if (auto it = m_TextureStates.find(key); it != m_TextureStates.end()) {
            before = it->second;
        }
        if (before == tex.desiredState) {
            continue;
        }
        we::rhi::ResourceBarrierDesc b{};
        b.isTexture = true;
        b.texture.texture = tex.handle;
        b.texture.before = before;
        b.texture.after = tex.desiredState;
        barriers.push_back(b);
        m_TextureStates[key] = tex.desiredState;
        if (tex.resourceId < m_Resources.size()) {
            m_Resources[tex.resourceId].state = tex.desiredState;
        }
    }
    for (const auto& buf : buffers) {
        if (buf.handle == we::rhi::RHIBufferHandle::Invalid) {
            continue;
        }
        const uint64_t key = static_cast<uint64_t>(buf.handle);
        we::rhi::ResourceState before = we::rhi::ResourceState::Undefined;
        if (auto it = m_BufferStates.find(key); it != m_BufferStates.end()) {
            before = it->second;
        }
        if (before == buf.desiredState) {
            continue;
        }
        we::rhi::ResourceBarrierDesc b{};
        b.isTexture = false;
        b.buffer.buffer = buf.handle;
        b.buffer.before = before;
        b.buffer.after = buf.desiredState;
        b.buffer.size = ~0ull;
        barriers.push_back(b);
        m_BufferStates[key] = buf.desiredState;
        if (buf.resourceId < m_Resources.size()) {
            m_Resources[buf.resourceId].state = buf.desiredState;
        }
    }
    if (!barriers.empty()) {
        cmd.ResourceBarrier(barriers);
    }
}

std::string RenderGraph::Dump() const {
    std::ostringstream oss;
    oss << "RenderGraph dump (" << m_Passes.size() << " passes, schedule="
        << (m_ScheduleMode == RGScheduleMode::AsyncPlanned ? "AsyncPlanned" : "SingleQueue")
        << ", asyncComputeRHI=" << (m_AsyncComputeCapable ? "true" : "false") << ")\n";
    for (const auto& info : m_Compiled) {
        oss << "  [" << info.passIndex << "] " << info.name
            << " queue=" << QueueName(info.queue)
            << " wave=" << info.plannedWave
            << (info.culled ? " CULLED" : " RUN");
        if (HasFlag(info.flags, GraphPassFlags::SideEffects)) {
            oss << " SideEffects";
        }
        if (HasFlag(info.flags, GraphPassFlags::KeepAlive)) {
            oss << " KeepAlive";
        }
        if (HasFlag(info.flags, GraphPassFlags::AsyncCapable)) {
            oss << " AsyncCapable";
        }
        oss << "\n";
    }
    oss << "Execute order:";
    for (uint32_t idx : m_ExecuteOrder) {
        oss << " " << m_Passes[idx]->GetName();
    }
    oss << "\n";
    return oss.str();
}

void RenderGraph::Execute(we::rhi::IRHICommandList& cmd, uint32_t frameIndex) {
    if (!m_Initialized || !m_Device) {
        return;
    }

    Compile();

    if (const char* dumpEnv = std::getenv("WE_RG_DUMP")) {
        if (dumpEnv[0] == '1' || dumpEnv[0] == 't' || dumpEnv[0] == 'T') {
            WE_LOG_INFO(we::LogCategory::Renderer.data(), Dump());
        }
    }

    GraphPassContext ctx{};
    ctx.device = m_Device;
    ctx.commandList = &cmd;
    ctx.frameIndex = frameIndex;

    for (uint32_t passIdx : m_ExecuteOrder) {
        auto& pass = m_Passes[passIdx];
        if (!pass) {
            continue;
        }
        std::vector<GraphTextureRef> textures;
        std::vector<GraphBufferRef> buffers;
        pass->Setup(textures, buffers);
        ResolveRefs(textures, buffers);
        EmitBarriers(cmd, textures, buffers);
        cmd.PushDebugGroup(pass->GetName());
        pass->Execute(ctx);
        cmd.PopDebugGroup();
    }
}

} // namespace we::runtime::renderer
