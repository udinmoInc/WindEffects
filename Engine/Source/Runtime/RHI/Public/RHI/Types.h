#pragma once

#include "RHI/Export.h"

#include <cstdint>

namespace we::rhi {

enum class RHIBackend : uint8_t {
    Auto = 0,
    Null,
    Vulkan,
    DirectX12,
    DirectX11,
    Metal,
    OpenGL,
    OpenGLES
};

[[nodiscard]] RHI_API const char* ToString(RHIBackend backend) noexcept;

// Opaque GPU resource handles — never map to API objects outside backend modules.
enum class RHIBufferHandle : uint64_t { Invalid = 0 };
enum class RHITextureHandle : uint64_t { Invalid = 0 };
enum class RHITextureViewHandle : uint64_t { Invalid = 0 };
enum class RHISamplerHandle : uint64_t { Invalid = 0 };
enum class RHIShaderHandle : uint64_t { Invalid = 0 };
enum class RHIPipelineLayoutHandle : uint64_t { Invalid = 0 };
enum class RHIGraphicsPipelineHandle : uint64_t { Invalid = 0 };
enum class RHIComputePipelineHandle : uint64_t { Invalid = 0 };
enum class RHIDescriptorSetLayoutHandle : uint64_t { Invalid = 0 };
enum class RHIDescriptorSetHandle : uint64_t { Invalid = 0 };
enum class RHIDescriptorPoolHandle : uint64_t { Invalid = 0 };
enum class RHIFenceHandle : uint64_t { Invalid = 0 };
enum class RHISemaphoreHandle : uint64_t { Invalid = 0 };
enum class RHIQueryPoolHandle : uint64_t { Invalid = 0 };
enum class RHICommandPoolHandle : uint64_t { Invalid = 0 };
enum class RHIFramebufferHandle : uint64_t { Invalid = 0 };
enum class RHIRenderPassHandle : uint64_t { Invalid = 0 };
enum class RHIPipelineCacheHandle : uint64_t { Invalid = 0 };

enum class QueueType : uint8_t {
    Graphics = 0,
    Compute,
    Transfer,
    Present
};

enum class Format : uint32_t {
    Unknown = 0,
    R8_UNORM,
    R8G8B8A8_UNORM,
    R8G8B8A8_SRGB,
    B8G8R8A8_UNORM,
    B8G8R8A8_SRGB,
    R16G16B16A16_SFLOAT,
    R32_SFLOAT,
    R32G32_SFLOAT,
    R32G32B32_SFLOAT,
    R32G32B32A32_SFLOAT,
    D32_SFLOAT,
    D24_UNORM_S8_UINT,
    D32_SFLOAT_S8_UINT
};

enum class BufferUsage : uint32_t {
    None            = 0,
    Vertex          = 1u << 0,
    Index           = 1u << 1,
    Uniform         = 1u << 2,
    Storage         = 1u << 3,
    TransferSrc     = 1u << 4,
    TransferDst     = 1u << 5,
    Indirect        = 1u << 6
};

[[nodiscard]] constexpr BufferUsage operator|(BufferUsage a, BufferUsage b) noexcept {
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
[[nodiscard]] constexpr BufferUsage operator&(BufferUsage a, BufferUsage b) noexcept {
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
[[nodiscard]] constexpr bool HasFlag(BufferUsage mask, BufferUsage bit) noexcept {
    return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(bit)) != 0;
}

enum class TextureUsage : uint32_t {
    None            = 0,
    Sampled         = 1u << 0,
    Storage         = 1u << 1,
    ColorAttachment = 1u << 2,
    DepthStencil    = 1u << 3,
    TransferSrc     = 1u << 4,
    TransferDst     = 1u << 5,
    Present         = 1u << 6
};

[[nodiscard]] constexpr TextureUsage operator|(TextureUsage a, TextureUsage b) noexcept {
    return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
[[nodiscard]] constexpr TextureUsage operator&(TextureUsage a, TextureUsage b) noexcept {
    return static_cast<TextureUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
[[nodiscard]] constexpr bool HasFlag(TextureUsage mask, TextureUsage bit) noexcept {
    return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(bit)) != 0;
}

enum class ResourceState : uint32_t {
    Undefined = 0,
    General,
    VertexBuffer,
    IndexBuffer,
    UniformBuffer,
    ShaderResource,
    UnorderedAccess,
    RenderTarget,
    DepthWrite,
    DepthRead,
    CopySrc,
    CopyDst,
    Present
};

enum class MemoryUsage : uint8_t {
    GpuLocal = 0,
    HostVisible,
    HostCached
};

enum class PrimitiveTopology : uint8_t {
    TriangleList = 0,
    TriangleStrip,
    LineList,
    PointList
};

enum class CullMode : uint8_t {
    None = 0,
    Front,
    Back
};

enum class CompareOp : uint8_t {
    Never = 0,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always
};

enum class LoadOp : uint8_t { Load = 0, Clear, DontCare };
enum class StoreOp : uint8_t { Store = 0, DontCare };

enum class IndexType : uint8_t { UInt16 = 0, UInt32 };

enum class DescriptorType : uint8_t {
    Sampler = 0,
    CombinedImageSampler,
    SampledImage,
    StorageImage,
    UniformBuffer,
    StorageBuffer,
    UniformBufferDynamic,
    StorageBufferDynamic
};

enum class ShaderStageFlags : uint32_t {
    None           = 0,
    Vertex         = 1u << 0,
    Fragment       = 1u << 1,
    Compute        = 1u << 2,
    Geometry       = 1u << 3,
    TessControl    = 1u << 4,
    TessEvaluation = 1u << 5,
    AllGraphics    = Vertex | Fragment | Geometry | TessControl | TessEvaluation,
    All            = AllGraphics | Compute
};

[[nodiscard]] constexpr ShaderStageFlags operator|(ShaderStageFlags a, ShaderStageFlags b) noexcept {
    return static_cast<ShaderStageFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
[[nodiscard]] constexpr ShaderStageFlags operator&(ShaderStageFlags a, ShaderStageFlags b) noexcept {
    return static_cast<ShaderStageFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
[[nodiscard]] constexpr bool HasFlag(ShaderStageFlags mask, ShaderStageFlags bit) noexcept {
    return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(bit)) != 0;
}

enum class Filter : uint8_t { Nearest = 0, Linear };
enum class AddressMode : uint8_t { Repeat = 0, MirroredRepeat, ClampToEdge, ClampToBorder };
enum class BlendFactor : uint8_t {
    Zero = 0,
    One,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    SrcColor,
    OneMinusSrcColor
};
enum class BlendOp : uint8_t { Add = 0, Subtract, ReverseSubtract, Min, Max };

enum class PipelineBindPoint : uint8_t { Graphics = 0, Compute };

struct Extent2D {
    uint32_t width = 0;
    uint32_t height = 0;
};

struct Extent3D {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
};

struct Color4f {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
};

struct Viewport {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

struct Scissor {
    int32_t x = 0;
    int32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

} // namespace we::rhi
