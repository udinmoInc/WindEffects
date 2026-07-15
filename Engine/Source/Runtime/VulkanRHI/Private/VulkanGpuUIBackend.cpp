#include "RHI/GpuBackends.h"
#include "RHI/RHIFactory.h"

#include "VulkanBackendShared.h"
#include "VulkanDevice.h"
#include "VulkanFormats.h"

#include "Core/DeviceContext.h"
#include "Core/ImageBarriers.h"
#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "Core/SwapchainManager.h"
#include "Resource/ResourceManager.h"
#include "Shader/ShaderLibrary.h"
#include "Shader/ShaderTypes.h"

#include "Platform/Platform.h"

#include <volk.h>

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace we::rhi::vulkan {
namespace {

struct TextPushConstants {
    float uScale[2];
    float uTranslate[2];
    float uAtlasSize[2];
    float uPixelRange;
    float padding;
};

static_assert(sizeof(TextPushConstants) == 32, "Text push constants must match TextMSDF.hlsl layout");
static_assert(sizeof(we::rhi::UIVertex) == sizeof(float) * 16, "UIVertex must match legacy UIVertex2 layout");

void FillUiTransformPushConstants(uint32_t width, uint32_t height, float out[4]) {
    out[0] = 2.0f / static_cast<float>(width);
    out[1] = 2.0f / static_cast<float>(height);
    out[2] = -1.0f;
    out[3] = -1.0f;
}

void FillTextPushConstants(
    uint32_t width,
    uint32_t height,
    uint32_t atlasWidth,
    uint32_t atlasHeight,
    float msdfPixelRange,
    TextPushConstants& out)
{
    float transform[4];
    FillUiTransformPushConstants(width, height, transform);
    out.uScale[0] = transform[0];
    out.uScale[1] = transform[1];
    out.uTranslate[0] = transform[2];
    out.uTranslate[1] = transform[3];
    out.uAtlasSize[0] = static_cast<float>(std::max(atlasWidth, 1u));
    out.uAtlasSize[1] = static_cast<float>(std::max(atlasHeight, 1u));
    out.uPixelRange = std::max(msdfPixelRange, 1.0f);
    out.padding = 0.0f;
}

[[nodiscard]] VkImageView ToVkImageView(RHITextureViewHandle handle) noexcept {
    return reinterpret_cast<VkImageView>(static_cast<uintptr_t>(handle));
}

[[nodiscard]] VkSampler ToVkSampler(RHISamplerHandle handle) noexcept {
    return reinterpret_cast<VkSampler>(static_cast<uintptr_t>(handle));
}

[[nodiscard]] VkDescriptorSet ToVkDescriptorSet(RHIDescriptorSetHandle handle) noexcept {
    return reinterpret_cast<VkDescriptorSet>(static_cast<uintptr_t>(handle));
}

[[nodiscard]] RHIDescriptorSetHandle FromVkDescriptorSet(VkDescriptorSet set) noexcept {
    return static_cast<RHIDescriptorSetHandle>(reinterpret_cast<uintptr_t>(set));
}

[[nodiscard]] RHISamplerHandle FromVkSampler(VkSampler sampler) noexcept {
    return static_cast<RHISamplerHandle>(reinterpret_cast<uintptr_t>(sampler));
}

void SubmitOneTimeUpload(
    we::runtime::renderer::DeviceContext* deviceContext,
    const std::function<void(VkCommandBuffer)>& record)
{
    if (!deviceContext || !record) {
        return;
    }

    VkDevice device = deviceContext->GetDevice();
    VkQueue queue = deviceContext->GetGraphicsQueue();
    const uint32_t queueFamily = deviceContext->GetGraphicsQueueFamily();

    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamily;
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        return;
    }

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(device, &allocInfo, &cmd) != VK_SUCCESS) {
        vkDestroyCommandPool(device, pool, nullptr);
        return;
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);
    record(cmd);
    vkEndCommandBuffer(cmd);

    VkFence fence = VK_NULL_HANDLE;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(device, &fenceInfo, nullptr, &fence);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(queue, 1, &submit, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(device, fence, nullptr);
    vkFreeCommandBuffers(device, pool, 1, &cmd);
    vkDestroyCommandPool(device, pool, nullptr);
}

[[nodiscard]] VkCommandBuffer ResolveCommandBuffer(const FramePresentParams& params) {
    if (params.commandList) {
        if (auto* vkList = dynamic_cast<VulkanCommandList*>(params.commandList)) {
            return vkList->GetVkCommandBuffer();
        }
    }
    return VK_NULL_HANDLE;
}

void BindVolkToVkDevice(VkDevice device) {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    volkLoadDevice(device);
}

struct FrameGeometry {
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
    VkDeviceSize vertexSize = 0;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexMemory = VK_NULL_HANDLE;
    VkDeviceSize indexSize = 0;
};

struct UploadedUITexture {
    RHITextureHandle texture = RHITextureHandle::Invalid;
    RHITextureViewHandle viewHandle = RHITextureViewHandle::Invalid;
    RHISamplerHandle samplerHandle = RHISamplerHandle::Invalid;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkDescriptorSet set = VK_NULL_HANDLE;
};

class VulkanGpuUIBackend final : public IGpuUIBackend {
public:
    bool Initialize(IRHIDevice& device, Format swapchainFormat, uint32_t framesInFlight) override {
        m_RHIDevice = &device;
        m_VulkanDevice = dynamic_cast<VulkanDevice*>(&device);
        if (!m_VulkanDevice) {
            WE_LOG_WARN(we::LogCategory::Renderer.data(),
                "VulkanGpuUIBackend: IRHIDevice is not a VulkanDevice; UI GPU recording disabled.");
            return false;
        }

        m_Device = m_VulkanDevice->GetVkDevice();
        m_SwapchainFormat = ToVkFormat(swapchainFormat);
        if (m_SwapchainFormat == VK_FORMAT_UNDEFINED) {
            m_SwapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;
        }
        m_MaxFramesInFlight = framesInFlight ? framesInFlight : 2;

        BindVolkToVkDevice(m_Device);
        if (!vkCmdBindDescriptorSets || !vkCmdDrawIndexed || !vkCmdBindPipeline) {
            WE_LOG_WARN(we::LogCategory::Renderer.data(),
                "VulkanGpuUIBackend: required device command entry points unavailable.");
            return false;
        }

        try {
            using namespace we::runtime::renderer;
            ShaderLibrary::Get().Initialize(
                we::platform::Platform::Get().GetExecutableDirectory() + "/Engine/Shaders",
                we::platform::Platform::Get().GetExecutableDirectory() + "/Engine/Shaders/Bytecode");
        } catch (...) {
            WE_LOG_WARN(we::LogCategory::Renderer.data(), "VulkanGpuUIBackend: ShaderLibrary init failed.");
        }

        CreateDescriptorSetLayouts();
        CreateDescriptorPool();
        if (m_TextureDescriptorSetLayout == VK_NULL_HANDLE || m_DescriptorPool == VK_NULL_HANDLE) {
            return false;
        }

        CreateDummyTexture();

        CreateGraphicsPipeline(m_SwapchainFormat);
        CreateTextGraphicsPipeline(m_SwapchainFormat);
        if (m_GraphicsPipeline == VK_NULL_HANDLE || m_PipelineLayout == VK_NULL_HANDLE
            || m_TextGraphicsPipeline == VK_NULL_HANDLE || m_TextPipelineLayout == VK_NULL_HANDLE) {
            return false;
        }

        m_FrameGeometry.resize(m_MaxFramesInFlight);
        m_Ready = true;
        WE_LOG_INFO(we::LogCategory::Startup, "VulkanGpuUIBackend initialized on shared IRHI VulkanDevice.");
        return true;
    }

    void Shutdown() override {
        std::lock_guard lock(m_Mutex);
        if (!m_Ready) {
            return;
        }

        if (m_Device != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(m_Device);

            if (m_TextGraphicsPipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(m_Device, m_TextGraphicsPipeline, nullptr);
                m_TextGraphicsPipeline = VK_NULL_HANDLE;
            }
            if (m_TextPipelineLayout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(m_Device, m_TextPipelineLayout, nullptr);
                m_TextPipelineLayout = VK_NULL_HANDLE;
            }
            if (m_GraphicsPipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
                m_GraphicsPipeline = VK_NULL_HANDLE;
            }
            if (m_PipelineLayout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
                m_PipelineLayout = VK_NULL_HANDLE;
            }
            if (m_TextureDescriptorSetLayout != VK_NULL_HANDLE) {
                vkDestroyDescriptorSetLayout(m_Device, m_TextureDescriptorSetLayout, nullptr);
                m_TextureDescriptorSetLayout = VK_NULL_HANDLE;
            }
            if (m_DescriptorPool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
                m_DescriptorPool = VK_NULL_HANDLE;
            }

            if (m_DummyDescriptorSet != VK_NULL_HANDLE && m_RHIDevice) {
                if (m_DummySamplerHandle != RHISamplerHandle::Invalid) {
                    (void)m_RHIDevice->DestroySampler(m_DummySamplerHandle);
                    m_DummySamplerHandle = RHISamplerHandle::Invalid;
                }
                if (m_DummyViewHandle != RHITextureViewHandle::Invalid) {
                    (void)m_RHIDevice->DestroyTextureView(m_DummyViewHandle);
                    m_DummyViewHandle = RHITextureViewHandle::Invalid;
                }
                if (m_DummyTexture != RHITextureHandle::Invalid) {
                    (void)m_RHIDevice->DestroyTexture(m_DummyTexture);
                    m_DummyTexture = RHITextureHandle::Invalid;
                }
                m_DummyDescriptorSet = VK_NULL_HANDLE;
                m_DummySampler = VK_NULL_HANDLE;
                m_DummyImageView = VK_NULL_HANDLE;
            }

            for (auto& [set, uploaded] : m_UploadedTextures) {
                (void)set;
                if (m_RHIDevice) {
                    if (uploaded.samplerHandle != RHISamplerHandle::Invalid) {
                        (void)m_RHIDevice->DestroySampler(uploaded.samplerHandle);
                    }
                    if (uploaded.viewHandle != RHITextureViewHandle::Invalid) {
                        (void)m_RHIDevice->DestroyTextureView(uploaded.viewHandle);
                    }
                    if (uploaded.texture != RHITextureHandle::Invalid) {
                        (void)m_RHIDevice->DestroyTexture(uploaded.texture);
                    }
                }
            }
            m_UploadedTextures.clear();

            for (auto& frame : m_FrameGeometry) {
                if (frame.vertexBuffer != VK_NULL_HANDLE) {
                    vkDestroyBuffer(m_Device, frame.vertexBuffer, nullptr);
                    frame.vertexBuffer = VK_NULL_HANDLE;
                }
                if (frame.vertexMemory != VK_NULL_HANDLE) {
                    vkFreeMemory(m_Device, frame.vertexMemory, nullptr);
                    frame.vertexMemory = VK_NULL_HANDLE;
                }
                if (frame.indexBuffer != VK_NULL_HANDLE) {
                    vkDestroyBuffer(m_Device, frame.indexBuffer, nullptr);
                    frame.indexBuffer = VK_NULL_HANDLE;
                }
                if (frame.indexMemory != VK_NULL_HANDLE) {
                    vkFreeMemory(m_Device, frame.indexMemory, nullptr);
                    frame.indexMemory = VK_NULL_HANDLE;
                }
            }
        }

        m_FrameGeometry.clear();
        m_RHIDevice = nullptr;
        m_VulkanDevice = nullptr;
        m_Cmd = VK_NULL_HANDLE;
        m_Ready = false;
    }

    [[nodiscard]] RHIDescriptorSetHandle RegisterTexture(
        RHITextureViewHandle view, RHISamplerHandle sampler) override
    {
        std::lock_guard lock(m_Mutex);
        VkImageView vkView = VK_NULL_HANDLE;
        VkSampler vkSampler = VK_NULL_HANDLE;
        if (m_VulkanDevice) {
            if (auto* v = m_VulkanDevice->FindTextureView(view)) {
                vkView = v->view;
            }
            if (auto* s = m_VulkanDevice->FindSampler(sampler)) {
                vkSampler = s->sampler;
            }
        }
        if (vkView == VK_NULL_HANDLE) {
            vkView = ToVkImageView(view);
        }
        if (vkSampler == VK_NULL_HANDLE) {
            vkSampler = ToVkSampler(sampler);
        }
        return FromVkDescriptorSet(AllocateTextureDescriptor(vkView, vkSampler));
    }

    void UpdateTexture(
        RHIDescriptorSetHandle set, RHITextureViewHandle view, RHISamplerHandle sampler) override
    {
        std::lock_guard lock(m_Mutex);
        UpdateTextureDescriptor(ToVkDescriptorSet(set), ToVkImageView(view), ToVkSampler(sampler));
    }

    void UnregisterTexture(RHIDescriptorSetHandle set) override {
        std::lock_guard lock(m_Mutex);
        DestroyUploadedTextureLocked(ToVkDescriptorSet(set));
    }

    [[nodiscard]] RHIDescriptorSetHandle UploadRgbaTexture(
        uint32_t width, uint32_t height, std::span<const uint8_t> rgba, bool linearFilter) override
    {
        std::lock_guard lock(m_Mutex);
        if (!m_RHIDevice || !m_VulkanDevice || width == 0 || height == 0
            || rgba.size() < static_cast<size_t>(width) * height * 4) {
            return RHIDescriptorSetHandle::Invalid;
        }

        TextureDesc texDesc{};
        texDesc.extent = {width, height, 1};
        texDesc.format = Format::R8G8B8A8_UNORM;
        texDesc.usage = TextureUsage::Sampled | TextureUsage::TransferDst;
        texDesc.debugName = "UIAtlas";
        auto tex = m_RHIDevice->CreateTexture(texDesc);
        if (!tex) {
            return RHIDescriptorSetHandle::Invalid;
        }

        TextureUpdateDesc update{};
        update.extent = {width, height, 1};
        update.data = rgba;
        if (!m_RHIDevice->UpdateTexture(*tex, update)) {
            (void)m_RHIDevice->DestroyTexture(*tex);
            return RHIDescriptorSetHandle::Invalid;
        }

        TextureViewDesc viewDesc{};
        viewDesc.texture = *tex;
        auto view = m_RHIDevice->CreateTextureView(viewDesc);
        if (!view) {
            (void)m_RHIDevice->DestroyTexture(*tex);
            return RHIDescriptorSetHandle::Invalid;
        }

        SamplerDesc samplerDesc{};
        samplerDesc.magFilter = linearFilter ? Filter::Linear : Filter::Nearest;
        samplerDesc.minFilter = samplerDesc.magFilter;
        samplerDesc.mipFilter = Filter::Nearest;
        samplerDesc.addressU = AddressMode::ClampToEdge;
        samplerDesc.addressV = AddressMode::ClampToEdge;
        samplerDesc.addressW = AddressMode::ClampToEdge;
        samplerDesc.anisotropy = false;
        auto sampler = m_RHIDevice->CreateSampler(samplerDesc);
        if (!sampler) {
            (void)m_RHIDevice->DestroyTextureView(*view);
            (void)m_RHIDevice->DestroyTexture(*tex);
            return RHIDescriptorSetHandle::Invalid;
        }

        auto* vkView = m_VulkanDevice->FindTextureView(*view);
        auto* vkSampler = m_VulkanDevice->FindSampler(*sampler);
        if (!vkView || !vkSampler) {
            (void)m_RHIDevice->DestroySampler(*sampler);
            (void)m_RHIDevice->DestroyTextureView(*view);
            (void)m_RHIDevice->DestroyTexture(*tex);
            return RHIDescriptorSetHandle::Invalid;
        }

        UploadedUITexture uploaded{};
        uploaded.texture = *tex;
        uploaded.viewHandle = *view;
        uploaded.samplerHandle = *sampler;
        uploaded.view = vkView->view;
        uploaded.sampler = vkSampler->sampler;
        uploaded.set = AllocateTextureDescriptor(uploaded.view, uploaded.sampler);
        if (uploaded.set == VK_NULL_HANDLE) {
            (void)m_RHIDevice->DestroySampler(*sampler);
            (void)m_RHIDevice->DestroyTextureView(*view);
            (void)m_RHIDevice->DestroyTexture(*tex);
            return RHIDescriptorSetHandle::Invalid;
        }

        m_UploadedTextures.emplace(uploaded.set, uploaded);
        return FromVkDescriptorSet(uploaded.set);
    }

    [[nodiscard]] RHIDescriptorSetHandle GetDummyTexture() const override {
        return FromVkDescriptorSet(m_DummyDescriptorSet);
    }

    [[nodiscard]] RHISamplerHandle GetDefaultSampler() const override {
        return FromVkSampler(m_DummySampler);
    }

    void BeginFrame(const FramePresentParams& params) override {
        BindVolkToVkDevice(m_Device);
        m_Cmd = ResolveCommandBuffer(params);
        if (!m_Cmd || !m_VulkanDevice || !m_RHIDevice) {
            return;
        }

        auto* swap = m_RHIDevice->GetSwapchain();
        const uint32_t width = params.extent.width
            ? params.extent.width
            : (swap ? swap->GetExtent().width : 0);
        const uint32_t height = params.extent.height
            ? params.extent.height
            : (swap ? swap->GetExtent().height : 0);
        m_CurrentWidth = width;
        m_CurrentHeight = height;

        VkImageView targetView = VK_NULL_HANDLE;
        if (swap) {
            if (auto* tex = m_VulkanDevice->FindTexture(swap->GetCurrentImage())) {
                targetView = tex->view;
            }
        }
        if (targetView == VK_NULL_HANDLE && params.targetView != RHITextureViewHandle::Invalid) {
            if (auto* view = m_VulkanDevice->FindTextureView(params.targetView)) {
                targetView = view->view;
            } else {
                targetView = ToVkImageView(params.targetView);
            }
        }
        if (targetView == VK_NULL_HANDLE) {
            WE_LOG_WARN(we::LogCategory::Renderer.data(), "VulkanGpuUIBackend::BeginFrame: null target view");
            m_Cmd = VK_NULL_HANDLE;
            return;
        }

        auto beginRendering = vkCmdBeginRendering
            ? vkCmdBeginRendering
            : reinterpret_cast<PFN_vkCmdBeginRendering>(
                vkGetDeviceProcAddr(m_Device, "vkCmdBeginRenderingKHR"));
        auto endRendering = vkCmdEndRendering
            ? vkCmdEndRendering
            : reinterpret_cast<PFN_vkCmdEndRendering>(
                vkGetDeviceProcAddr(m_Device, "vkCmdEndRenderingKHR"));
        if (!beginRendering || !endRendering || !vkCmdBindPipeline) {
            WE_LOG_ERROR(we::LogCategory::Renderer.data(),
                "VulkanGpuUIBackend::BeginFrame: required command entry points are null");
            m_Cmd = VK_NULL_HANDLE;
            return;
        }
        m_BeginRendering = beginRendering;
        m_EndRendering = endRendering;

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = targetView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea.offset = {0, 0};
        renderingInfo.renderArea.extent = {std::max(width, 1u), std::max(height, 1u)};
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;

        beginRendering(m_Cmd, &renderingInfo);
        vkCmdBindPipeline(m_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

        m_BoundDescriptorSet = VK_NULL_HANDLE;
        if (m_DummyDescriptorSet != VK_NULL_HANDLE && vkCmdBindDescriptorSets) {
            vkCmdBindDescriptorSets(
                m_Cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_PipelineLayout,
                0,
                1,
                &m_DummyDescriptorSet,
                0,
                nullptr);
            m_BoundDescriptorSet = m_DummyDescriptorSet;
        }

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(width);
        viewport.height = static_cast<float>(height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(m_Cmd, 0, 1, &viewport);

        VkRect2D fullScissor{};
        fullScissor.offset = {0, 0};
        fullScissor.extent = {width, height};
        vkCmdSetScissor(m_Cmd, 0, 1, &fullScissor);

        float pushConstants[4];
        FillUiTransformPushConstants(width, height, pushConstants);
        vkCmdPushConstants(m_Cmd, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), pushConstants);
    }

    void SubmitDrawList(const UIDrawList& list, uint32_t frameSlot) override {
        if (!m_Cmd || frameSlot >= m_FrameGeometry.size()) {
            return;
        }

        if (!list.vertices.empty() && !list.indices.empty()) {
            UpdateGeometryBuffers(frameSlot, list.vertices, list.indices);
        }

        const FrameGeometry& buffers = m_FrameGeometry[frameSlot];
        if (buffers.vertexBuffer == VK_NULL_HANDLE || buffers.indexBuffer == VK_NULL_HANDLE || list.batches.empty()) {
            return;
        }

        VkBuffer vertexBuffers[] = {buffers.vertexBuffer};
        VkDeviceSize vertexOffsets[] = {0};
        vkCmdBindVertexBuffers(m_Cmd, 0, 1, vertexBuffers, vertexOffsets);
        vkCmdBindIndexBuffer(m_Cmd, buffers.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        VkRect2D fullScissor{};
        fullScissor.offset = {0, 0};
        fullScissor.extent = {m_CurrentWidth, m_CurrentHeight};

        auto tryDrawBatch = [&](const UIDrawBatch& batch, bool useTextPipeline) -> bool {
            VkDescriptorSet textureSet = ToVkDescriptorSet(batch.texture);
            if (textureSet == VK_NULL_HANDLE) {
                return false;
            }
            if (textureSet == m_DummyDescriptorSet) {
                uint32_t firstVertex = 0;
                if (!list.indices.empty() && batch.firstIndex < list.indices.size()) {
                    firstVertex = list.indices[batch.firstIndex];
                }
                const bool samplesTexture = firstVertex < list.vertices.size()
                    && (list.vertices[firstVertex].sdfParams[1] < 0.5f
                        || (list.vertices[firstVertex].sdfParams[1] > 3.5f
                            && list.vertices[firstVertex].sdfParams[1] < 4.5f)
                        || (list.vertices[firstVertex].sdfParams[1] > 2.5f
                            && list.vertices[firstVertex].sdfParams[1] < 3.5f));
                if (samplesTexture) {
                    return false;
                }
            }
            if (batch.indexCount == 0) {
                return false;
            }

            VkRect2D batchScissor{};
            batchScissor.offset.x = static_cast<int32_t>(batch.scissor[0]);
            batchScissor.offset.y = static_cast<int32_t>(batch.scissor[1]);
            batchScissor.extent.width = static_cast<uint32_t>(batch.scissor[2]);
            batchScissor.extent.height = static_cast<uint32_t>(batch.scissor[3]);
            if (batchScissor.extent.width == 0 || batchScissor.extent.height == 0) {
                batchScissor = fullScissor;
            }
            vkCmdSetScissor(m_Cmd, 0, 1, &batchScissor);

            // Determine whether this batch's shader path will sample a texture.
            uint32_t firstVertex = 0;
            if (!list.indices.empty() && batch.firstIndex < list.indices.size()) {
                firstVertex = list.indices[batch.firstIndex];
            }
            const float type = (firstVertex < list.vertices.size())
                ? list.vertices[firstVertex].sdfParams[1]
                : 1.0f;
            const bool samplesTexture = type < 0.5f
                || (type > 2.5f && type < 3.5f)
                || (type > 3.5f && type < 4.5f);

            if (useTextPipeline) {
                TextPushConstants textPush{};
                FillTextPushConstants(
                    m_CurrentWidth,
                    m_CurrentHeight,
                    batch.atlasWidth,
                    batch.atlasHeight,
                    batch.msdfPixelRange,
                    textPush);
                vkCmdPushConstants(
                    m_Cmd,
                    m_TextPipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof(TextPushConstants),
                    &textPush);
            }

            const VkPipelineLayout layout = useTextPipeline ? m_TextPipelineLayout : m_PipelineLayout;
            if (textureSet != m_BoundDescriptorSet) {
                vkCmdBindDescriptorSets(
                    m_Cmd,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    layout,
                    0,
                    1,
                    &textureSet,
                    0,
                    nullptr);
                m_BoundDescriptorSet = textureSet;
            }

            vkCmdDrawIndexed(m_Cmd, batch.indexCount, 1, batch.firstIndex, batch.vertexOffset, 0);
            return true;
        };

        for (const auto& batch : list.batches) {
            if (!batch.isText) {
                (void)tryDrawBatch(batch, false);
            }
        }

        bool textPipelineBound = false;
        for (const auto& batch : list.batches) {
            if (!batch.isText) {
                continue;
            }
            if (!textPipelineBound) {
                vkCmdBindPipeline(m_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_TextGraphicsPipeline);
                textPipelineBound = true;
            }
            (void)tryDrawBatch(batch, true);
        }
    }

    void EndFrame() override {
        if (m_Cmd && m_EndRendering) {
            m_EndRendering(m_Cmd);
        }
        m_Cmd = VK_NULL_HANDLE;
        m_BeginRendering = nullptr;
        m_EndRendering = nullptr;
    }

private:
    VkDescriptorSet AllocateTextureDescriptor(VkImageView imageView, VkSampler sampler) {
        if (m_DescriptorPool == VK_NULL_HANDLE || m_TextureDescriptorSetLayout == VK_NULL_HANDLE
            || imageView == VK_NULL_HANDLE || sampler == VK_NULL_HANDLE) {
            return VK_NULL_HANDLE;
        }

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_DescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_TextureDescriptorSetLayout;

        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        if (vkAllocateDescriptorSets(m_Device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
            return VK_NULL_HANDLE;
        }

        UpdateTextureDescriptor(descriptorSet, imageView, sampler);
        return descriptorSet;
    }

    void UpdateTextureDescriptor(VkDescriptorSet descriptorSet, VkImageView imageView, VkSampler sampler) {
        if (descriptorSet == VK_NULL_HANDLE || imageView == VK_NULL_HANDLE || sampler == VK_NULL_HANDLE) {
            return;
        }

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = imageView;
        imageInfo.sampler = sampler;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_Device, 1, &descriptorWrite, 0, nullptr);
    }

    void DestroyUploadedTextureLocked(VkDescriptorSet descriptorSet) {
        if (descriptorSet == VK_NULL_HANDLE) {
            return;
        }

        const auto it = m_UploadedTextures.find(descriptorSet);
        if (it != m_UploadedTextures.end()) {
            UploadedUITexture& uploaded = it->second;
            if (uploaded.set != VK_NULL_HANDLE && m_DescriptorPool != VK_NULL_HANDLE) {
                vkFreeDescriptorSets(m_Device, m_DescriptorPool, 1, &uploaded.set);
            }
            if (m_RHIDevice) {
                if (uploaded.samplerHandle != RHISamplerHandle::Invalid) {
                    (void)m_RHIDevice->DestroySampler(uploaded.samplerHandle);
                }
                if (uploaded.viewHandle != RHITextureViewHandle::Invalid) {
                    (void)m_RHIDevice->DestroyTextureView(uploaded.viewHandle);
                }
                if (uploaded.texture != RHITextureHandle::Invalid) {
                    (void)m_RHIDevice->DestroyTexture(uploaded.texture);
                }
            }
            m_UploadedTextures.erase(it);
            return;
        }

        if (descriptorSet != m_DummyDescriptorSet && m_DescriptorPool != VK_NULL_HANDLE) {
            vkFreeDescriptorSets(m_Device, m_DescriptorPool, 1, &descriptorSet);
        }
    }

    void CreateDescriptorSetLayouts() {
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 0;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &samplerLayoutBinding;

        if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_TextureDescriptorSetLayout) != VK_SUCCESS) {
            m_TextureDescriptorSetLayout = VK_NULL_HANDLE;
        }
    }

    void CreateDescriptorPool() {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 4096;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 2048;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;

        if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
            m_DescriptorPool = VK_NULL_HANDLE;
        }
    }

    void CreateDummyTexture() {
        std::array<uint8_t, 4> pixel = {255, 255, 255, 255};
        auto set = UploadRgbaTexture(1, 1, pixel, false);
        if (set == RHIDescriptorSetHandle::Invalid) {
            return;
        }
        m_DummyDescriptorSet = ToVkDescriptorSet(set);
        // Keep dummy out of the uploaded destroy map's normal Unregister path by tracking separately.
        auto it = m_UploadedTextures.find(m_DummyDescriptorSet);
        if (it != m_UploadedTextures.end()) {
            m_DummySampler = it->second.sampler;
            m_DummyImageView = it->second.view;
            m_DummyTexture = it->second.texture;
            m_DummyViewHandle = it->second.viewHandle;
            m_DummySamplerHandle = it->second.samplerHandle;
            m_UploadedTextures.erase(it);
        }
    }

    void CreateGraphicsPipeline(VkFormat colorFormat) {
        using namespace we::runtime::renderer;

        VkShaderModule vertShaderModule = ShaderLibrary::Get().CreateShaderModule(
            m_Device, "UI", we::runtime::renderer::ShaderStage::Vertex);
        VkShaderModule fragShaderModule = ShaderLibrary::Get().CreateShaderModule(
            m_Device, "UI", we::runtime::renderer::ShaderStage::Pixel);
        if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
            return;
        }

        VkPipelineShaderStageCreateInfo shaderStages[2]{};
        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = vertShaderModule;
        shaderStages[0].pName = "VSMain";
        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = fragShaderModule;
        shaderStages[1].pName = "PSMain";

        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(we::rhi::UIVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};
        attributeDescriptions[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(we::rhi::UIVertex, position)};
        attributeDescriptions[1] = {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(we::rhi::UIVertex, uv)};
        attributeDescriptions[2] = {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(we::rhi::UIVertex, color)};
        attributeDescriptions[3] = {3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(we::rhi::UIVertex, sdfRect)};
        attributeDescriptions[4] = {4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(we::rhi::UIVertex, sdfParams)};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(float) * 4;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_TextureDescriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
            vkDestroyShaderModule(m_Device, fragShaderModule, nullptr);
            vkDestroyShaderModule(m_Device, vertShaderModule, nullptr);
            return;
        }

        VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
        pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        pipelineRenderingInfo.colorAttachmentCount = 1;
        pipelineRenderingInfo.pColorAttachmentFormats = &colorFormat;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext = &pipelineRenderingInfo;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_PipelineLayout;
        pipelineInfo.renderPass = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS) {
            m_GraphicsPipeline = VK_NULL_HANDLE;
        }

        vkDestroyShaderModule(m_Device, fragShaderModule, nullptr);
        vkDestroyShaderModule(m_Device, vertShaderModule, nullptr);
    }

    void CreateTextGraphicsPipeline(VkFormat colorFormat) {
        using namespace we::runtime::renderer;

        VkShaderModule vertShaderModule = ShaderLibrary::Get().CreateShaderModule(
            m_Device, "TextMSDF", we::runtime::renderer::ShaderStage::Vertex);
        VkShaderModule fragShaderModule = ShaderLibrary::Get().CreateShaderModule(
            m_Device, "TextMSDF", we::runtime::renderer::ShaderStage::Pixel);
        if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
            return;
        }

        VkPipelineShaderStageCreateInfo shaderStages[2]{};
        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = vertShaderModule;
        shaderStages[0].pName = "VSMain";
        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = fragShaderModule;
        shaderStages[1].pName = "PSMain";

        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(we::rhi::UIVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};
        attributeDescriptions[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(we::rhi::UIVertex, position)};
        attributeDescriptions[1] = {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(we::rhi::UIVertex, uv)};
        attributeDescriptions[2] = {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(we::rhi::UIVertex, color)};
        attributeDescriptions[3] = {3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(we::rhi::UIVertex, sdfRect)};
        attributeDescriptions[4] = {4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(we::rhi::UIVertex, sdfParams)};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(TextPushConstants);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_TextureDescriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_TextPipelineLayout) != VK_SUCCESS) {
            vkDestroyShaderModule(m_Device, fragShaderModule, nullptr);
            vkDestroyShaderModule(m_Device, vertShaderModule, nullptr);
            return;
        }

        VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
        pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        pipelineRenderingInfo.colorAttachmentCount = 1;
        pipelineRenderingInfo.pColorAttachmentFormats = &colorFormat;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext = &pipelineRenderingInfo;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_TextPipelineLayout;
        pipelineInfo.renderPass = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_TextGraphicsPipeline) != VK_SUCCESS) {
            m_TextGraphicsPipeline = VK_NULL_HANDLE;
        }

        vkDestroyShaderModule(m_Device, fragShaderModule, nullptr);
        vkDestroyShaderModule(m_Device, vertShaderModule, nullptr);
    }

    [[nodiscard]] uint32_t FindHostVisibleMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
        if (!m_VulkanDevice) {
            return UINT32_MAX;
        }
        return m_VulkanDevice->FindMemoryType(typeFilter, properties);
    }

    bool CreateHostVisibleBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkBuffer& buffer,
        VkDeviceMemory& memory) const
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            return false;
        }

        VkMemoryRequirements memRequirements{};
        vkGetBufferMemoryRequirements(m_Device, buffer, &memRequirements);

        const uint32_t memoryType = FindHostVisibleMemoryType(
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (memoryType == UINT32_MAX) {
            vkDestroyBuffer(m_Device, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
            return false;
        }

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = memoryType;
        if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
            vkDestroyBuffer(m_Device, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
            return false;
        }

        if (vkBindBufferMemory(m_Device, buffer, memory, 0) != VK_SUCCESS) {
            vkFreeMemory(m_Device, memory, nullptr);
            vkDestroyBuffer(m_Device, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
            memory = VK_NULL_HANDLE;
            return false;
        }
        return true;
    }

    void DestroyBufferPair(VkBuffer& buffer, VkDeviceMemory& memory) const {
        if (buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_Device, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
        }
        if (memory != VK_NULL_HANDLE) {
            vkFreeMemory(m_Device, memory, nullptr);
            memory = VK_NULL_HANDLE;
        }
    }

    void UpdateGeometryBuffers(
        uint32_t frameIndex,
        const std::vector<we::rhi::UIVertex>& vertices,
        const std::vector<uint32_t>& indices)
    {
        if (frameIndex >= m_FrameGeometry.size() || vertices.empty() || indices.empty()
            || !m_VulkanDevice) {
            return;
        }

        FrameGeometry& frame = m_FrameGeometry[frameIndex];
        const VkDeviceSize vertexSize = static_cast<VkDeviceSize>(vertices.size() * sizeof(we::rhi::UIVertex));
        const VkDeviceSize indexSize = static_cast<VkDeviceSize>(indices.size() * sizeof(uint32_t));

        if (vertexSize > frame.vertexSize) {
            DestroyBufferPair(frame.vertexBuffer, frame.vertexMemory);
            frame.vertexSize = vertexSize * 2;
            if (!CreateHostVisibleBuffer(
                    frame.vertexSize,
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    frame.vertexBuffer,
                    frame.vertexMemory)) {
                frame.vertexSize = 0;
                return;
            }
        }

        if (indexSize > frame.indexSize) {
            DestroyBufferPair(frame.indexBuffer, frame.indexMemory);
            frame.indexSize = indexSize * 2;
            if (!CreateHostVisibleBuffer(
                    frame.indexSize,
                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    frame.indexBuffer,
                    frame.indexMemory)) {
                frame.indexSize = 0;
                return;
            }
        }

        void* mapped = nullptr;
        if (vkMapMemory(m_Device, frame.vertexMemory, 0, vertexSize, 0, &mapped) != VK_SUCCESS || !mapped) {
            return;
        }
        std::memcpy(mapped, vertices.data(), static_cast<size_t>(vertexSize));
        vkUnmapMemory(m_Device, frame.vertexMemory);

        mapped = nullptr;
        if (vkMapMemory(m_Device, frame.indexMemory, 0, indexSize, 0, &mapped) != VK_SUCCESS || !mapped) {
            return;
        }
        std::memcpy(mapped, indices.data(), static_cast<size_t>(indexSize));
        vkUnmapMemory(m_Device, frame.indexMemory);
    }

    bool m_Ready = false;
    uint32_t m_MaxFramesInFlight = 2;
    uint32_t m_CurrentWidth = 0;
    uint32_t m_CurrentHeight = 0;
    VkFormat m_SwapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkCommandBuffer m_Cmd = VK_NULL_HANDLE;
    PFN_vkCmdBeginRendering m_BeginRendering = nullptr;
    PFN_vkCmdEndRendering m_EndRendering = nullptr;
    IRHIDevice* m_RHIDevice = nullptr;
    VulkanDevice* m_VulkanDevice = nullptr;

    VkDescriptorSetLayout m_TextureDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_TextPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_TextGraphicsPipeline = VK_NULL_HANDLE;

    RHITextureHandle m_DummyTexture = RHITextureHandle::Invalid;
    RHITextureViewHandle m_DummyViewHandle = RHITextureViewHandle::Invalid;
    RHISamplerHandle m_DummySamplerHandle = RHISamplerHandle::Invalid;
    VkImageView m_DummyImageView = VK_NULL_HANDLE;
    VkSampler m_DummySampler = VK_NULL_HANDLE;
    VkDescriptorSet m_DummyDescriptorSet = VK_NULL_HANDLE;
    VkDescriptorSet m_BoundDescriptorSet = VK_NULL_HANDLE;

    std::unordered_map<VkDescriptorSet, UploadedUITexture> m_UploadedTextures;
    std::vector<FrameGeometry> m_FrameGeometry;
    std::recursive_mutex m_Mutex;
};

std::unique_ptr<IGpuUIBackend> CreateVulkanGpuUIBackend() {
    return std::make_unique<VulkanGpuUIBackend>();
}

struct VulkanUIRegistrar {
    VulkanUIRegistrar() {
        GpuBackendRegistry::RegisterUI(&CreateVulkanGpuUIBackend);
    }
};

static VulkanUIRegistrar g_VulkanUIRegistrar;

} // namespace
} // namespace we::rhi::vulkan
