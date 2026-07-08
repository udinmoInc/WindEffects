#include "Core/SwapchainManager.h"
#include "Core/DeviceContext.h"
#include "Core/FrameSync.h"
#include "Core/Validation.h"
#include "Core/LogCategory.h"
#include <stdexcept>
#include <algorithm>
#include <SDL3/SDL_vulkan.h>
#include <algorithm>

namespace we::runtime::renderer {

SwapchainManager::~SwapchainManager() {
    Shutdown();
}

void SwapchainManager::Init(const SwapchainManagerConfig& config) {
    WE_VALIDATE_INIT(!m_Initialized, "SwapchainManager", "Already initialized.");
    WE_VALIDATE_INIT(config.deviceContext != nullptr, "SwapchainManager", "DeviceContext is null.");
    WE_VALIDATE_INIT(config.window != nullptr, "SwapchainManager", "Window is null.");

    m_Config = config;

    CreateSurface();
    CreateSwapchain();
    CreateImageViews();

    m_Initialized = true;
}

void SwapchainManager::Shutdown() {
    if (!m_Initialized) return;

    CleanupSwapchain();

    if (m_Surface) {
        vkDestroySurfaceKHR(m_Config.deviceContext->GetInstance(), m_Surface, nullptr);
        m_Surface = VK_NULL_HANDLE;
    }

    m_Initialized = false;
}

void SwapchainManager::Recreate() {
    WE_VALIDATE_RENDER(m_Initialized, "Swapchain", "Cannot recreate uninitialized swapchain.");
    
    int width = 0, height = 0;
    SDL_GetWindowSizeInPixels(m_Config.window, &width, &height);
    if (width == 0 || height == 0) return; // Minimized

    vkDeviceWaitIdle(m_Config.deviceContext->GetDevice());

    CleanupSwapchain();
    CreateSwapchain();
    CreateImageViews();
}

void SwapchainManager::CleanupSwapchain() {
    VkDevice device = m_Config.deviceContext->GetDevice();

    for (auto imageView : m_ImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    m_ImageViews.clear();

    if (m_Swapchain) {
        vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
        m_Swapchain = VK_NULL_HANDLE;
    }
}

void SwapchainManager::CreateSurface() {
    bool success = SDL_Vulkan_CreateSurface(m_Config.window, m_Config.deviceContext->GetInstance(), nullptr, &m_Surface);
    WE_VALIDATE_INIT(success, "SwapchainManager", "Failed to create SDL Vulkan surface.");

    VkPhysicalDevice physicalDevice = m_Config.deviceContext->GetPhysicalDevice();
    const uint32_t graphicsFamily = m_Config.deviceContext->GetGraphicsQueueFamily();

    VkBool32 graphicsPresent = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, graphicsFamily, m_Surface, &graphicsPresent);
    if (graphicsPresent) {
        m_PresentQueueFamily = graphicsFamily;
        m_PresentQueue = m_Config.deviceContext->GetGraphicsQueue();
    } else {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        m_PresentQueueFamily = ~0u;
        for (uint32_t i = 0; i < queueFamilyCount; ++i) {
            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, m_Surface, &presentSupport);
            if (presentSupport) {
                m_PresentQueueFamily = i;
                break;
            }
        }
        WE_VALIDATE_INIT(m_PresentQueueFamily != ~0u, "SwapchainManager", "No queue family supports presentation.");
        vkGetDeviceQueue(m_Config.deviceContext->GetDevice(), m_PresentQueueFamily, 0, &m_PresentQueue);
    }
}

void SwapchainManager::CreateSwapchain() {
    VkPhysicalDevice physicalDevice = m_Config.deviceContext->GetPhysicalDevice();
    VkDevice device = m_Config.deviceContext->GetDevice();

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, formats.data());

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &presentModeCount, presentModes.data());

    // Choose format - use UNORM for linear color space to avoid sRGB conversion affecting UI
    // Use RGBA format to match UI texture formats (RGBA8) and prevent channel swapping
    VkSurfaceFormatKHR surfaceFormat = formats[0];
    for (const auto& availableFormat : formats) {
        if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = availableFormat;
            break;
        }
    }

    // Choose present mode
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // Guaranteed
    if (!m_Config.vsync) {
        for (const auto& availablePresentMode : presentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                presentMode = availablePresentMode;
                break;
            }
        }
    }

    // Choose extent
    int width, height;
    SDL_GetWindowSizeInPixels(m_Config.window, &width, &height);
    VkExtent2D extent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_Surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_Swapchain);
    WE_VALIDATE_INIT(result == VK_SUCCESS, "SwapchainManager", "Failed to create swapchain.");

    vkGetSwapchainImagesKHR(device, m_Swapchain, &imageCount, nullptr);
    m_Images.resize(imageCount);
    vkGetSwapchainImagesKHR(device, m_Swapchain, &imageCount, m_Images.data());

    m_ImageFormat = surfaceFormat.format;
    m_Extent = extent;
}

void SwapchainManager::CreateImageViews() {
    VkDevice device = m_Config.deviceContext->GetDevice();
    m_ImageViews.resize(m_Images.size());

    for (size_t i = 0; i < m_Images.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_Images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_ImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(device, &createInfo, nullptr, &m_ImageViews[i]);
        WE_VALIDATE_INIT(result == VK_SUCCESS, "SwapchainManager", "Failed to create image views.");
    }
}

bool SwapchainManager::AcquireNextImage(uint32_t frameIndex, uint32_t& outImageIndex) {
    WE_VALIDATE_RENDER(m_Initialized, "SwapchainManager::AcquireNextImage", "Swapchain not initialized.");
    WE_VALIDATE_RENDER(m_FrameSync != nullptr, "SwapchainManager::AcquireNextImage", "FrameSync is null.");

    VkResult result = vkAcquireNextImageKHR(
        m_Config.deviceContext->GetDevice(),
        m_Swapchain,
        UINT64_MAX,
        m_FrameSync->GetImageAvailableSemaphore(frameIndex),
        VK_NULL_HANDLE,
        &outImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        Recreate();
        return false;
    }

    WE_VALIDATE_RENDER(result == VK_SUCCESS, "SwapchainManager::AcquireNextImage", "Failed to acquire swapchain image.");
    return true;
}

void SwapchainManager::SubmitFrame(uint32_t frameIndex, uint32_t imageIndex, VkCommandBuffer commandBuffer) {
    WE_VALIDATE_RENDER(m_Initialized, "SwapchainManager::SubmitFrame", "Swapchain not initialized.");
    WE_VALIDATE_RENDER(m_FrameSync != nullptr, "SwapchainManager::SubmitFrame", "FrameSync is null.");

    VkSemaphore waitSemaphores[] = { m_FrameSync->GetImageAvailableSemaphore(frameIndex) };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[] = { m_FrameSync->GetRenderFinishedSemaphore(frameIndex) };
    VkFence inFlightFence = m_FrameSync->GetInFlightFence(frameIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VkResult result = vkQueueSubmit(
        m_Config.deviceContext->GetGraphicsQueue(),
        1,
        &submitInfo,
        inFlightFence);

    if (result != VK_SUCCESS) {
        WE_LOG_ERROR(
            we::LogCategory::Renderer.data(),
            std::string("[SwapchainSubmit] vkQueueSubmit failed. frame=") + std::to_string(frameIndex) +
            " image=" + std::to_string(imageIndex) +
            " cmd=0x" + std::to_string(reinterpret_cast<uint64_t>(commandBuffer)) +
            " waitSem=0x" + std::to_string(reinterpret_cast<uint64_t>(waitSemaphores[0])) +
            " signalSem=0x" + std::to_string(reinterpret_cast<uint64_t>(signalSemaphores[0])) +
            " fence=0x" + std::to_string(reinterpret_cast<uint64_t>(inFlightFence)) +
            " vkResult=" + std::to_string(static_cast<int32_t>(result)));
    }

    WE_VALIDATE_RENDER(result == VK_SUCCESS, "SwapchainManager::SubmitFrame", "Failed to submit command buffer.");
}

void SwapchainManager::Present(uint32_t frameIndex, uint32_t imageIndex) {
    WE_VALIDATE_RENDER(m_Initialized, "SwapchainManager::Present", "Swapchain not initialized.");
    WE_VALIDATE_RENDER(m_FrameSync != nullptr, "SwapchainManager::Present", "FrameSync is null.");

    VkSemaphore signalSemaphores[] = { m_FrameSync->GetRenderFinishedSemaphore(frameIndex) };
    VkSwapchainKHR swapchains[] = { m_Swapchain };
    uint32_t imageIndices[] = { imageIndex };

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = imageIndices;

    VkResult result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        Recreate();
        return;
    }

    WE_VALIDATE_RENDER(result == VK_SUCCESS, "SwapchainManager::Present", "Failed to present swapchain image.");
}

} // namespace we::runtime::renderer
