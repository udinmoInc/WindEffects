#include "Terrain/TerrainRenderer.h"

#include "Core/Logger.h"
#include "Core/Math/Types.h"
#include "RHI/Desc.h"
#include "RHI/ShaderBytecode.h"
#include "Terrain/TerrainDiagnostics.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <span>
namespace we::runtime::terrain {
namespace {

we::math::Mat4 InvertViewProj(const we::math::Mat4& view, const we::math::Mat4& proj) {
    // Prefer identity invViewProj when inversion is unavailable — shader only needs view/proj for VS.
    (void)view;
    (void)proj;
    return we::math::Mat4{};
}

} // namespace

void TerrainRenderer::Init(we::rhi::IRHIDevice* device) {
    Shutdown();
    m_Device = device;
    m_Uploaded = 0;
    m_Initialized = true;
    m_Ready = false;
    if (!m_Device) {
        HE_ERROR("[Terrain] TerrainRenderer::Init failed — null IRHI device.");
        return;
    }
    if (!LoadShaders() || !EnsureGpuResources()) {
        HE_ERROR("[Terrain] TerrainRenderer GPU init failed.");
        Shutdown();
        m_Initialized = true; // keep device pointer for retry after bytecode lands
        m_Device = device;
        return;
    }
    m_Ready = true;
    HE_INFO("[Terrain] TerrainRenderer ready (GPU chunk mesh path).");
}

void TerrainRenderer::Shutdown() {
    if (m_Device) {
        for (auto& [_, gpu] : m_GpuChunks) {
            DestroyChunkGpu(gpu);
        }
        m_GpuChunks.clear();

        if (m_Pipeline != we::rhi::RHIGraphicsPipelineHandle::Invalid) {
            (void)m_Device->DestroyGraphicsPipeline(m_Pipeline);
            m_Pipeline = we::rhi::RHIGraphicsPipelineHandle::Invalid;
        }
        if (m_PipelineLayout != we::rhi::RHIPipelineLayoutHandle::Invalid) {
            (void)m_Device->DestroyPipelineLayout(m_PipelineLayout);
            m_PipelineLayout = we::rhi::RHIPipelineLayoutHandle::Invalid;
        }
        if (m_Pool != we::rhi::RHIDescriptorPoolHandle::Invalid) {
            (void)m_Device->DestroyDescriptorPool(m_Pool);
            m_Pool = we::rhi::RHIDescriptorPoolHandle::Invalid;
            m_CameraSet = we::rhi::RHIDescriptorSetHandle::Invalid;
            m_MaterialSet = we::rhi::RHIDescriptorSetHandle::Invalid;
        }
        if (m_CameraLayout != we::rhi::RHIDescriptorSetLayoutHandle::Invalid) {
            (void)m_Device->DestroyDescriptorSetLayout(m_CameraLayout);
            m_CameraLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
        }
        if (m_MaterialLayout != we::rhi::RHIDescriptorSetLayoutHandle::Invalid) {
            (void)m_Device->DestroyDescriptorSetLayout(m_MaterialLayout);
            m_MaterialLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
        }
        if (m_CameraBuffer != we::rhi::RHIBufferHandle::Invalid) {
            (void)m_Device->DestroyBuffer(m_CameraBuffer);
            m_CameraBuffer = we::rhi::RHIBufferHandle::Invalid;
        }
        if (m_MaterialBuffer != we::rhi::RHIBufferHandle::Invalid) {
            (void)m_Device->DestroyBuffer(m_MaterialBuffer);
            m_MaterialBuffer = we::rhi::RHIBufferHandle::Invalid;
        }
        if (m_Vs != we::rhi::RHIShaderHandle::Invalid) {
            (void)m_Device->DestroyShader(m_Vs);
            m_Vs = we::rhi::RHIShaderHandle::Invalid;
        }
        if (m_Ps != we::rhi::RHIShaderHandle::Invalid) {
            (void)m_Device->DestroyShader(m_Ps);
            m_Ps = we::rhi::RHIShaderHandle::Invalid;
        }
    }
    m_VsBytes.clear();
    m_PsBytes.clear();
    m_PipelineColorFormat = we::rhi::Format::Unknown;
    m_PipelineDepthFormat = we::rhi::Format::Unknown;
    m_PipelineHasDepth = false;
    m_Device = nullptr;
    m_Uploaded = 0;
    m_Initialized = false;
    m_Ready = false;
    m_Stats = {};
}

void TerrainRenderer::DestroyChunkGpu(ChunkGpu& gpu) {
    if (!m_Device) {
        gpu = {};
        return;
    }
    if (gpu.vertex != we::rhi::RHIBufferHandle::Invalid) {
        (void)m_Device->DestroyBuffer(gpu.vertex);
    }
    if (gpu.index != we::rhi::RHIBufferHandle::Invalid) {
        (void)m_Device->DestroyBuffer(gpu.index);
    }
    gpu = {};
}

bool TerrainRenderer::LoadShaders() {
    const auto format = we::rhi::ShaderBytecodeLoader::ResolveFormat(m_Device);
    m_VsBytes = we::rhi::ShaderBytecodeLoader::Load("Terrain", "VS", format);
    m_PsBytes = we::rhi::ShaderBytecodeLoader::Load("Terrain", "PS", format);
    if (m_VsBytes.empty() || m_PsBytes.empty()) {
        HE_ERROR("[Terrain] Failed to load Terrain shader bytecode (VS/PS).");
        return false;
    }

    we::rhi::ShaderDesc vsDesc{};
    vsDesc.stage = we::rhi::ShaderStage::Vertex;
    vsDesc.format = format;
    vsDesc.bytecode = m_VsBytes;
    vsDesc.entryPoint = "VSMain";
    vsDesc.debugName = "Terrain.VS";
    auto vs = m_Device->CreateShader(vsDesc);
    if (!vs) {
        return false;
    }
    m_Vs = *vs;

    we::rhi::ShaderDesc psDesc{};
    psDesc.stage = we::rhi::ShaderStage::Fragment;
    psDesc.format = format;
    psDesc.bytecode = m_PsBytes;
    psDesc.entryPoint = "PSMain";
    psDesc.debugName = "Terrain.PS";
    auto ps = m_Device->CreateShader(psDesc);
    if (!ps) {
        return false;
    }
    m_Ps = *ps;
    return true;
}

bool TerrainRenderer::EnsureGpuResources() {
    we::rhi::DescriptorSetLayoutDesc cameraLayoutDesc{};
    cameraLayoutDesc.debugName = "Terrain.CameraLayout";
    cameraLayoutDesc.bindings.push_back({
        0,
        we::rhi::DescriptorType::UniformBuffer,
        1,
        we::rhi::ShaderStageFlags::Vertex | we::rhi::ShaderStageFlags::Fragment});
    auto cameraLayout = m_Device->CreateDescriptorSetLayout(cameraLayoutDesc);
    if (!cameraLayout) {
        return false;
    }
    m_CameraLayout = *cameraLayout;

    we::rhi::DescriptorSetLayoutDesc materialLayoutDesc{};
    materialLayoutDesc.debugName = "Terrain.MaterialLayout";
    materialLayoutDesc.bindings.push_back({
        0,
        we::rhi::DescriptorType::UniformBuffer,
        1,
        we::rhi::ShaderStageFlags::Vertex | we::rhi::ShaderStageFlags::Fragment});
    auto materialLayout = m_Device->CreateDescriptorSetLayout(materialLayoutDesc);
    if (!materialLayout) {
        return false;
    }
    m_MaterialLayout = *materialLayout;

    we::rhi::PipelineLayoutDesc pipelineLayoutDesc{};
    pipelineLayoutDesc.debugName = "Terrain.PipelineLayout";
    pipelineLayoutDesc.setLayouts = {m_CameraLayout, m_MaterialLayout};
    auto pipelineLayout = m_Device->CreatePipelineLayout(pipelineLayoutDesc);
    if (!pipelineLayout) {
        return false;
    }
    m_PipelineLayout = *pipelineLayout;

    we::rhi::BufferDesc cameraBuf{};
    cameraBuf.size = sizeof(CameraUBO);
    cameraBuf.usage = we::rhi::BufferUsage::Uniform;
    cameraBuf.memory = we::rhi::MemoryUsage::HostVisible;
    cameraBuf.debugName = "Terrain.CameraUBO";
    auto cBuf = m_Device->CreateBuffer(cameraBuf);
    if (!cBuf) {
        return false;
    }
    m_CameraBuffer = *cBuf;

    we::rhi::BufferDesc materialBuf{};
    materialBuf.size = sizeof(MaterialUBO);
    materialBuf.usage = we::rhi::BufferUsage::Uniform;
    materialBuf.memory = we::rhi::MemoryUsage::HostVisible;
    materialBuf.debugName = "Terrain.MaterialUBO";
    auto mBuf = m_Device->CreateBuffer(materialBuf);
    if (!mBuf) {
        return false;
    }
    m_MaterialBuffer = *mBuf;

    we::rhi::DescriptorPoolDesc poolDesc{};
    poolDesc.maxSets = 4;
    poolDesc.debugName = "Terrain.Pool";
    poolDesc.poolSizes.push_back({we::rhi::DescriptorType::UniformBuffer, 8});
    auto pool = m_Device->CreateDescriptorPool(poolDesc);
    if (!pool) {
        return false;
    }
    m_Pool = *pool;

    we::rhi::DescriptorSetAllocateDesc camSetDesc{};
    camSetDesc.pool = m_Pool;
    camSetDesc.layout = m_CameraLayout;
    auto camSet = m_Device->AllocateDescriptorSet(camSetDesc);
    if (!camSet) {
        return false;
    }
    m_CameraSet = *camSet;

    we::rhi::DescriptorSetAllocateDesc matSetDesc{};
    matSetDesc.pool = m_Pool;
    matSetDesc.layout = m_MaterialLayout;
    auto matSet = m_Device->AllocateDescriptorSet(matSetDesc);
    if (!matSet) {
        return false;
    }
    m_MaterialSet = *matSet;

    we::rhi::DescriptorBufferInfo camInfo{m_CameraBuffer, 0, sizeof(CameraUBO)};
    we::rhi::DescriptorBufferInfo matInfo{m_MaterialBuffer, 0, sizeof(MaterialUBO)};
    we::rhi::WriteDescriptorSet writes[2]{};
    writes[0].set = m_CameraSet;
    writes[0].binding = 0;
    writes[0].type = we::rhi::DescriptorType::UniformBuffer;
    writes[0].count = 1;
    writes[0].bufferInfos = &camInfo;
    writes[1].set = m_MaterialSet;
    writes[1].binding = 0;
    writes[1].type = we::rhi::DescriptorType::UniformBuffer;
    writes[1].count = 1;
    writes[1].bufferInfos = &matInfo;
    m_Device->UpdateDescriptorSets(writes);
    return true;
}

bool TerrainRenderer::EnsurePipeline(
    we::rhi::Format colorFormat,
    we::rhi::Format depthFormat,
    bool hasDepth)
{
    if (m_Pipeline != we::rhi::RHIGraphicsPipelineHandle::Invalid
        && m_PipelineColorFormat == colorFormat
        && m_PipelineDepthFormat == depthFormat
        && m_PipelineHasDepth == hasDepth) {
        return true;
    }
    if (m_Pipeline != we::rhi::RHIGraphicsPipelineHandle::Invalid) {
        (void)m_Device->DestroyGraphicsPipeline(m_Pipeline);
        m_Pipeline = we::rhi::RHIGraphicsPipelineHandle::Invalid;
    }

    we::rhi::GraphicsPipelineDesc pso{};
    pso.vertexShader = m_Vs;
    pso.fragmentShader = m_Ps;
    pso.layout = m_PipelineLayout;
    pso.topology = we::rhi::PrimitiveTopology::TriangleList;
    pso.cullMode = we::rhi::CullMode::None;
    pso.depthTest = true;
    pso.depthWrite = true;
    pso.depthCompare = we::rhi::CompareOp::LessOrEqual;
    pso.blend.enable = false;
    pso.colorFormat = colorFormat;
    pso.depthFormat = depthFormat;
    pso.depthAttachment = hasDepth;
    pso.debugName = "Terrain.PSO";

    pso.vertexBindings.push_back({0, static_cast<uint32_t>(sizeof(TerrainVertex)), false});
    pso.vertexAttributes.push_back(
        {0, 0, we::rhi::Format::R32G32B32_SFLOAT, 0, "POSITION", 0});
    pso.vertexAttributes.push_back(
        {1, 0, we::rhi::Format::R32G32B32_SFLOAT, 12, "NORMAL", 0});
    pso.vertexAttributes.push_back(
        {2, 0, we::rhi::Format::R32G32_SFLOAT, 24, "TEXCOORD", 0});

    auto pipeline = m_Device->CreateGraphicsPipeline(pso);
    if (!pipeline) {
        HE_ERROR("[Terrain] CreateGraphicsPipeline failed.");
        return false;
    }
    m_Pipeline = *pipeline;
    m_PipelineColorFormat = colorFormat;
    m_PipelineDepthFormat = depthFormat;
    m_PipelineHasDepth = hasDepth;
    return true;
}

bool TerrainRenderer::UploadChunk(const TerrainChunk& chunk, ChunkGpu& gpu) {
    if (!m_Device || chunk.mesh.positions.empty() || chunk.mesh.indices.empty()) {
        return false;
    }
    const std::size_t vertCount = chunk.mesh.positions.size();
    std::vector<TerrainVertex> verts(vertCount);
    for (std::size_t i = 0; i < vertCount; ++i) {
        const we::math::Vec3& p = chunk.mesh.positions[i];
        const we::math::Vec3& n = (i < chunk.mesh.normals.size())
            ? chunk.mesh.normals[i]
            : we::math::Vec3(0.f, 1.f, 0.f);
        const we::math::Vec2 uv = (i < chunk.mesh.uvs.size())
            ? chunk.mesh.uvs[i]
            : we::math::Vec2(0.f, 0.f);
        verts[i] = {p.x, p.y, p.z, n.x, n.y, n.z, uv.x, uv.y};
    }

    const std::uint64_t vertexBytes = verts.size() * sizeof(TerrainVertex);
    const std::uint64_t indexBytes = chunk.mesh.indices.size() * sizeof(std::uint32_t);

    if (gpu.vertex == we::rhi::RHIBufferHandle::Invalid || gpu.vertexBytes < vertexBytes) {
        if (gpu.vertex != we::rhi::RHIBufferHandle::Invalid) {
            (void)m_Device->DestroyBuffer(gpu.vertex);
            gpu.vertex = we::rhi::RHIBufferHandle::Invalid;
        }
        we::rhi::BufferDesc desc{};
        desc.size = std::max<std::uint64_t>(vertexBytes, sizeof(TerrainVertex));
        desc.usage = we::rhi::BufferUsage::Vertex;
        desc.memory = we::rhi::MemoryUsage::HostVisible;
        desc.debugName = "Terrain.ChunkVB";
        auto buf = m_Device->CreateBuffer(desc);
        if (!buf) {
            return false;
        }
        gpu.vertex = *buf;
        gpu.vertexBytes = desc.size;
    }

    if (gpu.index == we::rhi::RHIBufferHandle::Invalid || gpu.indexBytes < indexBytes) {
        if (gpu.index != we::rhi::RHIBufferHandle::Invalid) {
            (void)m_Device->DestroyBuffer(gpu.index);
            gpu.index = we::rhi::RHIBufferHandle::Invalid;
        }
        we::rhi::BufferDesc desc{};
        desc.size = std::max<std::uint64_t>(indexBytes, sizeof(std::uint32_t));
        desc.usage = we::rhi::BufferUsage::Index;
        desc.memory = we::rhi::MemoryUsage::HostVisible;
        desc.debugName = "Terrain.ChunkIB";
        auto buf = m_Device->CreateBuffer(desc);
        if (!buf) {
            return false;
        }
        gpu.index = *buf;
        gpu.indexBytes = desc.size;
    }

    if (!m_Device->UpdateBuffer(
            gpu.vertex,
            std::span(reinterpret_cast<const std::uint8_t*>(verts.data()), vertexBytes))) {
        return false;
    }
    if (!m_Device->UpdateBuffer(
            gpu.index,
            std::span(
                reinterpret_cast<const std::uint8_t*>(chunk.mesh.indices.data()),
                indexBytes))) {
        return false;
    }

    gpu.vertexCount = static_cast<std::uint32_t>(vertCount);
    gpu.indexCount = static_cast<std::uint32_t>(chunk.mesh.indices.size());
    return true;
}

void TerrainRenderer::SyncChunks(TerrainChunkManager& chunks) {
    const auto t0 = std::chrono::steady_clock::now();
    m_Stats = {};
    m_Stats.chunksCreated = static_cast<std::uint32_t>(chunks.Chunks().size());
    m_Stats.materialAssigned = m_MaterialPath.empty()
        ? kDefaultLandscapeMaterialPath
        : m_MaterialPath;
    m_Stats.pipelineReady = m_Ready;
    m_Stats.renderProxyReady = m_Ready;

    if (!m_Initialized) {
        return;
    }
    if (!m_Ready && m_Device) {
        // Retry shader/GPU init (e.g. bytecode staged after first Create).
        if (LoadShaders() && EnsureGpuResources()) {
            m_Ready = true;
            m_Stats.pipelineReady = true;
            m_Stats.renderProxyReady = true;
            HE_INFO("[Terrain] TerrainRenderer GPU path recovered.");
        }
    }

    we::math::Vec3 bmin(1e30f);
    we::math::Vec3 bmax(-1e30f);
    bool anyBounds = false;
    std::uint32_t uploaded = 0;
    std::uint32_t uploadedDirty = 0;
    std::uint64_t memoryBytes = 0;

    for (TerrainChunk& chunk : chunks.Chunks()) {
        if (chunk.mesh.positions.empty() || chunk.mesh.indices.empty()) {
            continue;
        }
        if (!chunk.visible) {
            auto it = m_GpuChunks.find(chunk.id);
            if (it != m_GpuChunks.end()) {
                it->second.visible = false;
            }
            continue;
        }
        ++m_Stats.chunksVisible;
        ++m_Stats.chunksLoaded;
        m_Stats.vertices += static_cast<std::uint32_t>(chunk.mesh.positions.size());
        m_Stats.triangles += static_cast<std::uint32_t>(chunk.mesh.indices.size() / 3);

        if (!anyBounds) {
            bmin = chunk.bounds.min;
            bmax = chunk.bounds.max;
            anyBounds = true;
        } else {
            bmin = we::math::Vec3(
                std::min(bmin.x, chunk.bounds.min.x),
                std::min(bmin.y, chunk.bounds.min.y),
                std::min(bmin.z, chunk.bounds.min.z));
            bmax = we::math::Vec3(
                std::max(bmax.x, chunk.bounds.max.x),
                std::max(bmax.y, chunk.bounds.max.y),
                std::max(bmax.z, chunk.bounds.max.z));
        }

        if (!m_Ready || !m_Device) {
            continue;
        }

        ChunkGpu& gpu = m_GpuChunks[chunk.id];
        gpu.visible = true;
        if (chunk.gpuDirty || gpu.vertex == we::rhi::RHIBufferHandle::Invalid
            || gpu.indexCount == 0) {
            if (UploadChunk(chunk, gpu)) {
                chunk.gpuDirty = false;
                ++uploaded;
                ++uploadedDirty;
            } else {
                HE_ERROR(
                    "[Terrain] GPU upload failed for chunk ("
                    + std::to_string(chunk.id.x) + "," + std::to_string(chunk.id.z) + ").");
            }
        } else {
            ++uploaded;
        }
        memoryBytes += gpu.vertexBytes + gpu.indexBytes;
    }

    // Drop GPU buffers for chunks that no longer exist.
    for (auto it = m_GpuChunks.begin(); it != m_GpuChunks.end();) {
        if (!chunks.Find(it->first)) {
            DestroyChunkGpu(it->second);
            it = m_GpuChunks.erase(it);
        } else {
            ++it;
        }
    }

    m_Uploaded = uploaded;
    m_Stats.chunksUploaded = uploadedDirty;
    m_Stats.gpuBuffers = 0;
    for (const auto& [_, gpu] : m_GpuChunks) {
        if (gpu.vertex != we::rhi::RHIBufferHandle::Invalid
            && gpu.index != we::rhi::RHIBufferHandle::Invalid) {
            ++m_Stats.gpuBuffers;
        }
    }
    m_Stats.memoryBytes = memoryBytes;
    if (anyBounds) {
        m_Stats.bounds.min = bmin;
        m_Stats.bounds.max = bmax;
    }

    const auto uploadMicros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - t0)
            .count());
    m_Stats.gpuUploadMicros = uploadMicros;
    TerrainDiagnostics::Get().OnGpuUpload(uploadMicros, uploadedDirty);
}

void TerrainRenderer::BuildDrawList(
    const TerrainChunkManager& chunks,
    std::vector<DrawItem>& outDraws) const
{
    outDraws.clear();
    for (const TerrainChunk& chunk : chunks.Chunks()) {
        if (!chunk.visible || chunk.mesh.indices.empty()) {
            continue;
        }
        DrawItem item{};
        item.id = chunk.id;
        item.mesh = &chunk.mesh;
        item.lod = chunk.lod;
        outDraws.push_back(item);
    }
}

void TerrainRenderer::Draw(
    we::rhi::IRHICommandList& cmd,
    we::rhi::RHITextureHandle color,
    we::rhi::RHITextureHandle depth,
    we::rhi::Extent2D extent,
    const we::math::Mat4& view,
    const we::math::Mat4& proj,
    const we::math::Vec3& cameraPos)
{
    m_Stats.drawCalls = 0;
    if (!m_Ready || !m_Device || color == we::rhi::RHITextureHandle::Invalid
        || extent.width == 0 || extent.height == 0 || m_GpuChunks.empty()) {
        return;
    }

    const bool hasDepth = depth != we::rhi::RHITextureHandle::Invalid;
    if (!EnsurePipeline(
            we::rhi::Format::R16G16B16A16_SFLOAT,
            we::rhi::Format::D32_SFLOAT,
            hasDepth)) {
        return;
    }

    CameraUBO camera{};
    camera.view = view;
    camera.proj = proj;
    camera.invViewProj = InvertViewProj(view, proj);
    camera.position = cameraPos;
    (void)m_Device->UpdateBuffer(
        m_CameraBuffer,
        std::span(reinterpret_cast<const std::uint8_t*>(&camera), sizeof(camera)));

    MaterialUBO material{};
    material.albedo[0] = m_Albedo.x;
    material.albedo[1] = m_Albedo.y;
    material.albedo[2] = m_Albedo.z;
    material.albedo[3] = m_Albedo.w;
    material.lightDir[0] = 0.45f;
    material.lightDir[1] = 0.85f;
    material.lightDir[2] = 0.25f;
    material.lightDir[3] = m_Roughness;
    material.material[0] = m_Metallic;
    (void)m_Device->UpdateBuffer(
        m_MaterialBuffer,
        std::span(reinterpret_cast<const std::uint8_t*>(&material), sizeof(material)));

    we::rhi::RenderingInfo info{};
    we::rhi::ColorAttachmentDesc colorAtt{};
    colorAtt.texture = color;
    colorAtt.loadOp = we::rhi::LoadOp::Load;
    colorAtt.storeOp = we::rhi::StoreOp::Store;
    info.colorAttachments.push_back(colorAtt);
    if (hasDepth) {
        info.depth.enabled = true;
        info.depth.texture = depth;
        info.depth.loadOp = we::rhi::LoadOp::Load;
        info.depth.storeOp = we::rhi::StoreOp::Store;
    }
    info.renderArea = extent;

    cmd.BeginRendering(info);
    cmd.SetViewport({0, 0,
        static_cast<float>(extent.width),
        static_cast<float>(extent.height),
        0.0f, 1.0f});
    cmd.SetScissor({0, 0, extent.width, extent.height});
    cmd.BindGraphicsPipeline(m_Pipeline);
    const we::rhi::RHIDescriptorSetHandle sets[] = {m_CameraSet, m_MaterialSet};
    cmd.BindDescriptorSets(
        we::rhi::PipelineBindPoint::Graphics,
        m_PipelineLayout,
        0,
        sets);

    std::uint32_t rendered = 0;
    for (const auto& [_, gpu] : m_GpuChunks) {
        if (!gpu.visible
            || gpu.vertex == we::rhi::RHIBufferHandle::Invalid
            || gpu.index == we::rhi::RHIBufferHandle::Invalid
            || gpu.indexCount == 0) {
            continue;
        }
        cmd.BindVertexBuffer(0, gpu.vertex);
        cmd.BindIndexBuffer(gpu.index, 0, we::rhi::IndexType::UInt32);
        cmd.DrawIndexed(gpu.indexCount);
        ++m_Stats.drawCalls;
        ++rendered;
    }
    cmd.EndRendering();

    TerrainDiagnostics::Get().OnFrameRenderStats(
        m_Stats.chunksVisible,
        m_Stats.chunksStreamed,
        rendered,
        m_Stats.drawCalls,
        m_Stats.memoryBytes);
}

} // namespace we::runtime::terrain
