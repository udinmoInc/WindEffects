#pragma once

#include "Application/Export.h"

#include "Rendering/Text/GpuBatcher.h"

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

namespace we::UI::Text {

/**
 * @brief Graphics API type
 */
enum class GraphicsApi : uint8_t {
    Vulkan,
    DirectX12,
    Metal,
    OpenGL,
    Unknown
};

/**
 * @brief Texture format
 */
enum class TextureFormat : uint8_t {
    RGBA8_UNORM,      // Standard RGBA8 UNORM (for MSDF atlases)
    RGB8_UNORM,       // RGB8 UNORM
    R8_UNORM,         // Single channel UNORM
    RGBA8_SRGB,       // sRGB color space (not for MSDF)
    Unknown
};

/**
 * @brief Texture usage flags
 */
enum class TextureUsage : uint32_t {
    Sampled = 1 << 0,
    Storage = 1 << 1,
    RenderTarget = 1 << 2,
    TransferSrc = 1 << 3,
    TransferDst = 1 << 4
};

/**
 * @brief Buffer usage flags
 */
enum class BufferUsage : uint32_t {
    Vertex = 1 << 0,
    Index = 1 << 1,
    Uniform = 1 << 2,
    Storage = 1 << 3,
    TransferSrc = 1 << 4,
    TransferDst = 1 << 5
};

/**
 * @brief Memory property flags
 */
enum class MemoryProperty : uint32_t {
    DeviceLocal = 1 << 0,
    HostVisible = 1 << 1,
    HostCoherent = 1 << 2,
    HostCached = 1 << 3
};

/**
 * @brief Texture description
 */
struct TextureDesc {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;
    TextureFormat format = TextureFormat::RGBA8_UNORM;
    uint32_t usage = static_cast<uint32_t>(TextureUsage::Sampled) | 
                     static_cast<uint32_t>(TextureUsage::TransferDst);
    std::string name;
};

/**
 * @brief Buffer description
 */
struct BufferDesc {
    uint64_t size = 0;
    uint32_t usage = 0;
    uint32_t memoryProperties = static_cast<uint32_t>(MemoryProperty::DeviceLocal);
    std::string name;
};

/**
 * @brief Sampler description
 */
struct SamplerDesc {
    float minLod = 0.0f;
    float maxLod = 0.0f;
    float mipLodBias = 0.0f;
    float maxAnisotropy = 1.0f;
    bool anisotropyEnable = false;
    bool compareEnable = false;
    std::string name;
};

/**
 * @brief Texture handle (backend-agnostic)
 */
struct TextureHandle {
    uint64_t handle = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    TextureFormat format = TextureFormat::Unknown;
    void* apiSpecific = nullptr;  // API-specific pointer (e.g., VkImage, ID3D12Resource)
};

/**
 * @brief Buffer handle (backend-agnostic)
 */
struct BufferHandle {
    uint64_t handle = 0;
    uint64_t size = 0;
    void* mappedPtr = nullptr;
    void* apiSpecific = nullptr;
};

/**
 * @brief Sampler handle (backend-agnostic)
 */
struct SamplerHandle {
    uint64_t handle = 0;
    void* apiSpecific = nullptr;
};

/**
 * @brief Rendering backend interface
 * 
 * Provides graphics API-independent rendering interface.
 * Supports Vulkan, DirectX 12, Metal, and OpenGL.
 */
class APPLICATION_API IRenderingBackend {
public:
    virtual ~IRenderingBackend() = default;

    /**
     * @brief Initialize the rendering backend
     * @param api Graphics API to use
     * @return true if successful, false otherwise
     */
    virtual bool Initialize(GraphicsApi api) = 0;

    /**
     * @brief Shutdown the rendering backend
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Get the current graphics API
     * @return GraphicsApi type
     */
    virtual GraphicsApi GetApi() const = 0;

    /**
     * @brief Create a texture
     * @param desc Texture description
     * @param initialData Initial texture data (can be null)
     * @param dataSize Size of initial data
     * @return TextureHandle
     */
    virtual TextureHandle CreateTexture(const TextureDesc& desc, const void* initialData = nullptr, 
                                        size_t dataSize = 0) = 0;

    /**
     * @brief Destroy a texture
     * @param texture Texture handle
     */
    virtual void DestroyTexture(TextureHandle texture) = 0;

    /**
     * @brief Update texture data
     * @param texture Texture handle
     * @param data New texture data
     * @param dataSize Size of data
     * @param offsetX X offset (for partial updates)
     * @param offsetY Y offset
     * @param width Width of region to update
     * @param height Height of region to update
     * @return true if successful, false otherwise
     */
    virtual bool UpdateTexture(TextureHandle texture, const void* data, size_t dataSize,
                              uint32_t offsetX = 0, uint32_t offsetY = 0,
                              uint32_t width = 0, uint32_t height = 0) = 0;

    /**
     * @brief Create a buffer
     * @param desc Buffer description
     * @param initialData Initial buffer data (can be null)
     * @param dataSize Size of initial data
     * @return BufferHandle
     */
    virtual BufferHandle CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr,
                                      size_t dataSize = 0) = 0;

    /**
     * @brief Destroy a buffer
     * @param buffer Buffer handle
     */
    virtual void DestroyBuffer(BufferHandle buffer) = 0;

    /**
     * @brief Update buffer data
     * @param buffer Buffer handle
     * @param data New buffer data
     * @param dataSize Size of data
     * @param offset Offset into buffer
     * @return true if successful, false otherwise
     */
    virtual bool UpdateBuffer(BufferHandle buffer, const void* data, size_t dataSize,
                              uint64_t offset = 0) = 0;

    /**
     * @brief Map buffer memory
     * @param buffer Buffer handle
     * @param offset Offset into buffer
     * @param size Size to map
     * @return Mapped pointer, or nullptr if failed
     */
    virtual void* MapBuffer(BufferHandle buffer, uint64_t offset = 0, uint64_t size = 0) = 0;

    /**
     * @brief Unmap buffer memory
     * @param buffer Buffer handle
     */
    virtual void UnmapBuffer(BufferHandle buffer) = 0;

    /**
     * @brief Create a sampler
     * @param desc Sampler description
     * @return SamplerHandle
     */
    virtual SamplerHandle CreateSampler(const SamplerDesc& desc) = 0;

    /**
     * @brief Destroy a sampler
     * @param sampler Sampler handle
     */
    virtual void DestroySampler(SamplerHandle sampler) = 0;

    /**
     * @brief Begin a command buffer
     * @return Command buffer handle (backend-specific)
     */
    virtual void* BeginCommandBuffer() = 0;

    /**
     * @brief End a command buffer
     * @param cmdBuffer Command buffer handle
     */
    virtual void EndCommandBuffer(void* cmdBuffer) = 0;

    /**
     * @brief Submit command buffer for execution
     * @param cmdBuffer Command buffer handle
     * @return true if successful, false otherwise
     */
    virtual bool SubmitCommandBuffer(void* cmdBuffer) = 0;

    /**
     * @brief Create MSDF atlas texture
     * @param pixels Atlas pixel data (RGBA8)
     * @param width Atlas width
     * @param height Atlas height
     * @return TextureHandle
     */
    virtual TextureHandle CreateMsdfAtlas(const std::vector<uint8_t>& pixels, 
                                          int width, int height) = 0;

    /**
     * @brief Create vertex buffer for text rendering
     * @param vertices Vertex data
     * @return BufferHandle
     */
    virtual BufferHandle CreateVertexBuffer(const std::vector<TextVertex>& vertices) = 0;

    /**
     * @brief Create index buffer for text rendering
     * @param indices Index data
     * @return BufferHandle
     */
    virtual BufferHandle CreateIndexBuffer(const std::vector<uint16_t>& indices) = 0;

    /**
     * @brief Get API-specific device handle
     * @return API-specific device pointer
     */
    virtual void* GetDeviceHandle() const = 0;

    /**
     * @brief Check if backend is initialized
     * @return true if initialized, false otherwise
     */
    virtual bool IsInitialized() const = 0;

    /**
     * @brief Get backend capabilities
     * @return String describing capabilities
     */
    virtual std::string GetCapabilities() const = 0;
};

/**
 * @brief Vulkan rendering backend implementation
 */
class APPLICATION_API VulkanBackend : public IRenderingBackend {
public:
    VulkanBackend();
    ~VulkanBackend() override;

    bool Initialize(GraphicsApi api) override;
    void Shutdown() override;

    GraphicsApi GetApi() const override { return GraphicsApi::Vulkan; }

    TextureHandle CreateTexture(const TextureDesc& desc, const void* initialData = nullptr,
                                size_t dataSize = 0) override;
    void DestroyTexture(TextureHandle texture) override;

    bool UpdateTexture(TextureHandle texture, const void* data, size_t dataSize,
                      uint32_t offsetX = 0, uint32_t offsetY = 0,
                      uint32_t width = 0, uint32_t height = 0) override;

    BufferHandle CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr,
                              size_t dataSize = 0) override;
    void DestroyBuffer(BufferHandle buffer) override;

    bool UpdateBuffer(BufferHandle buffer, const void* data, size_t dataSize,
                      uint64_t offset = 0) override;

    void* MapBuffer(BufferHandle buffer, uint64_t offset = 0, uint64_t size = 0) override;
    void UnmapBuffer(BufferHandle buffer) override;

    SamplerHandle CreateSampler(const SamplerDesc& desc) override;
    void DestroySampler(SamplerHandle sampler) override;

    void* BeginCommandBuffer() override;
    void EndCommandBuffer(void* cmdBuffer) override;
    bool SubmitCommandBuffer(void* cmdBuffer) override;

    TextureHandle CreateMsdfAtlas(const std::vector<uint8_t>& pixels, int width, int height) override;
    BufferHandle CreateVertexBuffer(const std::vector<TextVertex>& vertices) override;
    BufferHandle CreateIndexBuffer(const std::vector<uint16_t>& indices) override;

    void* GetDeviceHandle() const override;
    bool IsInitialized() const override;
    std::string GetCapabilities() const override;

private:
    void* m_VulkanDevice = nullptr;  // VkDevice
    void* m_VulkanInstance = nullptr;  // VkInstance
    void* m_VulkanPhysicalDevice = nullptr;  // VkPhysicalDevice
    void* m_VulkanQueue = nullptr;  // VkQueue
    void* m_VulkanCommandPool = nullptr;  // VkCommandPool
    bool m_Initialized = false;
};

/**
 * @brief DirectX 12 rendering backend implementation
 */
class APPLICATION_API DirectX12Backend : public IRenderingBackend {
public:
    DirectX12Backend();
    ~DirectX12Backend() override;

    bool Initialize(GraphicsApi api) override;
    void Shutdown() override;

    GraphicsApi GetApi() const override { return GraphicsApi::DirectX12; }

    TextureHandle CreateTexture(const TextureDesc& desc, const void* initialData = nullptr,
                                size_t dataSize = 0) override;
    void DestroyTexture(TextureHandle texture) override;

    bool UpdateTexture(TextureHandle texture, const void* data, size_t dataSize,
                      uint32_t offsetX = 0, uint32_t offsetY = 0,
                      uint32_t width = 0, uint32_t height = 0) override;

    BufferHandle CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr,
                              size_t dataSize = 0) override;
    void DestroyBuffer(BufferHandle buffer) override;

    bool UpdateBuffer(BufferHandle buffer, const void* data, size_t dataSize,
                      uint64_t offset = 0) override;

    void* MapBuffer(BufferHandle buffer, uint64_t offset = 0, uint64_t size = 0) override;
    void UnmapBuffer(BufferHandle buffer) override;

    SamplerHandle CreateSampler(const SamplerDesc& desc) override;
    void DestroySampler(SamplerHandle sampler) override;

    void* BeginCommandBuffer() override;
    void EndCommandBuffer(void* cmdBuffer) override;
    bool SubmitCommandBuffer(void* cmdBuffer) override;

    TextureHandle CreateMsdfAtlas(const std::vector<uint8_t>& pixels, int width, int height) override;
    BufferHandle CreateVertexBuffer(const std::vector<TextVertex>& vertices) override;
    BufferHandle CreateIndexBuffer(const std::vector<uint16_t>& indices) override;

    void* GetDeviceHandle() const override;
    bool IsInitialized() const override;
    std::string GetCapabilities() const override;

private:
    void* m_D3D12Device = nullptr;  // ID3D12Device
    void* m_D3D12CommandQueue = nullptr;  // ID3D12CommandQueue
    void* m_D3D12CommandAllocator = nullptr;  // ID3D12CommandAllocator
    bool m_Initialized = false;
};

/**
 * @brief Metal rendering backend implementation
 */
class APPLICATION_API MetalBackend : public IRenderingBackend {
public:
    MetalBackend();
    ~MetalBackend() override;

    bool Initialize(GraphicsApi api) override;
    void Shutdown() override;

    GraphicsApi GetApi() const override { return GraphicsApi::Metal; }

    TextureHandle CreateTexture(const TextureDesc& desc, const void* initialData = nullptr,
                                size_t dataSize = 0) override;
    void DestroyTexture(TextureHandle texture) override;

    bool UpdateTexture(TextureHandle texture, const void* data, size_t dataSize,
                      uint32_t offsetX = 0, uint32_t offsetY = 0,
                      uint32_t width = 0, uint32_t height = 0) override;

    BufferHandle CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr,
                              size_t dataSize = 0) override;
    void DestroyBuffer(BufferHandle buffer) override;

    bool UpdateBuffer(BufferHandle buffer, const void* data, size_t dataSize,
                      uint64_t offset = 0) override;

    void* MapBuffer(BufferHandle buffer, uint64_t offset = 0, uint64_t size = 0) override;
    void UnmapBuffer(BufferHandle buffer) override;

    SamplerHandle CreateSampler(const SamplerDesc& desc) override;
    void DestroySampler(SamplerHandle sampler) override;

    void* BeginCommandBuffer() override;
    void EndCommandBuffer(void* cmdBuffer) override;
    bool SubmitCommandBuffer(void* cmdBuffer) override;

    TextureHandle CreateMsdfAtlas(const std::vector<uint8_t>& pixels, int width, int height) override;
    BufferHandle CreateVertexBuffer(const std::vector<TextVertex>& vertices) override;
    BufferHandle CreateIndexBuffer(const std::vector<uint16_t>& indices) override;

    void* GetDeviceHandle() const override;
    bool IsInitialized() const override;
    std::string GetCapabilities() const override;

private:
    void* m_MetalDevice = nullptr;  // id<MTLDevice>
    void* m_MetalCommandQueue = nullptr;  // id<MTLCommandQueue>
    bool m_Initialized = false;
};

/**
 * @brief OpenGL rendering backend implementation
 */
class APPLICATION_API OpenGLBackend : public IRenderingBackend {
public:
    OpenGLBackend();
    ~OpenGLBackend() override;

    bool Initialize(GraphicsApi api) override;
    void Shutdown() override;

    GraphicsApi GetApi() const override { return GraphicsApi::OpenGL; }

    TextureHandle CreateTexture(const TextureDesc& desc, const void* initialData = nullptr,
                                size_t dataSize = 0) override;
    void DestroyTexture(TextureHandle texture) override;

    bool UpdateTexture(TextureHandle texture, const void* data, size_t dataSize,
                      uint32_t offsetX = 0, uint32_t offsetY = 0,
                      uint32_t width = 0, uint32_t height = 0) override;

    BufferHandle CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr,
                              size_t dataSize = 0) override;
    void DestroyBuffer(BufferHandle buffer) override;

    bool UpdateBuffer(BufferHandle buffer, const void* data, size_t dataSize,
                      uint64_t offset = 0) override;

    void* MapBuffer(BufferHandle buffer, uint64_t offset = 0, uint64_t size = 0) override;
    void UnmapBuffer(BufferHandle buffer) override;

    SamplerHandle CreateSampler(const SamplerDesc& desc) override;
    void DestroySampler(SamplerHandle sampler) override;

    void* BeginCommandBuffer() override;
    void EndCommandBuffer(void* cmdBuffer) override;
    bool SubmitCommandBuffer(void* cmdBuffer) override;

    TextureHandle CreateMsdfAtlas(const std::vector<uint8_t>& pixels, int width, int height) override;
    BufferHandle CreateVertexBuffer(const std::vector<TextVertex>& vertices) override;
    BufferHandle CreateIndexBuffer(const std::vector<uint16_t>& indices) override;

    void* GetDeviceHandle() const override;
    bool IsInitialized() const override;
    std::string GetCapabilities() const override;

private:
    uint32_t m_GLVertexArray = 0;
    bool m_Initialized = false;
};

/**
 * @brief Rendering backend factory
 */
class APPLICATION_API RenderingBackendFactory {
public:
    /**
     * @brief Create a rendering backend for the specified API
     * @param api Graphics API
     * @return Unique pointer to backend, or nullptr if unsupported
     */
    static std::unique_ptr<IRenderingBackend> CreateBackend(GraphicsApi api);

    /**
     * @brief Get the default graphics API for the current platform
     * @return GraphicsApi type
     */
    static GraphicsApi GetDefaultApi();

    /**
     * @brief Check if an API is supported on the current platform
     * @param api Graphics API
     * @return true if supported, false otherwise
     */
    static bool IsApiSupported(GraphicsApi api);

    /**
     * @brief Get list of supported APIs
     * @return Vector of supported APIs
     */
    static std::vector<GraphicsApi> GetSupportedApis();
};

} // namespace we::UI::Text
