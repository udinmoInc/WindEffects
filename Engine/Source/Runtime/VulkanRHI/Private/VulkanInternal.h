#pragma once

#include "VulkanDevice.h"
#include "VulkanFormats.h"

#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"

#include <cstring>
#include <string>
#include <vector>
#include <volk.h>

namespace we::rhi::vulkan {

inline bool AreValidationLayersAvailable() {
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

inline bool ValidationRequestedFromEnvironment() {
    if (const char* env = std::getenv("WE_VULKAN_VALIDATION")) {
        return env[0] == '1' || std::strcmp(env, "true") == 0;
    }
    return false;
}

inline bool ValidationDisabledFromEnvironment() {
    if (const char* env = std::getenv("WE_VULKAN_VALIDATION")) {
        return env[0] == '0' || std::strcmp(env, "false") == 0;
    }
    return false;
}

inline uint32_t SelectInstanceApiVersion() {
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

inline VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
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


} // namespace we::rhi::vulkan