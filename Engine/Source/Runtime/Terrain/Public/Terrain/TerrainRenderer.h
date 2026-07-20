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
    TerrainRenderer() = default;
    TerrainRenderer(const TerrainRenderer&) = delete;
    TerrainRenderer& operator=(const TerrainRenderer&) = delete;
    TerrainRenderer(TerrainRenderer&&) = delete;
    TerrainRenderer& operator=(TerrainRenderer&&) = delete;

    void Init(we::rhi::IRHIDevice* device);
    void Shutdown();

    /// Upload / refresh GPU buffers for dirty visible chunks.
    void SyncChunks(TerrainChunkManager& chunks);

    /// Propagate CPU chunk.visible → GPU draw flags without uploading (call every frame).
    void SyncVisibility(const TerrainChunkManager& chunks);

    struct DrawItem {
        TerrainChunkId id{};
        const TerrainMeshCPU* mesh = nullptr;
        int lod = 0;
    };
    void BuildDrawList(const TerrainChunkManager& chunks, std::vector<DrawItem>& outDraws) const;

    /// Draw uploaded visible chunk meshes (world-space vertices, identity model).
    /// When lighting is non-null and valid, terrain uses scene sun/sky (same model as meshes).
    void Draw(
        we::rhi::IRHICommandList& cmd,
        we::rhi::RHITextureHandle color,
        we::rhi::RHITextureHandle depth,
        we::rhi::Extent2D extent,
        const we::math::Mat4& view,
        const we::math::Mat4& proj,
        const we::math::Vec3& cameraPos,
        const TerrainSceneLighting* lighting = nullptr);

    [[nodiscard]] std::uint32_t UploadedChunkCount() const { return m_Uploaded; }
    [[nodiscard]] const TerrainRenderStats& LastStats() const noexcept { return m_Stats; }
    [[nodiscard]] bool IsReady() const noexcept { return m_Ready; }
    [[nodiscard]] bool HasDevice() const noexcept { return m_Device != nullptr; }
    [[nodiscard]] bool LastLightingValid() const noexcept { return m_LastLightingValid; }

    void SetClipmapsEnabled(bool enabled) { m_Clipmaps = enabled; }
    bool ClipmapsEnabled() const { return m_Clipmaps; }

    void SetDefaultAlbedo(const we::math::Vec4& rgba) { m_Albedo = rgba; }
    void SetMaterialParams(const TerrainMaterialParams& params) {
        m_Albedo = params.albedo;
        m_Roughness = params.roughness;
        m_Metallic = params.metallic;
        m_GridSpacing = params.gridSpacing;
        m_GridLineWidth = params.gridLineWidth;
        m_GridColor = params.gridColor;
        m_GridOpacity = params.gridOpacity;
        m_GridFadeStart = params.gridFadeStart;
        m_GridFadeEnd = params.gridFadeEnd;
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
        we::rhi::IndexType indexType = we::rhi::IndexType::UInt16;
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
        float lightDir[4]{0.0f, 0.0f, 0.0f, kDefaultLandscapeRoughness};
        float material[4]{kDefaultLandscapeMetallic, 0.0f, 0.0f, 0.0f};
        float gridParams[4]{
            kDefaultLandscapeGridSpacing,
            kDefaultLandscapeGridLineWidth,
            kDefaultLandscapeGridOpacity,
            0.0f};
        float gridColor[4]{
            kDefaultLandscapeGridColorR,
            kDefaultLandscapeGridColorG,
            kDefaultLandscapeGridColorB,
            0.0f};
        float gridFade[4]{
            kDefaultLandscapeGridFadeStart,
            kDefaultLandscapeGridFadeEnd,
            0.0f,
            0.0f};
        float sunTravel[4]{0.3f, -0.8f, 0.2f, 1.2f};
        float sunColor[4]{1.0f, 0.98f, 0.95f, 0.0f};
        float skyAmbient[4]{0.35f, 0.42f, 0.55f, 1.0f};
        float skyLower[4]{0.15f, 0.16f, 0.18f, 0.0f};
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
    float m_GridSpacing = kDefaultLandscapeGridSpacing;
    float m_GridLineWidth = kDefaultLandscapeGridLineWidth;
    we::math::Vec3 m_GridColor{
        kDefaultLandscapeGridColorR,
        kDefaultLandscapeGridColorG,
        kDefaultLandscapeGridColorB};
    float m_GridOpacity = kDefaultLandscapeGridOpacity;
    float m_GridFadeStart = kDefaultLandscapeGridFadeStart;
    float m_GridFadeEnd = kDefaultLandscapeGridFadeEnd;
    std::string m_MaterialPath = kDefaultLandscapeMaterialPath;
    TerrainRenderStats m_Stats{};
    std::uint32_t m_Uploaded = 0;
    bool m_Clipmaps = false;
    bool m_Initialized = false;
    bool m_Ready = false;
    bool m_LastLightingValid = false;
    /// Reused scratch to avoid reallocating interleaved verts every dirty upload.
    std::vector<TerrainVertex> m_UploadScratch;
    std::vector<std::uint16_t> m_IndexScratch16;
};
} // namespace we::runtime::terrain
