#pragma once

#include "Platform/NativeHandle.h"
#include "RHI/Types.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::rhi {

class IRHICommandList;

enum class ShaderStage : uint8_t {
    Vertex = 0,
    Fragment,
    Compute,
    Geometry,
    TessControl,
    TessEvaluation,
    Mesh,
    Task,
    RayGen,
    ClosestHit,
    Miss,
    AnyHit,
    Intersection
};

struct RHIInitDesc {
    RHIBackend preferredBackend = RHIBackend::Auto;
    const char* appName = "WindEffects";
    const char* appVersion = "0.0.0";
    bool enableValidation = false;
    bool enableDebugMarkers = true;
    bool headless = false;
    uint32_t framesInFlight = 2;
};

struct AdapterDesc {
    uint32_t index = 0;
    std::string name;
    uint64_t dedicatedVideoMemory = 0;
    bool isDiscrete = true;
    bool isSoftware = false;
};

struct DeviceDesc {
    we::platform::WindowId windowId = we::platform::WindowId::Invalid;
    we::platform::NativeWindowHandle window{};
    uint32_t adapterIndex = 0;
    uint32_t framesInFlight = 2;
    bool enableValidation = false;
    bool vsync = true;
    bool headless = false;
    Extent2D headlessExtent{1280, 720};
    const char* debugName = "RHIDevice";
};

struct SwapchainDesc {
    we::platform::NativeWindowHandle window{};
    Extent2D extent{};
    uint32_t imageCount = 3;
    Format preferredFormat = Format::B8G8R8A8_UNORM;
    bool vsync = true;
    const char* debugName = "Swapchain";
};

struct BufferDesc {
    uint64_t size = 0;
    BufferUsage usage = BufferUsage::None;
    MemoryUsage memory = MemoryUsage::GpuLocal;
    const char* debugName = nullptr;
};

struct TextureDesc {
    Extent3D extent{};
    Format format = Format::R8G8B8A8_UNORM;
    TextureUsage usage = TextureUsage::Sampled;
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;
    uint32_t sampleCount = 1;
    const char* debugName = nullptr;
};

struct TextureViewDesc {
    RHITextureHandle texture = RHITextureHandle::Invalid;
    Format format = Format::Unknown; // Unknown = inherit
    uint32_t baseMip = 0;
    uint32_t mipCount = 1;
    uint32_t baseLayer = 0;
    uint32_t layerCount = 1;
    const char* debugName = nullptr;
};

struct SamplerDesc {
    Filter magFilter = Filter::Linear;
    Filter minFilter = Filter::Linear;
    Filter mipFilter = Filter::Linear;
    AddressMode addressU = AddressMode::Repeat;
    AddressMode addressV = AddressMode::Repeat;
    AddressMode addressW = AddressMode::Repeat;
    bool anisotropy = true;
    float maxAnisotropy = 8.0f;
    CompareOp compareOp = CompareOp::Always;
    bool compareEnable = false;
    float minLod = 0.0f;
    float maxLod = 1000.0f;
    const char* debugName = nullptr;
};

struct ShaderDesc {
    ShaderStage stage = ShaderStage::Vertex;
    ShaderBytecodeFormat format = ShaderBytecodeFormat::Auto;
    std::span<const uint8_t> bytecode{};
    const char* entryPoint = "main";
    const char* debugName = nullptr;
};

struct VertexAttributeDesc {
    uint32_t location = 0;
    uint32_t binding = 0;
    Format format = Format::R32G32B32A32_SFLOAT;
    uint32_t offset = 0;
    // DX12 input layout; ignored by Vulkan (uses location). Defaults: TEXCOORD+location.
    const char* semanticName = nullptr;
    uint32_t semanticIndex = ~0u;
};

struct VertexBindingDesc {
    uint32_t binding = 0;
    uint32_t stride = 0;
    bool perInstance = false;
};

struct ColorAttachmentDesc {
    RHITextureHandle texture = RHITextureHandle::Invalid;
    LoadOp loadOp = LoadOp::Clear;
    StoreOp storeOp = StoreOp::Store;
    Color4f clearColor{0.0f, 0.0f, 0.0f, 1.0f};
};

struct DepthAttachmentDesc {
    RHITextureHandle texture = RHITextureHandle::Invalid;
    LoadOp loadOp = LoadOp::Clear;
    StoreOp storeOp = StoreOp::Store;
    float clearDepth = 1.0f;
    uint32_t clearStencil = 0;
    bool enabled = false;
};

struct RenderingInfo {
    std::vector<ColorAttachmentDesc> colorAttachments{};
    DepthAttachmentDesc depth{};
    Extent2D renderArea{};
};

struct BlendStateDesc {
    bool enable = false;
    BlendFactor srcColor = BlendFactor::SrcAlpha;
    BlendFactor dstColor = BlendFactor::OneMinusSrcAlpha;
    BlendOp colorOp = BlendOp::Add;
    BlendFactor srcAlpha = BlendFactor::One;
    BlendFactor dstAlpha = BlendFactor::OneMinusSrcAlpha;
    BlendOp alphaOp = BlendOp::Add;
};

struct GraphicsPipelineDesc {
    RHIShaderHandle vertexShader = RHIShaderHandle::Invalid;
    RHIShaderHandle fragmentShader = RHIShaderHandle::Invalid;
    RHIPipelineLayoutHandle layout = RHIPipelineLayoutHandle::Invalid;
    std::vector<VertexBindingDesc> vertexBindings{};
    std::vector<VertexAttributeDesc> vertexAttributes{};
    PrimitiveTopology topology = PrimitiveTopology::TriangleList;
    CullMode cullMode = CullMode::Back;
    bool depthTest = true;
    bool depthWrite = true;
    CompareOp depthCompare = CompareOp::Less;
    BlendStateDesc blend{};
    Format colorFormat = Format::B8G8R8A8_UNORM;
    Format depthFormat = Format::D32_SFLOAT;
    bool depthAttachment = true;
    const char* debugName = nullptr;
};

struct ComputePipelineDesc {
    RHIShaderHandle computeShader = RHIShaderHandle::Invalid;
    RHIPipelineLayoutHandle layout = RHIPipelineLayoutHandle::Invalid;
    const char* debugName = nullptr;
};

struct DescriptorBindingDesc {
    uint32_t binding = 0;
    DescriptorType type = DescriptorType::UniformBuffer;
    uint32_t count = 1;
    ShaderStageFlags stages = ShaderStageFlags::All;
};

struct DescriptorSetLayoutDesc {
    std::vector<DescriptorBindingDesc> bindings{};
    const char* debugName = nullptr;
};

struct PushConstantRangeDesc {
    ShaderStageFlags stages = ShaderStageFlags::All;
    uint32_t offset = 0;
    uint32_t size = 0;
};

struct PipelineLayoutDesc {
    std::vector<RHIDescriptorSetLayoutHandle> setLayouts{};
    std::vector<PushConstantRangeDesc> pushConstantRanges{};
    // Retained for simple layouts that only need a single push-constant block.
    uint32_t pushConstantSize = 0;
    ShaderStageFlags pushConstantStages = ShaderStageFlags::All;
    const char* debugName = nullptr;
};

struct DescriptorPoolSizeDesc {
    DescriptorType type = DescriptorType::UniformBuffer;
    uint32_t count = 0;
};

struct DescriptorPoolDesc {
    uint32_t maxSets = 1;
    std::vector<DescriptorPoolSizeDesc> poolSizes{};
    const char* debugName = nullptr;
};

struct DescriptorSetAllocateDesc {
    RHIDescriptorPoolHandle pool = RHIDescriptorPoolHandle::Invalid;
    RHIDescriptorSetLayoutHandle layout = RHIDescriptorSetLayoutHandle::Invalid;
    const char* debugName = nullptr;
};

struct DescriptorBufferInfo {
    RHIBufferHandle buffer = RHIBufferHandle::Invalid;
    uint64_t offset = 0;
    uint64_t range = ~0ull; // ~0 = whole buffer from offset
};

struct DescriptorImageInfo {
    RHISamplerHandle sampler = RHISamplerHandle::Invalid;
    RHITextureViewHandle view = RHITextureViewHandle::Invalid;
    ResourceState imageLayout = ResourceState::ShaderResource;
};

struct WriteDescriptorSet {
    RHIDescriptorSetHandle set = RHIDescriptorSetHandle::Invalid;
    uint32_t binding = 0;
    uint32_t arrayElement = 0;
    DescriptorType type = DescriptorType::UniformBuffer;
    uint32_t count = 1;
    const DescriptorBufferInfo* bufferInfos = nullptr;
    const DescriptorImageInfo* imageInfos = nullptr;
};

struct TextureSubresourceLayers {
    uint32_t mipLevel = 0;
    uint32_t baseLayer = 0;
    uint32_t layerCount = 1;
};

struct TextureCopyRegion {
    TextureSubresourceLayers src{};
    uint32_t srcOffsetX = 0;
    uint32_t srcOffsetY = 0;
    uint32_t srcOffsetZ = 0;
    TextureSubresourceLayers dst{};
    uint32_t dstOffsetX = 0;
    uint32_t dstOffsetY = 0;
    uint32_t dstOffsetZ = 0;
    Extent3D extent{1, 1, 1};
};

struct BufferImageCopyRegion {
    uint64_t bufferOffset = 0;
    uint32_t bufferRowLength = 0;   // 0 = tightly packed
    uint32_t bufferImageHeight = 0; // 0 = tightly packed
    TextureSubresourceLayers image{};
    uint32_t imageOffsetX = 0;
    uint32_t imageOffsetY = 0;
    uint32_t imageOffsetZ = 0;
    Extent3D imageExtent{1, 1, 1};
};

struct TextureBlitRegion {
    TextureSubresourceLayers src{};
    int32_t srcOffset0X = 0;
    int32_t srcOffset0Y = 0;
    int32_t srcOffset0Z = 0;
    int32_t srcOffset1X = 0;
    int32_t srcOffset1Y = 0;
    int32_t srcOffset1Z = 0;
    TextureSubresourceLayers dst{};
    int32_t dstOffset0X = 0;
    int32_t dstOffset0Y = 0;
    int32_t dstOffset0Z = 0;
    int32_t dstOffset1X = 0;
    int32_t dstOffset1Y = 0;
    int32_t dstOffset1Z = 0;
};

struct TextureUpdateDesc {
    uint32_t mipLevel = 0;
    uint32_t arrayLayer = 0;
    uint32_t offsetX = 0;
    uint32_t offsetY = 0;
    uint32_t offsetZ = 0;
    Extent3D extent{1, 1, 1};
    std::span<const uint8_t> data{};
    uint32_t rowPitch = 0; // 0 = tightly packed
};

struct TextureBarrierDesc {
    RHITextureHandle texture = RHITextureHandle::Invalid;
    ResourceState before = ResourceState::Undefined;
    ResourceState after = ResourceState::Undefined;
    uint32_t baseMip = 0;
    uint32_t mipCount = ~0u;
    uint32_t baseLayer = 0;
    uint32_t layerCount = ~0u;
};

struct BufferBarrierDesc {
    RHIBufferHandle buffer = RHIBufferHandle::Invalid;
    ResourceState before = ResourceState::Undefined;
    ResourceState after = ResourceState::Undefined;
    uint64_t offset = 0;
    uint64_t size = ~0ull;
};

struct ResourceBarrierDesc {
    TextureBarrierDesc texture{};
    BufferBarrierDesc buffer{};
    bool isTexture = true;
};

struct FenceDesc {
    bool signaled = false;
    const char* debugName = nullptr;
};

struct SemaphoreDesc {
    const char* debugName = nullptr;
};

struct SubmitDesc {
    IRHICommandList* commandList = nullptr;
    std::span<const RHISemaphoreHandle> waitSemaphores{};
    std::span<const RHISemaphoreHandle> signalSemaphores{};
    RHIFenceHandle signalFence = RHIFenceHandle::Invalid;
};

} // namespace we::rhi
