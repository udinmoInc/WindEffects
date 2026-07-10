#include "Rendering/Text/RenderingBackend.h"
#include "Core/Logger.h"

#include <algorithm>

namespace we::UI::Text {

// ============================================================================
// Vulkan Backend Implementation
// ============================================================================

VulkanBackend::VulkanBackend() = default;

VulkanBackend::~VulkanBackend() {
    Shutdown();
}

bool VulkanBackend::Initialize(GraphicsApi api) {
    if (m_Initialized) {
        HE_WARN("VulkanBackend: Already initialized");
        return true;
    }

    if (api != GraphicsApi::Vulkan) {
        HE_ERROR("VulkanBackend: Requested API is not Vulkan");
        return false;
    }

    // Initialize Vulkan (placeholder - would need actual Vulkan initialization)
    // This would include:
    // - Create VkInstance
    // - Select physical device
    // - Create logical device
    // - Create command pool
    // - Get queue handles
    
    HE_INFO("VulkanBackend: Initialized (placeholder implementation)");
    m_Initialized = true;
    return true;
}

void VulkanBackend::Shutdown() {
    if (!m_Initialized) {
        return;
    }

    // Cleanup Vulkan resources (placeholder)
    m_VulkanDevice = nullptr;
    m_VulkanInstance = nullptr;
    m_VulkanPhysicalDevice = nullptr;
    m_VulkanQueue = nullptr;
    m_VulkanCommandPool = nullptr;
    m_Initialized = false;

    HE_INFO("VulkanBackend: Shutdown");
}

TextureHandle VulkanBackend::CreateTexture(const TextureDesc& desc, const void* initialData,
                                           size_t dataSize) {
    TextureHandle handle;
    handle.width = desc.width;
    handle.height = desc.height;
    handle.format = desc.format;
    
    // Create Vulkan texture (placeholder)
    handle.handle = 1;
    handle.apiSpecific = nullptr;
    
    HE_INFO("VulkanBackend: Created texture " + desc.name + " (" + 
            std::to_string(desc.width) + "x" + std::to_string(desc.height) + ")");
    
    return handle;
}

void VulkanBackend::DestroyTexture(TextureHandle texture) {
    // Destroy Vulkan texture (placeholder)
    HE_INFO("VulkanBackend: Destroyed texture");
}

bool VulkanBackend::UpdateTexture(TextureHandle texture, const void* data, size_t dataSize,
                                  uint32_t offsetX, uint32_t offsetY,
                                  uint32_t width, uint32_t height) {
    // Update Vulkan texture (placeholder)
    return true;
}

BufferHandle VulkanBackend::CreateBuffer(const BufferDesc& desc, const void* initialData,
                                        size_t dataSize) {
    BufferHandle handle;
    handle.size = desc.size;
    
    // Create Vulkan buffer (placeholder)
    handle.handle = 1;
    handle.apiSpecific = nullptr;
    
    HE_INFO("VulkanBackend: Created buffer " + desc.name + " (" + 
            std::to_string(desc.size) + " bytes)");
    
    return handle;
}

void VulkanBackend::DestroyBuffer(BufferHandle buffer) {
    // Destroy Vulkan buffer (placeholder)
    HE_INFO("VulkanBackend: Destroyed buffer");
}

bool VulkanBackend::UpdateBuffer(BufferHandle buffer, const void* data, size_t dataSize,
                                  uint64_t offset) {
    // Update Vulkan buffer (placeholder)
    return true;
}

void* VulkanBackend::MapBuffer(BufferHandle buffer, uint64_t offset, uint64_t size) {
    // Map Vulkan buffer (placeholder)
    return nullptr;
}

void VulkanBackend::UnmapBuffer(BufferHandle buffer) {
    // Unmap Vulkan buffer (placeholder)
}

SamplerHandle VulkanBackend::CreateSampler(const SamplerDesc& desc) {
    SamplerHandle handle;
    
    // Create Vulkan sampler (placeholder)
    handle.handle = 1;
    handle.apiSpecific = nullptr;
    
    HE_INFO("VulkanBackend: Created sampler " + desc.name);
    
    return handle;
}

void VulkanBackend::DestroySampler(SamplerHandle sampler) {
    // Destroy Vulkan sampler (placeholder)
    HE_INFO("VulkanBackend: Destroyed sampler");
}

void* VulkanBackend::BeginCommandBuffer() {
    // Begin Vulkan command buffer (placeholder)
    return nullptr;
}

void VulkanBackend::EndCommandBuffer(void* cmdBuffer) {
    // End Vulkan command buffer (placeholder)
}

bool VulkanBackend::SubmitCommandBuffer(void* cmdBuffer) {
    // Submit Vulkan command buffer (placeholder)
    return true;
}

TextureHandle VulkanBackend::CreateMsdfAtlas(const std::vector<uint8_t>& pixels, int width, int height) {
    TextureDesc desc;
    desc.width = width;
    desc.height = height;
    desc.format = TextureFormat::RGBA8_UNORM;
    desc.usage = static_cast<uint32_t>(TextureUsage::Sampled) | 
                 static_cast<uint32_t>(TextureUsage::TransferDst);
    desc.name = "MSDFAtlas";
    
    return CreateTexture(desc, pixels.data(), pixels.size());
}

BufferHandle VulkanBackend::CreateVertexBuffer(const std::vector<TextVertex>& vertices) {
    BufferDesc desc;
    desc.size = vertices.size() * sizeof(TextVertex);
    desc.usage = static_cast<uint32_t>(BufferUsage::Vertex) | 
                 static_cast<uint32_t>(BufferUsage::TransferDst);
    desc.memoryProperties = static_cast<uint32_t>(MemoryProperty::DeviceLocal);
    desc.name = "VertexBuffer";
    
    return CreateBuffer(desc, vertices.data(), desc.size);
}

BufferHandle VulkanBackend::CreateIndexBuffer(const std::vector<uint16_t>& indices) {
    BufferDesc desc;
    desc.size = indices.size() * sizeof(uint16_t);
    desc.usage = static_cast<uint32_t>(BufferUsage::Index) | 
                 static_cast<uint32_t>(BufferUsage::TransferDst);
    desc.memoryProperties = static_cast<uint32_t>(MemoryProperty::DeviceLocal);
    desc.name = "IndexBuffer";
    
    return CreateBuffer(desc, indices.data(), desc.size);
}

void* VulkanBackend::GetDeviceHandle() const {
    return m_VulkanDevice;
}

bool VulkanBackend::IsInitialized() const {
    return m_Initialized;
}

std::string VulkanBackend::GetCapabilities() const {
    return "Vulkan Backend (placeholder implementation)";
}

// ============================================================================
// DirectX 12 Backend Implementation
// ============================================================================

DirectX12Backend::DirectX12Backend() = default;

DirectX12Backend::~DirectX12Backend() {
    Shutdown();
}

bool DirectX12Backend::Initialize(GraphicsApi api) {
    if (m_Initialized) {
        HE_WARN("DirectX12Backend: Already initialized");
        return true;
    }

    if (api != GraphicsApi::DirectX12) {
        HE_ERROR("DirectX12Backend: Requested API is not DirectX 12");
        return false;
    }

    // Initialize DirectX 12 (placeholder)
    HE_INFO("DirectX12Backend: Initialized (placeholder implementation)");
    m_Initialized = true;
    return true;
}

void DirectX12Backend::Shutdown() {
    if (!m_Initialized) {
        return;
    }

    m_D3D12Device = nullptr;
    m_D3D12CommandQueue = nullptr;
    m_D3D12CommandAllocator = nullptr;
    m_Initialized = false;

    HE_INFO("DirectX12Backend: Shutdown");
}

TextureHandle DirectX12Backend::CreateTexture(const TextureDesc& desc, const void* initialData,
                                              size_t dataSize) {
    TextureHandle handle;
    handle.width = desc.width;
    handle.height = desc.height;
    handle.format = desc.format;
    handle.handle = 1;
    handle.apiSpecific = nullptr;
    
    HE_INFO("DirectX12Backend: Created texture " + desc.name);
    return handle;
}

void DirectX12Backend::DestroyTexture(TextureHandle texture) {
    HE_INFO("DirectX12Backend: Destroyed texture");
}

bool DirectX12Backend::UpdateTexture(TextureHandle texture, const void* data, size_t dataSize,
                                     uint32_t offsetX, uint32_t offsetY,
                                     uint32_t width, uint32_t height) {
    return true;
}

BufferHandle DirectX12Backend::CreateBuffer(const BufferDesc& desc, const void* initialData,
                                             size_t dataSize) {
    BufferHandle handle;
    handle.size = desc.size;
    handle.handle = 1;
    handle.apiSpecific = nullptr;
    
    HE_INFO("DirectX12Backend: Created buffer " + desc.name);
    return handle;
}

void DirectX12Backend::DestroyBuffer(BufferHandle buffer) {
    HE_INFO("DirectX12Backend: Destroyed buffer");
}

bool DirectX12Backend::UpdateBuffer(BufferHandle buffer, const void* data, size_t dataSize,
                                     uint64_t offset) {
    return true;
}

void* DirectX12Backend::MapBuffer(BufferHandle buffer, uint64_t offset, uint64_t size) {
    return nullptr;
}

void DirectX12Backend::UnmapBuffer(BufferHandle buffer) {
}

SamplerHandle DirectX12Backend::CreateSampler(const SamplerDesc& desc) {
    SamplerHandle handle;
    handle.handle = 1;
    handle.apiSpecific = nullptr;
    
    HE_INFO("DirectX12Backend: Created sampler " + desc.name);
    return handle;
}

void DirectX12Backend::DestroySampler(SamplerHandle sampler) {
    HE_INFO("DirectX12Backend: Destroyed sampler");
}

void* DirectX12Backend::BeginCommandBuffer() {
    return nullptr;
}

void DirectX12Backend::EndCommandBuffer(void* cmdBuffer) {
}

bool DirectX12Backend::SubmitCommandBuffer(void* cmdBuffer) {
    return true;
}

TextureHandle DirectX12Backend::CreateMsdfAtlas(const std::vector<uint8_t>& pixels, int width, int height) {
    TextureDesc desc;
    desc.width = width;
    desc.height = height;
    desc.format = TextureFormat::RGBA8_UNORM;
    desc.usage = static_cast<uint32_t>(TextureUsage::Sampled) | 
                 static_cast<uint32_t>(TextureUsage::TransferDst);
    desc.name = "MSDFAtlas";
    
    return CreateTexture(desc, pixels.data(), pixels.size());
}

BufferHandle DirectX12Backend::CreateVertexBuffer(const std::vector<TextVertex>& vertices) {
    BufferDesc desc;
    desc.size = vertices.size() * sizeof(TextVertex);
    desc.usage = static_cast<uint32_t>(BufferUsage::Vertex);
    desc.name = "VertexBuffer";
    
    return CreateBuffer(desc, vertices.data(), desc.size);
}

BufferHandle DirectX12Backend::CreateIndexBuffer(const std::vector<uint16_t>& indices) {
    BufferDesc desc;
    desc.size = indices.size() * sizeof(uint16_t);
    desc.usage = static_cast<uint32_t>(BufferUsage::Index);
    desc.name = "IndexBuffer";
    
    return CreateBuffer(desc, indices.data(), desc.size);
}

void* DirectX12Backend::GetDeviceHandle() const {
    return m_D3D12Device;
}

bool DirectX12Backend::IsInitialized() const {
    return m_Initialized;
}

std::string DirectX12Backend::GetCapabilities() const {
    return "DirectX 12 Backend (placeholder implementation)";
}

// ============================================================================
// Metal Backend Implementation
// ============================================================================

MetalBackend::MetalBackend() = default;

MetalBackend::~MetalBackend() {
    Shutdown();
}

bool MetalBackend::Initialize(GraphicsApi api) {
    if (m_Initialized) {
        HE_WARN("MetalBackend: Already initialized");
        return true;
    }

    if (api != GraphicsApi::Metal) {
        HE_ERROR("MetalBackend: Requested API is not Metal");
        return false;
    }

    // Initialize Metal (placeholder)
    HE_INFO("MetalBackend: Initialized (placeholder implementation)");
    m_Initialized = true;
    return true;
}

void MetalBackend::Shutdown() {
    if (!m_Initialized) {
        return;
    }

    m_MetalDevice = nullptr;
    m_MetalCommandQueue = nullptr;
    m_Initialized = false;

    HE_INFO("MetalBackend: Shutdown");
}

TextureHandle MetalBackend::CreateTexture(const TextureDesc& desc, const void* initialData,
                                           size_t dataSize) {
    TextureHandle handle;
    handle.width = desc.width;
    handle.height = desc.height;
    handle.format = desc.format;
    handle.handle = 1;
    handle.apiSpecific = nullptr;
    
    HE_INFO("MetalBackend: Created texture " + desc.name);
    return handle;
}

void MetalBackend::DestroyTexture(TextureHandle texture) {
    HE_INFO("MetalBackend: Destroyed texture");
}

bool MetalBackend::UpdateTexture(TextureHandle texture, const void* data, size_t dataSize,
                                  uint32_t offsetX, uint32_t offsetY,
                                  uint32_t width, uint32_t height) {
    return true;
}

BufferHandle MetalBackend::CreateBuffer(const BufferDesc& desc, const void* initialData,
                                        size_t dataSize) {
    BufferHandle handle;
    handle.size = desc.size;
    handle.handle = 1;
    handle.apiSpecific = nullptr;
    
    HE_INFO("MetalBackend: Created buffer " + desc.name);
    return handle;
}

void MetalBackend::DestroyBuffer(BufferHandle buffer) {
    HE_INFO("MetalBackend: Destroyed buffer");
}

bool MetalBackend::UpdateBuffer(BufferHandle buffer, const void* data, size_t dataSize,
                                  uint64_t offset) {
    return true;
}

void* MetalBackend::MapBuffer(BufferHandle buffer, uint64_t offset, uint64_t size) {
    return nullptr;
}

void MetalBackend::UnmapBuffer(BufferHandle buffer) {
}

SamplerHandle MetalBackend::CreateSampler(const SamplerDesc& desc) {
    SamplerHandle handle;
    handle.handle = 1;
    handle.apiSpecific = nullptr;
    
    HE_INFO("MetalBackend: Created sampler " + desc.name);
    return handle;
}

void MetalBackend::DestroySampler(SamplerHandle sampler) {
    HE_INFO("MetalBackend: Destroyed sampler");
}

void* MetalBackend::BeginCommandBuffer() {
    return nullptr;
}

void MetalBackend::EndCommandBuffer(void* cmdBuffer) {
}

bool MetalBackend::SubmitCommandBuffer(void* cmdBuffer) {
    return true;
}

TextureHandle MetalBackend::CreateMsdfAtlas(const std::vector<uint8_t>& pixels, int width, int height) {
    TextureDesc desc;
    desc.width = width;
    desc.height = height;
    desc.format = TextureFormat::RGBA8_UNORM;
    desc.usage = static_cast<uint32_t>(TextureUsage::Sampled);
    desc.name = "MSDFAtlas";
    
    return CreateTexture(desc, pixels.data(), pixels.size());
}

BufferHandle MetalBackend::CreateVertexBuffer(const std::vector<TextVertex>& vertices) {
    BufferDesc desc;
    desc.size = vertices.size() * sizeof(TextVertex);
    desc.usage = static_cast<uint32_t>(BufferUsage::Vertex);
    desc.name = "VertexBuffer";
    
    return CreateBuffer(desc, vertices.data(), desc.size);
}

BufferHandle MetalBackend::CreateIndexBuffer(const std::vector<uint16_t>& indices) {
    BufferDesc desc;
    desc.size = indices.size() * sizeof(uint16_t);
    desc.usage = static_cast<uint32_t>(BufferUsage::Index);
    desc.name = "IndexBuffer";
    
    return CreateBuffer(desc, indices.data(), desc.size);
}

void* MetalBackend::GetDeviceHandle() const {
    return m_MetalDevice;
}

bool MetalBackend::IsInitialized() const {
    return m_Initialized;
}

std::string MetalBackend::GetCapabilities() const {
    return "Metal Backend (placeholder implementation)";
}

// ============================================================================
// OpenGL Backend Implementation
// ============================================================================

OpenGLBackend::OpenGLBackend() = default;

OpenGLBackend::~OpenGLBackend() {
    Shutdown();
}

bool OpenGLBackend::Initialize(GraphicsApi api) {
    if (m_Initialized) {
        HE_WARN("OpenGLBackend: Already initialized");
        return true;
    }

    if (api != GraphicsApi::OpenGL) {
        HE_ERROR("OpenGLBackend: Requested API is not OpenGL");
        return false;
    }

    // Initialize OpenGL (placeholder)
    HE_INFO("OpenGLBackend: Initialized (placeholder implementation)");
    m_Initialized = true;
    return true;
}

void OpenGLBackend::Shutdown() {
    if (!m_Initialized) {
        return;
    }

    m_GLVertexArray = 0;
    m_Initialized = false;

    HE_INFO("OpenGLBackend: Shutdown");
}

TextureHandle OpenGLBackend::CreateTexture(const TextureDesc& desc, const void* initialData,
                                           size_t dataSize) {
    TextureHandle handle;
    handle.width = desc.width;
    handle.height = desc.height;
    handle.format = desc.format;
    handle.handle = 1;
    handle.apiSpecific = nullptr;
    
    HE_INFO("OpenGLBackend: Created texture " + desc.name);
    return handle;
}

void OpenGLBackend::DestroyTexture(TextureHandle texture) {
    HE_INFO("OpenGLBackend: Destroyed texture");
}

bool OpenGLBackend::UpdateTexture(TextureHandle texture, const void* data, size_t dataSize,
                                  uint32_t offsetX, uint32_t offsetY,
                                  uint32_t width, uint32_t height) {
    return true;
}

BufferHandle OpenGLBackend::CreateBuffer(const BufferDesc& desc, const void* initialData,
                                        size_t dataSize) {
    BufferHandle handle;
    handle.size = desc.size;
    handle.handle = 1;
    handle.apiSpecific = nullptr;
    
    HE_INFO("OpenGLBackend: Created buffer " + desc.name);
    return handle;
}

void OpenGLBackend::DestroyBuffer(BufferHandle buffer) {
    HE_INFO("OpenGLBackend: Destroyed buffer");
}

bool OpenGLBackend::UpdateBuffer(BufferHandle buffer, const void* data, size_t dataSize,
                                  uint64_t offset) {
    return true;
}

void* OpenGLBackend::MapBuffer(BufferHandle buffer, uint64_t offset, uint64_t size) {
    return nullptr;
}

void OpenGLBackend::UnmapBuffer(BufferHandle buffer) {
}

SamplerHandle OpenGLBackend::CreateSampler(const SamplerDesc& desc) {
    SamplerHandle handle;
    handle.handle = 1;
    handle.apiSpecific = nullptr;
    
    HE_INFO("OpenGLBackend: Created sampler " + desc.name);
    return handle;
}

void OpenGLBackend::DestroySampler(SamplerHandle sampler) {
    HE_INFO("OpenGLBackend: Destroyed sampler");
}

void* OpenGLBackend::BeginCommandBuffer() {
    return nullptr;
}

void OpenGLBackend::EndCommandBuffer(void* cmdBuffer) {
}

bool OpenGLBackend::SubmitCommandBuffer(void* cmdBuffer) {
    return true;
}

TextureHandle OpenGLBackend::CreateMsdfAtlas(const std::vector<uint8_t>& pixels, int width, int height) {
    TextureDesc desc;
    desc.width = width;
    desc.height = height;
    desc.format = TextureFormat::RGBA8_UNORM;
    desc.usage = static_cast<uint32_t>(TextureUsage::Sampled);
    desc.name = "MSDFAtlas";
    
    return CreateTexture(desc, pixels.data(), pixels.size());
}

BufferHandle OpenGLBackend::CreateVertexBuffer(const std::vector<TextVertex>& vertices) {
    BufferDesc desc;
    desc.size = vertices.size() * sizeof(TextVertex);
    desc.usage = static_cast<uint32_t>(BufferUsage::Vertex);
    desc.name = "VertexBuffer";
    
    return CreateBuffer(desc, vertices.data(), desc.size);
}

BufferHandle OpenGLBackend::CreateIndexBuffer(const std::vector<uint16_t>& indices) {
    BufferDesc desc;
    desc.size = indices.size() * sizeof(uint16_t);
    desc.usage = static_cast<uint32_t>(BufferUsage::Index);
    desc.name = "IndexBuffer";
    
    return CreateBuffer(desc, indices.data(), desc.size);
}

void* OpenGLBackend::GetDeviceHandle() const {
    return nullptr;
}

bool OpenGLBackend::IsInitialized() const {
    return m_Initialized;
}

std::string OpenGLBackend::GetCapabilities() const {
    return "OpenGL Backend (placeholder implementation)";
}

// ============================================================================
// Rendering Backend Factory Implementation
// ============================================================================

std::unique_ptr<IRenderingBackend> RenderingBackendFactory::CreateBackend(GraphicsApi api) {
    switch (api) {
        case GraphicsApi::Vulkan:
            return std::make_unique<VulkanBackend>();
        case GraphicsApi::DirectX12:
            return std::make_unique<DirectX12Backend>();
        case GraphicsApi::Metal:
            return std::make_unique<MetalBackend>();
        case GraphicsApi::OpenGL:
            return std::make_unique<OpenGLBackend>();
        default:
            HE_ERROR("RenderingBackendFactory: Unsupported API");
            return nullptr;
    }
}

GraphicsApi RenderingBackendFactory::GetDefaultApi() {
#ifdef _WIN32
    return GraphicsApi::DirectX12;
#elif defined(__APPLE__)
    return GraphicsApi::Metal;
#else
    return GraphicsApi::Vulkan;
#endif
}

bool RenderingBackendFactory::IsApiSupported(GraphicsApi api) {
    // Check platform support
#ifdef _WIN32
    return api == GraphicsApi::DirectX12 || api == GraphicsApi::Vulkan || api == GraphicsApi::OpenGL;
#elif defined(__APPLE__)
    return api == GraphicsApi::Metal || api == GraphicsApi::OpenGL;
#elif defined(__linux__)
    return api == GraphicsApi::Vulkan || api == GraphicsApi::OpenGL;
#else
    return api == GraphicsApi::OpenGL;
#endif
}

std::vector<GraphicsApi> RenderingBackendFactory::GetSupportedApis() {
    std::vector<GraphicsApi> apis;
    
    if (IsApiSupported(GraphicsApi::Vulkan)) apis.push_back(GraphicsApi::Vulkan);
    if (IsApiSupported(GraphicsApi::DirectX12)) apis.push_back(GraphicsApi::DirectX12);
    if (IsApiSupported(GraphicsApi::Metal)) apis.push_back(GraphicsApi::Metal);
    if (IsApiSupported(GraphicsApi::OpenGL)) apis.push_back(GraphicsApi::OpenGL);
    
    return apis;
}

} // namespace we::UI::Text
