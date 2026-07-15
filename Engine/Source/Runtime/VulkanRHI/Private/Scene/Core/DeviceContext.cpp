#include "Core/DeviceContext.h"
#include "Core/Validation.h"
#include "Core/VulkanSurface.h"
#include <stdexcept>
#include <string>
#include <set>
#include <algorithm>
#include <cstring>
#include <vector>

namespace we::runtime::renderer {

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

std::string VkResultToString(VkResult result) {
    switch (result) {
    case VK_SUCCESS: return "VK_SUCCESS";
    case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    default: break;
    }
    return "VkResult=" + std::to_string(static_cast<int>(result));
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

} // namespace

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) 
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        WE_LOG_ERROR("Vulkan", "Validation Layer Error!");
    } else {
        WE_LOG_TRACE("Vulkan", "Validation Layer Trace!");
    }
    return VK_FALSE;
}

DeviceContext::~DeviceContext() {
    Shutdown();
}

void DeviceContext::Init(const DeviceContextConfig& config) {
    WE_VALIDATE_INIT(!m_Initialized, "DeviceContext", "DeviceContext is already initialized.");
    
    m_NativeWindow = config.nativeWindow;
    InitVolk();
    CreateInstance(config);
    PickPhysicalDevice();
    CreateLogicalDevice();

    m_Initialized = true;
}

void DeviceContext::Shutdown() {
    if (!m_Initialized) return;

    if (m_Device) {
        vkDeviceWaitIdle(m_Device);
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

    m_Initialized = false;
}

void DeviceContext::InitVolk() {
    VkResult result = volkInitialize();
    WE_VALIDATE_INIT(result == VK_SUCCESS, "DeviceContext", "Failed to initialize Volk.");
}

void DeviceContext::CreateInstance(const DeviceContextConfig& config) {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = config.appName.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "WindEffects";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = SelectInstanceApiVersion();

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    std::vector<const char*> extensions = GetRequiredVulkanInstanceExtensions();
    WE_VALIDATE_INIT(!extensions.empty(), "DeviceContext", "No Vulkan instance extensions available for this platform.");

    bool useValidationLayers = config.enableValidationLayers;
    if (!ValidationDisabledFromEnvironment() && ValidationRequestedFromEnvironment()) {
        useValidationLayers = true;
    }
    if (useValidationLayers && !AreValidationLayersAvailable()) {
        WE_LOG_WARN("Vulkan",
            "VK_LAYER_KHRONOS_validation is not available. Continuing without validation layers. "
            "Install the Vulkan SDK or set WE_VULKAN_VALIDATION=0 to silence this warning.");
        useValidationLayers = false;
    }

    if (useValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
    if (useValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);
    if (result != VK_SUCCESS) {
        WE_VALIDATE_INIT(false, "DeviceContext",
            std::string("Failed to create Vulkan instance (")
                + VkResultToString(result)
                + "). Ensure a Vulkan 1.3-capable GPU driver is installed and vulkan-1.dll is loadable.");
    }

    volkLoadInstance(m_Instance);

    if (useValidationLayers) {
        VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
        debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugInfo.pfnUserCallback = DebugCallback;

        result = vkCreateDebugUtilsMessengerEXT(m_Instance, &debugInfo, nullptr, &m_DebugMessenger);
        WE_VALIDATE_INIT(result == VK_SUCCESS, "DeviceContext", "Failed to create debug messenger.");
    }
}

void DeviceContext::PickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
    WE_VALIDATE_INIT(deviceCount > 0, "DeviceContext", "Failed to find GPUs with Vulkan support.");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

    // Pick first discrete GPU, or fallback to first available
    for (const auto& device : devices) {
        VkPhysicalDeviceProperties properties;
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

void DeviceContext::CreateLogicalDevice() {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            m_GraphicsQueueFamily = i;
            break;
        }
    }
    WE_VALIDATE_INIT(m_GraphicsQueueFamily != ~0u, "DeviceContext", "Failed to find a graphics queue family.");

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = m_GraphicsQueueFamily;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE; // Enable standard features

    // Require VK_KHR_dynamic_rendering and VK_KHR_synchronization2
    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &features13;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VkResult result = vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device);
    WE_VALIDATE_INIT(result == VK_SUCCESS, "DeviceContext", "Failed to create logical device.");

    volkLoadDevice(m_Device);

    vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily, 0, &m_GraphicsQueue);
}

VkInstance DeviceContext::GetInstance() const { return m_Instance; }
VkPhysicalDevice DeviceContext::GetPhysicalDevice() const { return m_PhysicalDevice; }
VkDevice DeviceContext::GetDevice() const { return m_Device; }

uint32_t DeviceContext::GetGraphicsQueueFamily() const { return m_GraphicsQueueFamily; }
VkQueue DeviceContext::GetGraphicsQueue() const { return m_GraphicsQueue; }

} // namespace we::runtime::renderer
