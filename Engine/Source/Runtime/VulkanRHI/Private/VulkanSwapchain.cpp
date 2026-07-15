#include "VulkanDevice.h"
#include "VulkanFormats.h"
#include "VulkanPlatformSurface.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "Platform/Platform.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace we::rhi::vulkan {

VulkanSwapchain::VulkanSwapchain(VulkanDevice* device)
    : m_Device(device)
{
}

VulkanSwapchain::~VulkanSwapchain() {
    Destroy();
}

void VulkanSwapchain::SetFrameSync(
    const std::vector<VkSemaphore>& imageAvailable,
    const std::vector<VkSemaphore>& renderFinished,
    const std::vector<VkFence>& inFlight)
{
    m_ImageAvailable = &imageAvailable;
    m_RenderFinished = &renderFinished;
    m_InFlight = &inFlight;
}

RHIResult<void> VulkanSwapchain::Create(const DeviceDesc& desc) {
    m_Desc = desc;
    try {
        m_Surface = CreateSurface(m_Device->GetVkInstance(), desc.window);
    } catch (const std::exception& ex) {
        return RHIError::Make(RHIErrorCode::BackendFailure, ex.what(), "CreateSurface");
    }

    VkBool32 presentSupport = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(
        m_Device->GetVkPhysicalDevice(),
        m_Device->GetGraphicsQueueFamily(),
        m_Surface,
        &presentSupport);

    auto& graphicsQueue = static_cast<VulkanQueue&>(m_Device->GetQueue(QueueType::Graphics));
    if (presentSupport) {
        m_PresentFamily = m_Device->GetGraphicsQueueFamily();
        m_PresentQueue = graphicsQueue.GetVkQueue();
    } else {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_Device->GetVkPhysicalDevice(), &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_Device->GetVkPhysicalDevice(), &queueFamilyCount, families.data());
        m_PresentFamily = ~0u;
        for (uint32_t i = 0; i < queueFamilyCount; ++i) {
            VkBool32 ok = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(m_Device->GetVkPhysicalDevice(), i, m_Surface, &ok);
            if (ok) {
                m_PresentFamily = i;
                break;
            }
        }
        if (m_PresentFamily == ~0u) {
            return RHIError::Make(RHIErrorCode::BackendFailure, "No present queue family.", "CreateSwapchain");
        }
        vkGetDeviceQueue(m_Device->GetVkDevice(), m_PresentFamily, 0, &m_PresentQueue);
    }

    return Rebuild();
}

void VulkanSwapchain::Destroy() {
    CleanupImages();
    if (m_Swapchain && m_Device) {
        vkDestroySwapchainKHR(m_Device->GetVkDevice(), m_Swapchain, nullptr);
        m_Swapchain = VK_NULL_HANDLE;
    }
    if (m_Surface && m_Device) {
        vkDestroySurfaceKHR(m_Device->GetVkInstance(), m_Surface, nullptr);
        m_Surface = VK_NULL_HANDLE;
    }
}

void VulkanSwapchain::CleanupImages() {
    if (!m_Device) {
        return;
    }
    m_Device->ClearSwapchainTextureHandles(m_Handles);
    m_Handles.clear();
    for (auto view : m_ImageViews) {
        if (view) {
            vkDestroyImageView(m_Device->GetVkDevice(), view, nullptr);
        }
    }
    m_ImageViews.clear();
    m_Images.clear();
}

RHIResult<void> VulkanSwapchain::Rebuild() {
    if (!m_Device || !m_Surface) {
        return RHIError::Make(RHIErrorCode::NotInitialized, "Swapchain surface missing.", "Rebuild");
    }

    vkDeviceWaitIdle(m_Device->GetVkDevice());
    CleanupImages();
    if (m_Swapchain) {
        vkDestroySwapchainKHR(m_Device->GetVkDevice(), m_Swapchain, nullptr);
        m_Swapchain = VK_NULL_HANDLE;
    }

    VkSurfaceCapabilitiesKHR capabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Device->GetVkPhysicalDevice(), m_Surface, &capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device->GetVkPhysicalDevice(), m_Surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device->GetVkPhysicalDevice(), m_Surface, &formatCount, formats.data());

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device->GetVkPhysicalDevice(), m_Surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        m_Device->GetVkPhysicalDevice(), m_Surface, &presentModeCount, presentModes.data());

    VkSurfaceFormatKHR surfaceFormat = formats.empty()
        ? VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
        : formats[0];
    for (const auto& available : formats) {
        if (available.format == VK_FORMAT_R8G8B8A8_UNORM
            && available.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = available;
            break;
        }
    }

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (!m_Desc.vsync) {
        for (const auto& mode : presentModes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                presentMode = mode;
                break;
            }
        }
    }

    Extent2D extent{};
    if (m_Desc.windowId != we::platform::WindowId::Invalid) {
        const auto pixelSize = we::platform::Platform::Get().GetWindowPixelSize(m_Desc.windowId);
        extent.width = pixelSize.x;
        extent.height = pixelSize.y;
    }
    if (extent.width == 0 || extent.height == 0) {
        extent.width = capabilities.currentExtent.width;
        extent.height = capabilities.currentExtent.height;
    }
    if (capabilities.currentExtent.width != UINT32_MAX) {
        extent.width = capabilities.currentExtent.width;
        extent.height = capabilities.currentExtent.height;
    } else {
        extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    if (extent.width == 0 || extent.height == 0) {
        return RHIError::Make(RHIErrorCode::OutOfDate, "Swapchain extent is zero (minimized).", "Rebuild");
    }

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
    createInfo.imageExtent = {extent.width, extent.height};
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(m_Device->GetVkDevice(), &createInfo, nullptr, &m_Swapchain) != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkCreateSwapchainKHR failed.", "Rebuild");
    }

    vkGetSwapchainImagesKHR(m_Device->GetVkDevice(), m_Swapchain, &imageCount, nullptr);
    m_Images.resize(imageCount);
    vkGetSwapchainImagesKHR(m_Device->GetVkDevice(), m_Swapchain, &imageCount, m_Images.data());

    m_ImageViews.resize(imageCount);
    m_Handles.resize(imageCount);
    m_VkFormat = surfaceFormat.format;
    m_Format = FromVkFormat(surfaceFormat.format);
    m_Extent = extent;

    for (uint32_t i = 0; i < imageCount; ++i) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_Images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_VkFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(m_Device->GetVkDevice(), &viewInfo, nullptr, &m_ImageViews[i]) != VK_SUCCESS) {
            return RHIError::Make(RHIErrorCode::BackendFailure, "Failed to create swapchain image view.", "Rebuild");
        }
        m_Handles[i] = m_Device->RegisterSwapchainTexture(m_Images[i], m_ImageViews[i], m_Extent, m_Format);
    }

    m_NeedsRebuild = false;
    m_CurrentIndex = 0;
    return RHIResult<void>::Success();
}

RHITextureHandle VulkanSwapchain::GetCurrentImage() const {
    if (m_Handles.empty()) {
        return RHITextureHandle::Invalid;
    }
    return m_Handles[m_CurrentIndex % m_Handles.size()];
}

VkImage VulkanSwapchain::GetVkImage(uint32_t index) const {
    return index < m_Images.size() ? m_Images[index] : VK_NULL_HANDLE;
}

VkImageView VulkanSwapchain::GetVkImageView(uint32_t index) const {
    return index < m_ImageViews.size() ? m_ImageViews[index] : VK_NULL_HANDLE;
}

RHIResult<void> VulkanSwapchain::Resize(Extent2D) {
    return Rebuild();
}

RHIResult<uint32_t> VulkanSwapchain::AcquireNextImage() {
    return AcquireNextImageForSlot(m_FrameSlot);
}

RHIResult<uint32_t> VulkanSwapchain::AcquireNextImageForSlot(uint32_t frameSlot) {
    if (!m_Device || !m_Swapchain || !m_ImageAvailable || m_ImageAvailable->empty()) {
        return RHIError::Make(RHIErrorCode::NotInitialized, "Swapchain not ready.", "AcquireNextImage");
    }

    const uint32_t slot = frameSlot % static_cast<uint32_t>(m_ImageAvailable->size());
    uint32_t imageIndex = 0;
    const VkResult result = vkAcquireNextImageKHR(
        m_Device->GetVkDevice(),
        m_Swapchain,
        UINT64_MAX,
        (*m_ImageAvailable)[slot],
        VK_NULL_HANDLE,
        &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        m_NeedsRebuild = true;
        (void)Rebuild();
        return RHIError::Make(RHIErrorCode::OutOfDate, "Swapchain out of date.", "AcquireNextImage");
    }
    if (result != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkAcquireNextImageKHR failed.", "AcquireNextImage", result);
    }
    m_CurrentIndex = imageIndex;
    m_FrameSlot = slot;
    return imageIndex;
}

RHIResult<void> VulkanSwapchain::Present() {
    return PresentForSlot(m_FrameSlot);
}

RHIResult<void> VulkanSwapchain::PresentForSlot(uint32_t frameSlot) {
    if (!m_Device || !m_Swapchain || !m_RenderFinished || m_RenderFinished->empty()) {
        return RHIError::Make(RHIErrorCode::NotInitialized, "Swapchain not ready.", "Present");
    }

    const uint32_t slot = frameSlot % static_cast<uint32_t>(m_RenderFinished->size());
    VkSemaphore waitSemaphores[] = {(*m_RenderFinished)[slot]};
    VkSwapchainKHR swapchains[] = {m_Swapchain};
    uint32_t imageIndices[] = {m_CurrentIndex};

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = waitSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = imageIndices;

    const VkResult result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        m_NeedsRebuild = true;
        return RHIError::Make(RHIErrorCode::OutOfDate, "Present out of date.", "Present");
    }
    if (result != VK_SUCCESS) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "vkQueuePresentKHR failed.", "Present", result);
    }
    return RHIResult<void>::Success();
}

} // namespace we::rhi::vulkan
