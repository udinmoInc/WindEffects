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
    bool anisotropy = true;
    float maxAnisotropy = 8.0f;
    const char* debugName = nullptr;
};

struct ShaderDesc {
    ShaderStage stage = ShaderStage::Vertex;
    std::span<const uint8_t> bytecode{};
    const char* entryPoint = "main";
    const char* debugName = nullptr;
};

struct VertexAttributeDesc {
    uint32_t location = 0;
    uint32_t binding = 0;
    Format format = Format::R32G32B32A32_SFLOAT;
    uint32_t offset = 0;
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
    Format colorFormat = Format::B8G8R8A8_UNORM;
    Format depthFormat = Format::D32_SFLOAT;
    const char* debugName = nullptr;
};

struct ComputePipelineDesc {
    RHIShaderHandle computeShader = RHIShaderHandle::Invalid;
    RHIPipelineLayoutHandle layout = RHIPipelineLayoutHandle::Invalid;
    const char* debugName = nullptr;
};

struct PipelineLayoutDesc {
    // Placeholder for bindless / descriptor set layouts — extended as Renderer migrates.
    uint32_t pushConstantSize = 0;
    const char* debugName = nullptr;
};

} // namespace we::rhi
