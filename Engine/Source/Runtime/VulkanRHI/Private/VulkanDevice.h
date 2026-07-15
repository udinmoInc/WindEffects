#pragma once

#include "RHI/IRHI.h"

#include <deque>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <volk.h>

namespace we::rhi::vulkan {

class VulkanDevice;

class VulkanQueue final : public IRHIQueue {
public:
    explicit VulkanQueue(QueueType type, VkQueue queue = VK_NULL_HANDLE)
        : m_Type(type)
        , m_Queue(queue)
    {
    }

    void SetQueue(VkQueue queue) { m_Queue = queue; }
    [[nodiscard]] VkQueue GetVkQueue() const { return m_Queue; }

    [[nodiscard]] QueueType GetType() const override { return m_Type; }
    RHIResult<void> Submit(IRHICommandList& commandList) override;
    RHIResult<void> WaitIdle() override;

private:
    QueueType m_Type = QueueType::Graphics;
    VkQueue m_Queue = VK_NULL_HANDLE;
};

class VulkanCommandList final : public IRHICommandList {
public:
    explicit VulkanCommandList(VulkanDevice* device);

    void SetCommandBuffer(VkCommandBuffer cmd) { m_Cmd = cmd; }
    [[nodiscard]] VkCommandBuffer GetVkCommandBuffer() const { return m_Cmd; }
    [[nodiscard]] bool IsRecording() const { return m_Recording; }

    void Begin() override;
    void End() override;
    void BeginRendering(const RenderingInfo& info) override;
    void EndRendering() override;
    void SetViewport(const Viewport& viewport) override;
    void SetScissor(const Scissor& scissor) override;
    void BindGraphicsPipeline(RHIGraphicsPipelineHandle pipeline) override;
    void BindComputePipeline(RHIComputePipelineHandle pipeline) override;
    void BindVertexBuffer(uint32_t binding, RHIBufferHandle buffer, uint64_t offset = 0) override;
    void BindIndexBuffer(RHIBufferHandle buffer, uint64_t offset = 0, IndexType type = IndexType::UInt32) override;
    void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0) override;
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0) override;
    void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
    void CopyBuffer(RHIBufferHandle src, RHIBufferHandle dst, uint64_t size, uint64_t srcOffset = 0, uint64_t dstOffset = 0) override;
    void TransitionTexture(RHITextureHandle texture, ResourceState before, ResourceState after) override;
    void PushDebugGroup(std::string_view name) override;
    void PopDebugGroup() override;
    void InsertDebugMarker(std::string_view name) override;

private:
    VulkanDevice* m_Device = nullptr;
    VkCommandBuffer m_Cmd = VK_NULL_HANDLE;
    bool m_Recording = false;
    bool m_InRendering = false;
};

class VulkanSwapchain final : public IRHISwapchain {
public:
    explicit VulkanSwapchain(VulkanDevice* device);
    ~VulkanSwapchain() override;

    RHIResult<void> Create(const DeviceDesc& desc);
    void Destroy();
    void SetFrameSync(
        const std::vector<VkSemaphore>& imageAvailable,
        const std::vector<VkSemaphore>& renderFinished,
        const std::vector<VkFence>& inFlight);

    [[nodiscard]] Extent2D GetExtent() const override { return m_Extent; }
    [[nodiscard]] Format GetFormat() const override { return m_Format; }
    [[nodiscard]] uint32_t GetImageCount() const override { return static_cast<uint32_t>(m_Images.size()); }
    [[nodiscard]] RHITextureHandle GetCurrentImage() const override;
    [[nodiscard]] uint32_t GetCurrentImageIndex() const override { return m_CurrentIndex; }

    RHIResult<void> Resize(Extent2D extent) override;
    RHIResult<uint32_t> AcquireNextImage() override;
    RHIResult<void> Present() override;

    [[nodiscard]] VkSwapchainKHR GetVkSwapchain() const { return m_Swapchain; }
    [[nodiscard]] VkImage GetVkImage(uint32_t index) const;
    [[nodiscard]] VkImageView GetVkImageView(uint32_t index) const;
    [[nodiscard]] VkQueue GetPresentQueue() const { return m_PresentQueue; }
    [[nodiscard]] bool NeedsRebuild() const { return m_NeedsRebuild; }
    void ClearNeedsRebuild() { m_NeedsRebuild = false; }
    void SetFrameSlot(uint32_t slot) { m_FrameSlot = slot; }
    [[nodiscard]] uint32_t GetFrameSlot() const { return m_FrameSlot; }

    RHIResult<uint32_t> AcquireNextImageForSlot(uint32_t frameSlot);
    RHIResult<void> PresentForSlot(uint32_t frameSlot);

private:
    RHIResult<void> Rebuild();
    void CleanupImages();

    VulkanDevice* m_Device = nullptr;
    DeviceDesc m_Desc{};
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    VkFormat m_VkFormat = VK_FORMAT_UNDEFINED;
    Format m_Format = Format::Unknown;
    Extent2D m_Extent{};
    uint32_t m_CurrentIndex = 0;
    uint32_t m_PresentFamily = ~0u;
    VkQueue m_PresentQueue = VK_NULL_HANDLE;
    bool m_NeedsRebuild = false;
    uint32_t m_FrameSlot = 0;

    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_ImageViews;
    std::vector<RHITextureHandle> m_Handles;

    const std::vector<VkSemaphore>* m_ImageAvailable = nullptr;
    const std::vector<VkSemaphore>* m_RenderFinished = nullptr;
    const std::vector<VkFence>* m_InFlight = nullptr;
};

struct VulkanBuffer {
    BufferDesc desc{};
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
    bool hostVisible = false;
};

struct VulkanTexture {
    TextureDesc desc{};
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    ResourceState state = ResourceState::Undefined;
    bool ownsImage = true;
    bool isSwapchain = false;
};

struct VulkanTextureView {
    TextureViewDesc desc{};
    VkImageView view = VK_NULL_HANDLE;
};

struct VulkanSampler {
    SamplerDesc desc{};
    VkSampler sampler = VK_NULL_HANDLE;
};

struct VulkanShader {
    ShaderDesc desc{};
    VkShaderModule module = VK_NULL_HANDLE;
};

struct VulkanPipelineLayout {
    PipelineLayoutDesc desc{};
    VkPipelineLayout layout = VK_NULL_HANDLE;
};

struct VulkanGraphicsPipeline {
    GraphicsPipelineDesc desc{};
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
};

struct VulkanComputePipeline {
    ComputePipelineDesc desc{};
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
};

enum class DeferredKind : uint8_t {
    Buffer,
    Texture,
    TextureView,
    Sampler,
    Shader,
    PipelineLayout,
    GraphicsPipeline,
    ComputePipeline
};

struct DeferredDestroyItem {
    DeferredKind kind = DeferredKind::Buffer;
    uint64_t handle = 0;
    uint64_t frame = 0;
};

class VulkanDevice final : public IRHIDevice {
public:
    explicit VulkanDevice(const DeviceDesc& desc);
    ~VulkanDevice() override;

    [[nodiscard]] bool IsValid() const override { return m_Valid; }
    [[nodiscard]] RHIBackend GetBackend() const override { return RHIBackend::Vulkan; }
    [[nodiscard]] const RHICapabilities& GetCapabilities() const override { return m_Caps; }
    [[nodiscard]] const RHIDiagnostics& GetDiagnostics() const override { return m_Diagnostics; }
    void ResetDiagnostics() override { m_Diagnostics.Reset(); }

    [[nodiscard]] IRHIQueue& GetQueue(QueueType type) override;
    [[nodiscard]] IRHISwapchain* GetSwapchain() override { return m_Swapchain.get(); }

    [[nodiscard]] RHIResult<RHIBufferHandle> CreateBuffer(const BufferDesc& desc) override;
    RHIResult<void> DestroyBuffer(RHIBufferHandle handle) override;
    [[nodiscard]] void* MapBuffer(RHIBufferHandle handle) override;
    void UnmapBuffer(RHIBufferHandle handle) override;
    RHIResult<void> UpdateBuffer(RHIBufferHandle handle, std::span<const uint8_t> data, uint64_t offset = 0) override;

    [[nodiscard]] RHIResult<RHITextureHandle> CreateTexture(const TextureDesc& desc) override;
    RHIResult<void> DestroyTexture(RHITextureHandle handle) override;
    [[nodiscard]] RHIResult<RHITextureViewHandle> CreateTextureView(const TextureViewDesc& desc) override;
    RHIResult<void> DestroyTextureView(RHITextureViewHandle handle) override;

    [[nodiscard]] RHIResult<RHISamplerHandle> CreateSampler(const SamplerDesc& desc = {}) override;
    RHIResult<void> DestroySampler(RHISamplerHandle handle) override;

    [[nodiscard]] RHIResult<RHIShaderHandle> CreateShader(const ShaderDesc& desc) override;
    RHIResult<void> DestroyShader(RHIShaderHandle handle) override;

    [[nodiscard]] RHIResult<RHIPipelineLayoutHandle> CreatePipelineLayout(const PipelineLayoutDesc& desc = {}) override;
    RHIResult<void> DestroyPipelineLayout(RHIPipelineLayoutHandle handle) override;

    [[nodiscard]] RHIResult<RHIGraphicsPipelineHandle> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
    RHIResult<void> DestroyGraphicsPipeline(RHIGraphicsPipelineHandle handle) override;

    [[nodiscard]] RHIResult<RHIComputePipelineHandle> CreateComputePipeline(const ComputePipelineDesc& desc) override;
    RHIResult<void> DestroyComputePipeline(RHIComputePipelineHandle handle) override;

    [[nodiscard]] IRHICommandList* BeginFrame() override;
    RHIResult<void> Submit(IRHICommandList* commandList) override;
    RHIResult<void> Present() override;
    RHIResult<void> EndFrame() override;
    RHIResult<void> WaitIdle() override;

    void SetResourceName(RHIBufferHandle handle, std::string_view name) override;
    void SetResourceName(RHITextureHandle handle, std::string_view name) override;
    void TickDeferredDestruction() override;

    // Internal accessors for command list / swapchain.
    [[nodiscard]] VkDevice GetVkDevice() const { return m_Device; }
    [[nodiscard]] VkPhysicalDevice GetVkPhysicalDevice() const { return m_PhysicalDevice; }
    [[nodiscard]] VkInstance GetVkInstance() const { return m_Instance; }
    [[nodiscard]] uint32_t GetGraphicsQueueFamily() const { return m_GraphicsFamily; }
    [[nodiscard]] VulkanBuffer* FindBuffer(RHIBufferHandle handle);
    [[nodiscard]] VulkanTexture* FindTexture(RHITextureHandle handle);
    [[nodiscard]] VulkanGraphicsPipeline* FindGraphicsPipeline(RHIGraphicsPipelineHandle handle);
    [[nodiscard]] VulkanComputePipeline* FindComputePipeline(RHIComputePipelineHandle handle);
    RHITextureHandle RegisterSwapchainTexture(VkImage image, VkImageView view, Extent2D extent, Format format);
    void ClearSwapchainTextureHandles(const std::vector<RHITextureHandle>& handles);
    [[nodiscard]] uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    void EnqueueDeferred(DeferredKind kind, uint64_t handle);

private:
    RHIResult<void> CreateInstance(const DeviceDesc& desc);
    RHIResult<void> PickPhysicalDevice(uint32_t adapterIndex);
    RHIResult<void> CreateLogicalDevice();
    RHIResult<void> CreateFrameSync();
    RHIResult<void> CreateCommandPool();
    void DestroyImmediate(DeferredKind kind, uint64_t handle);
    [[nodiscard]] uint64_t AllocHandle();

    DeviceDesc m_Desc{};
    bool m_Valid = false;
    RHICapabilities m_Caps{};
    RHIDiagnostics m_Diagnostics{};

    VkInstance m_Instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    uint32_t m_GraphicsFamily = ~0u;

    VulkanQueue m_GraphicsQueue{QueueType::Graphics};
    VulkanQueue m_ComputeQueue{QueueType::Compute};
    VulkanQueue m_TransferQueue{QueueType::Transfer};

    std::unique_ptr<VulkanSwapchain> m_Swapchain;
    VulkanCommandList m_FrameCmd{this};

    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_CommandBuffers;
    std::vector<VkSemaphore> m_ImageAvailable;
    std::vector<VkSemaphore> m_RenderFinished;
    std::vector<VkFence> m_InFlight;

    uint32_t m_FramesInFlight = 2;
    uint32_t m_FrameSlot = 0;
    uint64_t m_FrameNumber = 0;
    bool m_FrameActive = false;
    IRHICommandList* m_ActiveCmd = nullptr;

    uint64_t m_NextHandle = 1;
    std::unordered_map<uint64_t, VulkanBuffer> m_Buffers;
    std::unordered_map<uint64_t, VulkanTexture> m_Textures;
    std::unordered_map<uint64_t, VulkanTextureView> m_TextureViews;
    std::unordered_map<uint64_t, VulkanSampler> m_Samplers;
    std::unordered_map<uint64_t, VulkanShader> m_Shaders;
    std::unordered_map<uint64_t, VulkanPipelineLayout> m_PipelineLayouts;
    std::unordered_map<uint64_t, VulkanGraphicsPipeline> m_GraphicsPipelines;
    std::unordered_map<uint64_t, VulkanComputePipeline> m_ComputePipelines;
    std::deque<DeferredDestroyItem> m_Deferred;
    std::mutex m_HandleMutex;
};

[[nodiscard]] std::unique_ptr<IRHIDevice> CreateVulkanDevice(const DeviceDesc& desc);

} // namespace we::rhi::vulkan
