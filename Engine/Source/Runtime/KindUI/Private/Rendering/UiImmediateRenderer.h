#pragma once

#include "RHI/GpuBackends.h"
#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <cstdint>
#include <mutex>
#include <span>
#include <unordered_map>
#include <vector>

namespace we::runtime::kindui {

// Backend-agnostic immediate-mode UI recorder. All GPU work goes through IRHI only.
class UiImmediateRenderer {
public:
    UiImmediateRenderer() = default;
    ~UiImmediateRenderer();

    UiImmediateRenderer(const UiImmediateRenderer&) = delete;
    UiImmediateRenderer& operator=(const UiImmediateRenderer&) = delete;

    bool Init(we::rhi::IRHIDevice* device, we::rhi::Format swapchainFormat, uint32_t framesInFlight);
    void Shutdown();

    [[nodiscard]] bool IsReady() const { return m_Ready; }

    [[nodiscard]] we::rhi::RHIDescriptorSetHandle RegisterTexture(
        we::rhi::RHITextureViewHandle view, we::rhi::RHISamplerHandle sampler);
    void UpdateTexture(
        we::rhi::RHIDescriptorSetHandle set,
        we::rhi::RHITextureViewHandle view,
        we::rhi::RHISamplerHandle sampler);
    void UnregisterTexture(we::rhi::RHIDescriptorSetHandle set);
    [[nodiscard]] we::rhi::RHIDescriptorSetHandle UploadRgbaTexture(
        uint32_t width,
        uint32_t height,
        std::span<const uint8_t> rgba,
        bool linearFilter);

    [[nodiscard]] we::rhi::RHIDescriptorSetHandle GetDummyTexture() const { return m_DummySet; }
    [[nodiscard]] we::rhi::RHISamplerHandle GetDefaultSampler() const { return m_DummySampler; }

    void BeginFrame(const we::rhi::FramePresentParams& params);
    void SubmitDrawList(const we::rhi::UIDrawList& list, uint32_t frameSlot);
    void EndFrame();

private:
    struct FrameGeometry {
        we::rhi::RHIBufferHandle vertexBuffer = we::rhi::RHIBufferHandle::Invalid;
        we::rhi::RHIBufferHandle indexBuffer = we::rhi::RHIBufferHandle::Invalid;
        uint64_t vertexCapacity = 0;
        uint64_t indexCapacity = 0;
    };

    struct UploadedTexture {
        we::rhi::RHITextureHandle texture = we::rhi::RHITextureHandle::Invalid;
        we::rhi::RHITextureViewHandle view = we::rhi::RHITextureViewHandle::Invalid;
        we::rhi::RHISamplerHandle sampler = we::rhi::RHISamplerHandle::Invalid;
        we::rhi::RHIDescriptorSetHandle set = we::rhi::RHIDescriptorSetHandle::Invalid;
    };

    bool LoadShaders();
    bool CreatePipelines();
    bool CreateDummyTexture();
    [[nodiscard]] we::rhi::RHIDescriptorSetHandle AllocateTextureSet(
        we::rhi::RHITextureViewHandle view, we::rhi::RHISamplerHandle sampler);
    void WriteTextureSet(
        we::rhi::RHIDescriptorSetHandle set,
        we::rhi::RHITextureViewHandle view,
        we::rhi::RHISamplerHandle sampler);
    void DestroyUploaded(UploadedTexture& uploaded);
    void UpdateGeometryBuffers(
        uint32_t frameSlot,
        const std::vector<we::rhi::UIVertex>& vertices,
        const std::vector<uint32_t>& indices);
    [[nodiscard]] bool EnsureBuffer(
        we::rhi::RHIBufferHandle& handle,
        uint64_t& capacity,
        uint64_t required,
        we::rhi::BufferUsage usage,
        const char* debugName);

    we::rhi::IRHIDevice* m_Device = nullptr;
    we::rhi::IRHICommandList* m_Cmd = nullptr;
    we::rhi::Format m_SwapchainFormat = we::rhi::Format::B8G8R8A8_UNORM;
    uint32_t m_MaxFramesInFlight = 2;
    uint32_t m_CurrentWidth = 0;
    uint32_t m_CurrentHeight = 0;
    bool m_Ready = false;
    bool m_InRenderPass = false;

    we::rhi::RHIShaderHandle m_UiVs = we::rhi::RHIShaderHandle::Invalid;
    we::rhi::RHIShaderHandle m_UiPs = we::rhi::RHIShaderHandle::Invalid;
    we::rhi::RHIShaderHandle m_TextVs = we::rhi::RHIShaderHandle::Invalid;
    we::rhi::RHIShaderHandle m_TextPs = we::rhi::RHIShaderHandle::Invalid;
    std::vector<uint8_t> m_UiVsBytes;
    std::vector<uint8_t> m_UiPsBytes;
    std::vector<uint8_t> m_TextVsBytes;
    std::vector<uint8_t> m_TextPsBytes;

    we::rhi::RHIDescriptorSetLayoutHandle m_TextureLayout = we::rhi::RHIDescriptorSetLayoutHandle::Invalid;
    we::rhi::RHIDescriptorPoolHandle m_Pool = we::rhi::RHIDescriptorPoolHandle::Invalid;
    we::rhi::RHIPipelineLayoutHandle m_UiLayout = we::rhi::RHIPipelineLayoutHandle::Invalid;
    we::rhi::RHIPipelineLayoutHandle m_TextLayout = we::rhi::RHIPipelineLayoutHandle::Invalid;
    we::rhi::RHIGraphicsPipelineHandle m_UiPipeline = we::rhi::RHIGraphicsPipelineHandle::Invalid;
    we::rhi::RHIGraphicsPipelineHandle m_TextPipeline = we::rhi::RHIGraphicsPipelineHandle::Invalid;

    we::rhi::RHITextureHandle m_DummyTexture = we::rhi::RHITextureHandle::Invalid;
    we::rhi::RHITextureViewHandle m_DummyView = we::rhi::RHITextureViewHandle::Invalid;
    we::rhi::RHISamplerHandle m_DummySampler = we::rhi::RHISamplerHandle::Invalid;
    we::rhi::RHIDescriptorSetHandle m_DummySet = we::rhi::RHIDescriptorSetHandle::Invalid;
    we::rhi::RHIDescriptorSetHandle m_BoundSet = we::rhi::RHIDescriptorSetHandle::Invalid;

    std::unordered_map<uint64_t, UploadedTexture> m_Uploaded;
    std::vector<FrameGeometry> m_FrameGeometry;
    std::recursive_mutex m_Mutex;
};

} // namespace we::runtime::kindui
