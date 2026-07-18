#include "VulkanDevice.h"
#include "VulkanFormats.h"
#include "VulkanPlatformSurface.h"
#include "VulkanInternal.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "RHI/ShaderBytecode.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

namespace we::rhi::vulkan {
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
    if (auto r = CreateFrameCommandPool(); !r) {
        WE_LOG_ERROR(we::LogCategory::Startup, r.error.message);
        return;
    }

    FillCapabilities();
    m_Caps.debugMarkers = m_DebugMessenger != VK_NULL_HANDLE;
    m_Caps.maxFramesInFlight = m_FramesInFlight;

    if (we::platform::IsValid(desc.window) && !desc.headless) {
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
    m_GraphicsQueue.SetDevice(this);
    m_ComputeQueue.SetQueue(graphicsQueue);
    m_ComputeQueue.SetDevice(this);
    m_TransferQueue.SetQueue(graphicsQueue);
    m_TransferQueue.SetDevice(this);
    return RHIResult<void>::Success();
}

void VulkanDevice::FillCapabilities() {
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &props);
    VkPhysicalDeviceFeatures features{};
    vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &features);
    VkPhysicalDeviceMemoryProperties memProps{};
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProps);

    m_Caps.dynamicRendering = true;
    m_Caps.timestamps = true; // query pools + WriteTimestamp
    m_Caps.pipelineStatistics = false;
    m_Caps.timelineSemaphores = false;
    m_Caps.asyncCompute = false; // graphics queue used for compute today
    m_Caps.bindlessResources = false;
    m_Caps.meshShaders = false;
    m_Caps.rayTracing = false;
    m_Caps.debugMarkers = m_DebugMessenger != VK_NULL_HANDLE;
    m_Caps.geometryShaders = features.geometryShader == VK_TRUE;
    m_Caps.tessellation = features.tessellationShader == VK_TRUE;
    m_Caps.multiDrawIndirect = features.multiDrawIndirect == VK_TRUE;
    m_Caps.samplerAnisotropy = features.samplerAnisotropy == VK_TRUE;
    m_Caps.fillModeNonSolid = features.fillModeNonSolid == VK_TRUE;
    m_Caps.depthClamp = features.depthClamp == VK_TRUE;
    m_Caps.independentBlend = features.independentBlend == VK_TRUE;
    m_Caps.shaderFloat64 = features.shaderFloat64 == VK_TRUE;
    m_Caps.shaderInt64 = features.shaderInt64 == VK_TRUE;
    m_Caps.drawIndirectFirstInstance = features.drawIndirectFirstInstance == VK_TRUE;
    m_Caps.textureCompressionBC = features.textureCompressionBC == VK_TRUE;
    m_Caps.maxSamplerAnisotropy = props.limits.maxSamplerAnisotropy;
    m_Caps.maxColorAttachments = props.limits.maxColorAttachments;
    m_Caps.maxPushConstantBytes = props.limits.maxPushConstantsSize;
    m_Caps.maxBoundDescriptorSets = props.limits.maxBoundDescriptorSets;
    m_Caps.maxDescriptorSetSamplers = props.limits.maxDescriptorSetSamplers;
    m_Caps.maxDescriptorSetUniformBuffers = props.limits.maxDescriptorSetUniformBuffers;
    m_Caps.maxDescriptorSetStorageBuffers = props.limits.maxDescriptorSetStorageBuffers;
    m_Caps.maxDescriptorSetSampledImages = props.limits.maxDescriptorSetSampledImages;
    m_Caps.maxDescriptorSetStorageImages = props.limits.maxDescriptorSetStorageImages;
    m_Caps.maxComputeWorkGroupInvocations = props.limits.maxComputeWorkGroupInvocations;
    m_Caps.maxComputeWorkGroupSizeX = props.limits.maxComputeWorkGroupSize[0];
    m_Caps.maxComputeWorkGroupSizeY = props.limits.maxComputeWorkGroupSize[1];
    m_Caps.maxComputeWorkGroupSizeZ = props.limits.maxComputeWorkGroupSize[2];
    m_Caps.maxTextureDimension2D = props.limits.maxImageDimension2D;
    m_Caps.minUniformBufferOffsetAlignment = props.limits.minUniformBufferOffsetAlignment;
    m_Caps.minStorageBufferOffsetAlignment = props.limits.minStorageBufferOffsetAlignment;

    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        const auto flags = memProps.memoryTypes[i].propertyFlags;
        if ((flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            && (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
            m_Caps.hostVisibleDeviceLocal = true;
            break;
        }
    }
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

RHIResult<void> VulkanDevice::CreateFrameCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_GraphicsFamily;
    if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "Failed to create command pool.", "CreateFrameCommandPool");
    }

    m_CommandBuffers.resize(m_FramesInFlight);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = m_FramesInFlight;
    if (vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "Failed to allocate command buffers.", "CreateFrameCommandPool");
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

VulkanTextureView* VulkanDevice::FindTextureView(RHITextureViewHandle handle) {
    auto it = m_TextureViews.find(static_cast<uint64_t>(handle));
    return it == m_TextureViews.end() ? nullptr : &it->second;
}

VulkanSampler* VulkanDevice::FindSampler(RHISamplerHandle handle) {
    auto it = m_Samplers.find(static_cast<uint64_t>(handle));
    return it == m_Samplers.end() ? nullptr : &it->second;
}

VulkanShader* VulkanDevice::FindShader(RHIShaderHandle handle) {
    auto it = m_Shaders.find(static_cast<uint64_t>(handle));
    return it == m_Shaders.end() ? nullptr : &it->second;
}

VulkanDescriptorSet* VulkanDevice::FindDescriptorSet(RHIDescriptorSetHandle handle) {
    auto it = m_DescriptorSets.find(static_cast<uint64_t>(handle));
    return it == m_DescriptorSets.end() ? nullptr : &it->second;
}

VulkanPipelineLayout* VulkanDevice::FindPipelineLayout(RHIPipelineLayoutHandle handle) {
    auto it = m_PipelineLayouts.find(static_cast<uint64_t>(handle));
    return it == m_PipelineLayouts.end() ? nullptr : &it->second;
}

VulkanGraphicsPipeline* VulkanDevice::FindGraphicsPipeline(RHIGraphicsPipelineHandle handle) {
    auto it = m_GraphicsPipelines.find(static_cast<uint64_t>(handle));
    return it == m_GraphicsPipelines.end() ? nullptr : &it->second;
}

VulkanComputePipeline* VulkanDevice::FindComputePipeline(RHIComputePipelineHandle handle) {
    auto it = m_ComputePipelines.find(static_cast<uint64_t>(handle));
    return it == m_ComputePipelines.end() ? nullptr : &it->second;
}

VulkanFence* VulkanDevice::FindFence(RHIFenceHandle handle) {
    auto it = m_Fences.find(static_cast<uint64_t>(handle));
    return it == m_Fences.end() ? nullptr : &it->second;
}

VulkanSemaphore* VulkanDevice::FindSemaphore(RHISemaphoreHandle handle) {
    auto it = m_Semaphores.find(static_cast<uint64_t>(handle));
    return it == m_Semaphores.end() ? nullptr : &it->second;
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
} // namespace we::rhi::vulkan
