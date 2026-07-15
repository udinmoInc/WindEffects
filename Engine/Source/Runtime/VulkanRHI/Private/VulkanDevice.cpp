#include "VulkanDevice.h"
#include "VulkanFormats.h"
#include "VulkanPlatformSurface.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>

namespace we::rhi::vulkan {
namespace {

bool AreValidationLayersAvailable() {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    if (layerCount == 0) {
        return false;
    }
    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
    for (const auto& layer : layers) {
        if (std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
            return true;
        }
    }
    return false;
}

bool ValidationRequestedFromEnvironment() {
    if (const char* env = std::getenv("WE_VULKAN_VALIDATION")) {
        return env[0] == '1' || std::strcmp(env, "true") == 0;
    }
    return false;
}

bool ValidationDisabledFromEnvironment() {
    if (const char* env = std::getenv("WE_VULKAN_VALIDATION")) {
        return env[0] == '0' || std::strcmp(env, "false") == 0;
    }
    return false;
}

uint32_t SelectInstanceApiVersion() {
    uint32_t requested = VK_API_VERSION_1_3;
    const auto enumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
        vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
    if (!enumerateInstanceVersion) {
        return requested;
    }
    uint32_t supported = 0;
    if (enumerateInstanceVersion(&supported) != VK_SUCCESS) {
        return requested;
    }
    return std::min(requested, supported);
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*)
{
    if (!pCallbackData || !pCallbackData->pMessage) {
        return VK_FALSE;
    }
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(), std::string("Vulkan: ") + pCallbackData->pMessage);
    }
    return VK_FALSE;
}

} // namespace

VulkanDevice::VulkanDevice(const DeviceDesc& desc)
    : m_Desc(desc)
    , m_FramesInFlight(desc.framesInFlight ? desc.framesInFlight : 2)
{
    const auto start = std::chrono::steady_clock::now();

    if (volkInitialize() != VK_SUCCESS) {
        WE_LOG_ERROR(we::LogCategory::Startup, "VulkanRHI: volkInitialize failed.");
        return;
    }

    if (auto r = CreateInstance(desc); !r) {
        WE_LOG_ERROR(we::LogCategory::Startup, r.error.message);
        return;
    }
    if (auto r = PickPhysicalDevice(desc.adapterIndex); !r) {
        WE_LOG_ERROR(we::LogCategory::Startup, r.error.message);
        return;
    }
    if (auto r = CreateLogicalDevice(); !r) {
        WE_LOG_ERROR(we::LogCategory::Startup, r.error.message);
        return;
    }
    if (auto r = CreateFrameSync(); !r) {
        WE_LOG_ERROR(we::LogCategory::Startup, r.error.message);
        return;
    }
    if (auto r = CreateCommandPool(); !r) {
        WE_LOG_ERROR(we::LogCategory::Startup, r.error.message);
        return;
    }

    m_Caps.dynamicRendering = true;
    m_Caps.debugMarkers = m_DebugMessenger != VK_NULL_HANDLE;
    m_Caps.maxFramesInFlight = m_FramesInFlight;
    m_Caps.timestamps = true;

    if (we::platform::IsValid(desc.window)) {
        m_Swapchain = std::make_unique<VulkanSwapchain>(this);
        m_Swapchain->SetFrameSync(m_ImageAvailable, m_RenderFinished, m_InFlight);
        if (auto r = m_Swapchain->Create(desc); !r) {
            WE_LOG_ERROR(we::LogCategory::Startup, r.error.message);
            m_Swapchain.reset();
            return;
        }
    }

    const auto end = std::chrono::steady_clock::now();
    m_Diagnostics.deviceCreateMs = std::chrono::duration<double, std::milli>(end - start).count();
    m_Valid = true;
    WE_LOG_INFO(we::LogCategory::Startup, "VulkanRHI device created.");
}

VulkanDevice::~VulkanDevice() {
    if (m_Device) {
        vkDeviceWaitIdle(m_Device);
    }

    // Flush deferred destroys immediately.
    while (!m_Deferred.empty()) {
        auto item = m_Deferred.front();
        m_Deferred.pop_front();
        DestroyImmediate(item.kind, item.handle);
    }

    m_Swapchain.reset();

    if (m_CommandPool) {
        vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
        m_CommandPool = VK_NULL_HANDLE;
    }
    for (auto s : m_ImageAvailable) {
        vkDestroySemaphore(m_Device, s, nullptr);
    }
    for (auto s : m_RenderFinished) {
        vkDestroySemaphore(m_Device, s, nullptr);
    }
    for (auto f : m_InFlight) {
        vkDestroyFence(m_Device, f, nullptr);
    }
    m_ImageAvailable.clear();
    m_RenderFinished.clear();
    m_InFlight.clear();

    if (m_Device) {
        vkDestroyDevice(m_Device, nullptr);
        m_Device = VK_NULL_HANDLE;
    }
    if (m_DebugMessenger) {
        vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
        m_DebugMessenger = VK_NULL_HANDLE;
    }
    if (m_Instance) {
        vkDestroyInstance(m_Instance, nullptr);
        m_Instance = VK_NULL_HANDLE;
    }
}

RHIResult<void> VulkanDevice::CreateInstance(const DeviceDesc& desc) {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = desc.debugName ? desc.debugName : "WindEffects";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "WindEffects";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = SelectInstanceApiVersion();

    std::vector<const char*> extensions = GetRequiredInstanceExtensions();
    if (extensions.empty()) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "No Vulkan instance extensions.", "CreateInstance");
    }

    bool useValidation = desc.enableValidation;
    if (!ValidationDisabledFromEnvironment() && ValidationRequestedFromEnvironment()) {
        useValidation = true;
    }
    if (useValidation && !AreValidationLayersAvailable()) {
        WE_LOG_WARN(we::LogCategory::Startup,
            "VK_LAYER_KHRONOS_validation unavailable; continuing without validation.");
        useValidation = false;
    }
    if (useValidation) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
    if (useValidation) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }

    if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkCreateInstance failed.", "CreateInstance");
    }
    volkLoadInstance(m_Instance);

    if (useValidation) {
        VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
        debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugInfo.pfnUserCallback = DebugCallback;
        if (vkCreateDebugUtilsMessengerEXT(m_Instance, &debugInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS) {
            return RHIError::Make(RHIErrorCode::BackendFailure, "Failed to create debug messenger.", "CreateInstance");
        }
    }
    return RHIResult<void>::Success();
}

RHIResult<void> VulkanDevice::PickPhysicalDevice(uint32_t adapterIndex) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "No Vulkan GPUs found.", "PickPhysicalDevice");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

    if (adapterIndex < deviceCount) {
        m_PhysicalDevice = devices[adapterIndex];
    } else {
        for (const auto& device : devices) {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(device, &properties);
            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                m_PhysicalDevice = device;
                break;
            }
        }
        if (m_PhysicalDevice == VK_NULL_HANDLE) {
            m_PhysicalDevice = devices[0];
        }
    }
    return RHIResult<void>::Success();
}

RHIResult<void> VulkanDevice::CreateLogicalDevice() {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            m_GraphicsFamily = i;
            break;
        }
    }
    if (m_GraphicsFamily == ~0u) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "No graphics queue family.", "CreateLogicalDevice");
    }

    float priority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = m_GraphicsFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &priority;

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;

    const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &features13;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = deviceExtensions;

    if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkCreateDevice failed.", "CreateLogicalDevice");
    }
    volkLoadDevice(m_Device);

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(m_Device, m_GraphicsFamily, 0, &graphicsQueue);
    m_GraphicsQueue.SetQueue(graphicsQueue);
    m_ComputeQueue.SetQueue(graphicsQueue);
    m_TransferQueue.SetQueue(graphicsQueue);
    return RHIResult<void>::Success();
}

RHIResult<void> VulkanDevice::CreateFrameSync() {
    m_ImageAvailable.resize(m_FramesInFlight);
    m_RenderFinished.resize(m_FramesInFlight);
    m_InFlight.resize(m_FramesInFlight);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < m_FramesInFlight; ++i) {
        if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailable[i]) != VK_SUCCESS
            || vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinished[i]) != VK_SUCCESS
            || vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlight[i]) != VK_SUCCESS) {
            return RHIError::Make(RHIErrorCode::BackendFailure, "Failed to create frame sync objects.", "CreateFrameSync");
        }
    }
    return RHIResult<void>::Success();
}

RHIResult<void> VulkanDevice::CreateCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_GraphicsFamily;
    if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "Failed to create command pool.", "CreateCommandPool");
    }

    m_CommandBuffers.resize(m_FramesInFlight);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = m_FramesInFlight;
    if (vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "Failed to allocate command buffers.", "CreateCommandPool");
    }
    return RHIResult<void>::Success();
}

uint64_t VulkanDevice::AllocHandle() {
    return m_NextHandle++;
}

IRHIQueue& VulkanDevice::GetQueue(QueueType type) {
    switch (type) {
    case QueueType::Compute: return m_ComputeQueue;
    case QueueType::Transfer: return m_TransferQueue;
    default: return m_GraphicsQueue;
    }
}

uint32_t VulkanDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memProperties{};
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i))
            && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return ~0u;
}

VulkanBuffer* VulkanDevice::FindBuffer(RHIBufferHandle handle) {
    auto it = m_Buffers.find(static_cast<uint64_t>(handle));
    return it == m_Buffers.end() ? nullptr : &it->second;
}

VulkanTexture* VulkanDevice::FindTexture(RHITextureHandle handle) {
    auto it = m_Textures.find(static_cast<uint64_t>(handle));
    return it == m_Textures.end() ? nullptr : &it->second;
}

VulkanGraphicsPipeline* VulkanDevice::FindGraphicsPipeline(RHIGraphicsPipelineHandle handle) {
    auto it = m_GraphicsPipelines.find(static_cast<uint64_t>(handle));
    return it == m_GraphicsPipelines.end() ? nullptr : &it->second;
}

VulkanComputePipeline* VulkanDevice::FindComputePipeline(RHIComputePipelineHandle handle) {
    auto it = m_ComputePipelines.find(static_cast<uint64_t>(handle));
    return it == m_ComputePipelines.end() ? nullptr : &it->second;
}

RHITextureHandle VulkanDevice::RegisterSwapchainTexture(
    VkImage image,
    VkImageView view,
    Extent2D extent,
    Format format)
{
    const auto handle = static_cast<RHITextureHandle>(AllocHandle());
    VulkanTexture tex{};
    tex.desc.extent = {extent.width, extent.height, 1};
    tex.desc.format = format;
    tex.desc.usage = TextureUsage::ColorAttachment | TextureUsage::Present | TextureUsage::TransferDst;
    tex.image = image;
    tex.view = view;
    tex.ownsImage = false;
    tex.isSwapchain = true;
    tex.state = ResourceState::Present;
    m_Textures.emplace(static_cast<uint64_t>(handle), tex);
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

void VulkanDevice::ClearSwapchainTextureHandles(const std::vector<RHITextureHandle>& handles) {
    for (auto h : handles) {
        m_Textures.erase(static_cast<uint64_t>(h));
    }
}

void VulkanDevice::EnqueueDeferred(DeferredKind kind, uint64_t handle) {
    m_Deferred.push_back(DeferredDestroyItem{kind, handle, m_FrameNumber + m_FramesInFlight});
    ++m_Diagnostics.deferredDestroys;
}

RHIResult<RHIBufferHandle> VulkanDevice::CreateBuffer(const BufferDesc& desc) {
    if (desc.size == 0) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Buffer size is 0.", "CreateBuffer");
    }

    VulkanBuffer buffer{};
    buffer.desc = desc;
    buffer.hostVisible = desc.memory != MemoryUsage::GpuLocal;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = desc.size;
    bufferInfo.usage = ToVkBufferUsage(desc.usage);
    if (bufferInfo.usage == 0) {
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer.buffer) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::OutOfMemory, "vkCreateBuffer failed.", "CreateBuffer");
    }

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(m_Device, buffer.buffer, &memRequirements);

    VkMemoryPropertyFlags memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (buffer.hostVisible) {
        memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, memFlags);
    if (allocInfo.memoryTypeIndex == ~0u) {
        vkDestroyBuffer(m_Device, buffer.buffer, nullptr);
        return RHIError::Make(RHIErrorCode::OutOfMemory, "No suitable memory type.", "CreateBuffer");
    }

    if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &buffer.memory) != VK_SUCCESS) {
        vkDestroyBuffer(m_Device, buffer.buffer, nullptr);
        return RHIError::Make(RHIErrorCode::OutOfMemory, "vkAllocateMemory failed.", "CreateBuffer");
    }
    vkBindBufferMemory(m_Device, buffer.buffer, buffer.memory, 0);

    if (buffer.hostVisible) {
        vkMapMemory(m_Device, buffer.memory, 0, desc.size, 0, &buffer.mapped);
    }

    const auto handle = static_cast<RHIBufferHandle>(AllocHandle());
    m_Buffers.emplace(static_cast<uint64_t>(handle), buffer);
    ++m_Diagnostics.resourcesCreated;
    m_Diagnostics.memory.bufferBytes += desc.size;
    m_Diagnostics.memory.allocatedBytes += desc.size;
    return handle;
}

RHIResult<void> VulkanDevice::DestroyBuffer(RHIBufferHandle handle) {
    if (!FindBuffer(handle)) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown buffer.", "DestroyBuffer");
    }
    EnqueueDeferred(DeferredKind::Buffer, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

void* VulkanDevice::MapBuffer(RHIBufferHandle handle) {
    auto* buf = FindBuffer(handle);
    return buf ? buf->mapped : nullptr;
}

void VulkanDevice::UnmapBuffer(RHIBufferHandle) {}

RHIResult<void> VulkanDevice::UpdateBuffer(RHIBufferHandle handle, std::span<const uint8_t> data, uint64_t offset) {
    auto* buf = FindBuffer(handle);
    if (!buf) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown buffer.", "UpdateBuffer");
    }
    if (!buf->mapped || !buf->hostVisible) {
        return RHIError::Make(RHIErrorCode::NotSupported, "Buffer is not host-visible.", "UpdateBuffer");
    }
    if (offset + data.size() > buf->desc.size) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Update exceeds buffer size.", "UpdateBuffer");
    }
    std::memcpy(static_cast<uint8_t*>(buf->mapped) + offset, data.data(), data.size());
    return RHIResult<void>::Success();
}

RHIResult<RHITextureHandle> VulkanDevice::CreateTexture(const TextureDesc& desc) {
    VulkanTexture tex{};
    tex.desc = desc;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {desc.extent.width, desc.extent.height, desc.extent.depth ? desc.extent.depth : 1};
    imageInfo.mipLevels = desc.mipLevels ? desc.mipLevels : 1;
    imageInfo.arrayLayers = desc.arrayLayers ? desc.arrayLayers : 1;
    imageInfo.format = ToVkFormat(desc.format);
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = ToVkImageUsage(desc.usage);
    if (imageInfo.usage == 0) {
        imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_Device, &imageInfo, nullptr, &tex.image) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::OutOfMemory, "vkCreateImage failed.", "CreateTexture");
    }

    VkMemoryRequirements memRequirements{};
    vkGetImageMemoryRequirements(m_Device, tex.image, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (allocInfo.memoryTypeIndex == ~0u
        || vkAllocateMemory(m_Device, &allocInfo, nullptr, &tex.memory) != VK_SUCCESS) {
        vkDestroyImage(m_Device, tex.image, nullptr);
        return RHIError::Make(RHIErrorCode::OutOfMemory, "Texture memory allocation failed.", "CreateTexture");
    }
    vkBindImageMemory(m_Device, tex.image, tex.memory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = tex.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = imageInfo.format;
    viewInfo.subresourceRange.aspectMask = IsDepthFormat(desc.format)
        ? VK_IMAGE_ASPECT_DEPTH_BIT
        : VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = imageInfo.mipLevels;
    viewInfo.subresourceRange.layerCount = imageInfo.arrayLayers;
    if (vkCreateImageView(m_Device, &viewInfo, nullptr, &tex.view) != VK_SUCCESS) {
        vkDestroyImage(m_Device, tex.image, nullptr);
        vkFreeMemory(m_Device, tex.memory, nullptr);
        return RHIError::Make(RHIErrorCode::BackendFailure, "Failed to create image view.", "CreateTexture");
    }

    const auto handle = static_cast<RHITextureHandle>(AllocHandle());
    m_Textures.emplace(static_cast<uint64_t>(handle), tex);
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> VulkanDevice::DestroyTexture(RHITextureHandle handle) {
    auto* tex = FindTexture(handle);
    if (!tex) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown texture.", "DestroyTexture");
    }
    if (tex->isSwapchain) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Cannot destroy swapchain texture.", "DestroyTexture");
    }
    EnqueueDeferred(DeferredKind::Texture, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<RHITextureViewHandle> VulkanDevice::CreateTextureView(const TextureViewDesc& desc) {
    auto* tex = FindTexture(desc.texture);
    if (!tex) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown texture.", "CreateTextureView");
    }
    VulkanTextureView view{};
    view.desc = desc;
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = tex->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = desc.format == Format::Unknown ? ToVkFormat(tex->desc.format) : ToVkFormat(desc.format);
    viewInfo.subresourceRange.aspectMask = IsDepthFormat(tex->desc.format)
        ? VK_IMAGE_ASPECT_DEPTH_BIT
        : VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = desc.baseMip;
    viewInfo.subresourceRange.levelCount = desc.mipCount;
    viewInfo.subresourceRange.baseArrayLayer = desc.baseLayer;
    viewInfo.subresourceRange.layerCount = desc.layerCount;
    if (vkCreateImageView(m_Device, &viewInfo, nullptr, &view.view) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "Failed to create texture view.", "CreateTextureView");
    }
    const auto handle = static_cast<RHITextureViewHandle>(AllocHandle());
    m_TextureViews.emplace(static_cast<uint64_t>(handle), view);
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> VulkanDevice::DestroyTextureView(RHITextureViewHandle handle) {
    if (m_TextureViews.find(static_cast<uint64_t>(handle)) == m_TextureViews.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown texture view.", "DestroyTextureView");
    }
    EnqueueDeferred(DeferredKind::TextureView, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<RHISamplerHandle> VulkanDevice::CreateSampler(const SamplerDesc& desc) {
    VulkanSampler sampler{};
    sampler.desc = desc;
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.anisotropyEnable = desc.anisotropy ? VK_TRUE : VK_FALSE;
    info.maxAnisotropy = desc.maxAnisotropy;
    info.maxLod = VK_LOD_CLAMP_NONE;
    if (vkCreateSampler(m_Device, &info, nullptr, &sampler.sampler) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkCreateSampler failed.", "CreateSampler");
    }
    const auto handle = static_cast<RHISamplerHandle>(AllocHandle());
    m_Samplers.emplace(static_cast<uint64_t>(handle), sampler);
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> VulkanDevice::DestroySampler(RHISamplerHandle handle) {
    if (m_Samplers.find(static_cast<uint64_t>(handle)) == m_Samplers.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown sampler.", "DestroySampler");
    }
    EnqueueDeferred(DeferredKind::Sampler, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<RHIShaderHandle> VulkanDevice::CreateShader(const ShaderDesc& desc) {
    if (desc.bytecode.empty()) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Empty shader bytecode.", "CreateShader");
    }
    VulkanShader shader{};
    shader.desc = desc;
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = desc.bytecode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(desc.bytecode.data());
    if (vkCreateShaderModule(m_Device, &createInfo, nullptr, &shader.module) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkCreateShaderModule failed.", "CreateShader");
    }
    const auto handle = static_cast<RHIShaderHandle>(AllocHandle());
    m_Shaders.emplace(static_cast<uint64_t>(handle), shader);
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> VulkanDevice::DestroyShader(RHIShaderHandle handle) {
    if (m_Shaders.find(static_cast<uint64_t>(handle)) == m_Shaders.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown shader.", "DestroyShader");
    }
    EnqueueDeferred(DeferredKind::Shader, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<RHIPipelineLayoutHandle> VulkanDevice::CreatePipelineLayout(const PipelineLayoutDesc&) {
    VulkanPipelineLayout layout{};
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (vkCreatePipelineLayout(m_Device, &info, nullptr, &layout.layout) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkCreatePipelineLayout failed.", "CreatePipelineLayout");
    }
    const auto handle = static_cast<RHIPipelineLayoutHandle>(AllocHandle());
    m_PipelineLayouts.emplace(static_cast<uint64_t>(handle), layout);
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> VulkanDevice::DestroyPipelineLayout(RHIPipelineLayoutHandle handle) {
    if (m_PipelineLayouts.find(static_cast<uint64_t>(handle)) == m_PipelineLayouts.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown pipeline layout.", "DestroyPipelineLayout");
    }
    EnqueueDeferred(DeferredKind::PipelineLayout, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<RHIGraphicsPipelineHandle> VulkanDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) {
    // Minimal stub: full graphics PSO creation lands with pass migration.
    VulkanGraphicsPipeline pipeline{};
    pipeline.desc = desc;
    const auto handle = static_cast<RHIGraphicsPipelineHandle>(AllocHandle());
    m_GraphicsPipelines.emplace(static_cast<uint64_t>(handle), pipeline);
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> VulkanDevice::DestroyGraphicsPipeline(RHIGraphicsPipelineHandle handle) {
    if (m_GraphicsPipelines.find(static_cast<uint64_t>(handle)) == m_GraphicsPipelines.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown graphics pipeline.", "DestroyGraphicsPipeline");
    }
    EnqueueDeferred(DeferredKind::GraphicsPipeline, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<RHIComputePipelineHandle> VulkanDevice::CreateComputePipeline(const ComputePipelineDesc& desc) {
    VulkanComputePipeline pipeline{};
    pipeline.desc = desc;
    const auto handle = static_cast<RHIComputePipelineHandle>(AllocHandle());
    m_ComputePipelines.emplace(static_cast<uint64_t>(handle), pipeline);
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> VulkanDevice::DestroyComputePipeline(RHIComputePipelineHandle handle) {
    if (m_ComputePipelines.find(static_cast<uint64_t>(handle)) == m_ComputePipelines.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown compute pipeline.", "DestroyComputePipeline");
    }
    EnqueueDeferred(DeferredKind::ComputePipeline, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

IRHICommandList* VulkanDevice::BeginFrame() {
    if (!m_Valid || m_FrameActive) {
        return nullptr;
    }

    vkWaitForFences(m_Device, 1, &m_InFlight[m_FrameSlot], VK_TRUE, UINT64_MAX);
    vkResetFences(m_Device, 1, &m_InFlight[m_FrameSlot]);

    if (m_Swapchain) {
        m_Swapchain->SetFrameSlot(m_FrameSlot);
        auto acquire = m_Swapchain->AcquireNextImageForSlot(m_FrameSlot);
        if (!acquire) {
            if (acquire.error.code == RHIErrorCode::OutOfDate) {
                m_Swapchain->SetFrameSlot(m_FrameSlot);
                acquire = m_Swapchain->AcquireNextImageForSlot(m_FrameSlot);
            }
            if (!acquire) {
                return nullptr;
            }
        }
    }

    VkCommandBuffer cmd = m_CommandBuffers[m_FrameSlot];
    vkResetCommandBuffer(cmd, 0);
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    m_FrameCmd.SetCommandBuffer(cmd);
    m_FrameCmd.Begin();
    m_FrameActive = true;
    m_ActiveCmd = &m_FrameCmd;
    return &m_FrameCmd;
}

RHIResult<void> VulkanDevice::Submit(IRHICommandList* commandList) {
    if (!commandList || !m_FrameActive) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "No active frame command list.", "Submit");
    }
    commandList->End();
    auto* vkCmd = static_cast<VulkanCommandList*>(commandList);
    VkCommandBuffer cmd = vkCmd->GetVkCommandBuffer();
    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkEndCommandBuffer failed.", "Submit");
    }

    VkSemaphore waitSemaphores[] = {m_ImageAvailable[m_FrameSlot]};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT};
    VkSemaphore signalSemaphores[] = {m_RenderFinished[m_FrameSlot]};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    if (m_Swapchain) {
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
    }
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    if (vkQueueSubmit(m_GraphicsQueue.GetVkQueue(), 1, &submitInfo, m_InFlight[m_FrameSlot]) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkQueueSubmit failed.", "Submit");
    }
    return RHIResult<void>::Success();
}

RHIResult<void> VulkanDevice::Present() {
    if (!m_Swapchain) {
        return RHIResult<void>::Success();
    }
    return m_Swapchain->PresentForSlot(m_FrameSlot);
}

RHIResult<void> VulkanDevice::EndFrame() {
    m_FrameActive = false;
    m_ActiveCmd = nullptr;
    m_FrameSlot = (m_FrameSlot + 1) % m_FramesInFlight;
    ++m_FrameNumber;
    ++m_Diagnostics.lastFrame.frameIndex;
    TickDeferredDestruction();
    return RHIResult<void>::Success();
}

RHIResult<void> VulkanDevice::WaitIdle() {
    if (m_Device) {
        vkDeviceWaitIdle(m_Device);
    }
    return RHIResult<void>::Success();
}

void VulkanDevice::SetResourceName(RHIBufferHandle handle, std::string_view name) {
    if (!vkSetDebugUtilsObjectNameEXT) {
        return;
    }
    auto* buf = FindBuffer(handle);
    if (!buf) {
        return;
    }
    const std::string n(name);
    VkDebugUtilsObjectNameInfoEXT info{};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    info.objectType = VK_OBJECT_TYPE_BUFFER;
    info.objectHandle = reinterpret_cast<uint64_t>(buf->buffer);
    info.pObjectName = n.c_str();
    vkSetDebugUtilsObjectNameEXT(m_Device, &info);
}

void VulkanDevice::SetResourceName(RHITextureHandle handle, std::string_view name) {
    if (!vkSetDebugUtilsObjectNameEXT) {
        return;
    }
    auto* tex = FindTexture(handle);
    if (!tex) {
        return;
    }
    const std::string n(name);
    VkDebugUtilsObjectNameInfoEXT info{};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    info.objectType = VK_OBJECT_TYPE_IMAGE;
    info.objectHandle = reinterpret_cast<uint64_t>(tex->image);
    info.pObjectName = n.c_str();
    vkSetDebugUtilsObjectNameEXT(m_Device, &info);
}

void VulkanDevice::DestroyImmediate(DeferredKind kind, uint64_t handle) {
    switch (kind) {
    case DeferredKind::Buffer: {
        auto it = m_Buffers.find(handle);
        if (it == m_Buffers.end()) {
            break;
        }
        if (it->second.mapped) {
            vkUnmapMemory(m_Device, it->second.memory);
        }
        vkDestroyBuffer(m_Device, it->second.buffer, nullptr);
        vkFreeMemory(m_Device, it->second.memory, nullptr);
        m_Buffers.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::Texture: {
        auto it = m_Textures.find(handle);
        if (it == m_Textures.end() || it->second.isSwapchain) {
            break;
        }
        if (it->second.view) {
            vkDestroyImageView(m_Device, it->second.view, nullptr);
        }
        if (it->second.ownsImage && it->second.image) {
            vkDestroyImage(m_Device, it->second.image, nullptr);
        }
        if (it->second.memory) {
            vkFreeMemory(m_Device, it->second.memory, nullptr);
        }
        m_Textures.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::TextureView: {
        auto it = m_TextureViews.find(handle);
        if (it == m_TextureViews.end()) {
            break;
        }
        vkDestroyImageView(m_Device, it->second.view, nullptr);
        m_TextureViews.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::Sampler: {
        auto it = m_Samplers.find(handle);
        if (it == m_Samplers.end()) {
            break;
        }
        vkDestroySampler(m_Device, it->second.sampler, nullptr);
        m_Samplers.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::Shader: {
        auto it = m_Shaders.find(handle);
        if (it == m_Shaders.end()) {
            break;
        }
        vkDestroyShaderModule(m_Device, it->second.module, nullptr);
        m_Shaders.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::PipelineLayout: {
        auto it = m_PipelineLayouts.find(handle);
        if (it == m_PipelineLayouts.end()) {
            break;
        }
        vkDestroyPipelineLayout(m_Device, it->second.layout, nullptr);
        m_PipelineLayouts.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::GraphicsPipeline: {
        auto it = m_GraphicsPipelines.find(handle);
        if (it == m_GraphicsPipelines.end()) {
            break;
        }
        if (it->second.pipeline) {
            vkDestroyPipeline(m_Device, it->second.pipeline, nullptr);
        }
        m_GraphicsPipelines.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    case DeferredKind::ComputePipeline: {
        auto it = m_ComputePipelines.find(handle);
        if (it == m_ComputePipelines.end()) {
            break;
        }
        if (it->second.pipeline) {
            vkDestroyPipeline(m_Device, it->second.pipeline, nullptr);
        }
        m_ComputePipelines.erase(it);
        ++m_Diagnostics.resourcesDestroyed;
        break;
    }
    }
}

void VulkanDevice::TickDeferredDestruction() {
    while (!m_Deferred.empty() && m_Deferred.front().frame <= m_FrameNumber) {
        auto item = m_Deferred.front();
        m_Deferred.pop_front();
        DestroyImmediate(item.kind, item.handle);
    }
}

std::unique_ptr<IRHIDevice> CreateVulkanDevice(const DeviceDesc& desc) {
    auto device = std::make_unique<VulkanDevice>(desc);
    if (!device->IsValid()) {
        return nullptr;
    }
    return device;
}

} // namespace we::rhi::vulkan
