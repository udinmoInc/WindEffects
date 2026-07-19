#pragma once

#include "Terrain/Export.h"
#include "Terrain/TerrainTypes.h"
#include "Terrain/TerrainChunkManager.h"
#include "Terrain/TerrainMaterialSystem.h"
#include "RHI/IRHI.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
namespace we::runtime::terrain {

/// Uploads visible chunk meshes and draws them into the viewport color/depth targets.
class TERRAIN_API TerrainRenderer {
public:
    void Init(we::rhi::IRHIDevice* device);
    void Shutdown();

    /// Upload / refresh GPU buffers for dirty visible chunks.
    void SyncChunks(TerrainChunkManager& chunks);

    struct DrawItem {
        TerrainChunkId id{};
        const TerrainMeshCPU* mesh = nullptr;
        int lod = 0;
    };
    void BuildDrawList(const TerrainChunkManager& chunks, std::vector<DrawItem>& outDraws) const;

    /// Draw uploaded visible chunk meshes (world-space vertices, identity model).
    void Draw(
        we::rhi::IRHICommandList& cmd,
        we::rhi::RHITextureHandle color,
        we::rhi::RHITextureHandle depth,
        we::rhi::Extent2D extent,
        const we::math::Mat4& view,
        const we::math::Mat4& proj,
        const we::math::Vec3& cameraPos);

    [[nodiscard]] std::uint32_t UploadedChunkCount() const { return m_Uploaded; }
    [[nodiscard]] const TerrainRenderStats& LastStats() const noexcept { return m_Stats; }
    [[nodiscard]] bool IsReady() const noexcept { return m_Ready; }
    [[nodiscard]] bool HasDevice() const noexcept { return m_Device != nullptr; }

    void SetClipmapsEnabled(bool enabled) { m_Clipmaps = enabled; }
    bool ClipmapsEnabled() const { return m_Clipmaps; }

    void SetDefaultAlbedo(const we::math::Vec4& rgba) { m_Albedo = rgba; }
    void SetMaterialParams(const TerrainMaterialParams& params) {
        m_Albedo = params.albedo;
        m_Roughness = params.roughness;
        m_Metallic = params.metallic;
        m_MaterialPath = params.materialPath;
    }
    [[nodiscard]] const std::string& MaterialPath() const noexcept { return m_MaterialPath; }

private:
    struct ChunkGpu {
        we::rhi::RHIBufferHandle vertex = we::rhi::RHIBufferHandle::Invalid;
        we::rhi::RHIBufferHandle index = we::rhi::RHIBufferHandle::Invalid;
        std::uint32_t indexCount = 0;
        std::uint32_t vertexCount = 0;
        std::uint64_t vertexBytes = 0;
        std::uint64_t indexBytes = 0;
        bool visible = true;
    };

    struct TerrainVertex {
        float px, py, pz;
        float nx, ny, nz;
        float u, v;
    };

    struct CameraUBO {
        we::math::Mat4 view{};
        we::math::Mat4 proj{};
        we::math::Mat4 invViewProj{};
        we::math::Vec3 position{};
        float padding = 0.0f;
    };

    struct MaterialUBO {
        float albedo[4]{
            kDefaultLandscapeAlbedoR,
            kDefaultLandscapeAlbedoG,
            kDefaultLandscapeAlbedoB,
            1.0f};
        float lightDir[4]{0.45f, 0.85f, 0.25f, kDefaultLandscapeRoughness};
        float material[4]{kDefaultLandscapeMetallic, 0.0f, 0.0f, 0.0f};
    };

    bool EnsurePipeline(we::rhi::Format colorFormat, we::rhi::Format depthFormat, bool hasDepth);
    bool LoadShaders();
    bool EnsureGpuResources();
    void DestroyChunkGpu(ChunkGpu& gpu);
    bool UploadChunk(const TerrainChunk& chunk, ChunkGpu& gpu);

    we::rhi::IRHIDevice* m_Device = nullptr;
    we::rhi::RHIShaderHandle m_Vs = we::rhi::RHIShaderHandle::Invalid;
    we::rhi::RHIShaderHandle m_Ps = we::rhi::RHIShaderHandle::Invalid;
    std::vector<std::uint8_t> m_VsBytes;
    std::vector<std::uint8_t> m_PsBytes;

    we::rhi::RHIDescriptorSetLayoutHandle m_CameraLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
    we::rhi::RHIDescriptorSetLayoutHandle m_MaterialLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
    we::rhi::RHIPipelineLayoutHandle m_PipelineLayout = we::rhi::RHIPipelineLayoutHandle::Invalid;
    we::rhi::RHIGraphicsPipelineHandle m_Pipeline = we::rhi::RHIGraphicsPipelineHandle::Invalid;
    we::rhi::RHIDescriptorPoolHandle m_Pool = we::rhi::RHIDescriptorPoolHandle::Invalid;
    we::rhi::RHIDescriptorSetHandle m_CameraSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    we::rhi::RHIDescriptorSetHandle m_MaterialSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    we::rhi::RHIBufferHandle m_CameraBuffer = we::rhi::RHIBufferHandle::Invalid;
    we::rhi::RHIBufferHandle m_MaterialBuffer = we::rhi::RHIBufferHandle::Invalid;

    we::rhi::Format m_PipelineColorFormat = we::rhi::Format::Unknown;
    we::rhi::Format m_PipelineDepthFormat = we::rhi::Format::Unknown;
    bool m_PipelineHasDepth = false;

    std::unordered_map<TerrainChunkId, ChunkGpu, TerrainChunkIdHash> m_GpuChunks;
    we::math::Vec4 m_Albedo{
        kDefaultLandscapeAlbedoR,
        kDefaultLandscapeAlbedoG,
        kDefaultLandscapeAlbedoB,
        1.0f};
    float m_Roughness = kDefaultLandscapeRoughness;
    float m_Metallic = kDefaultLandscapeMetallic;
    std::string m_MaterialPath = kDefaultLandscapeMaterialPath;
    TerrainRenderStats m_Stats{};
    std::uint32_t m_Uploaded = 0;
    bool m_Clipmaps = false;
    bool m_Initialized = false;
    bool m_Ready = false;
};
} // namespace we::runtime::terrain
