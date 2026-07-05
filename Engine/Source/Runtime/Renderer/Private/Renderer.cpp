#include "Renderer/Renderer.hpp"
#include "Renderer/RendererConfig.hpp"
#include "Core/Logger.hpp"

#if WE_HAS_VULKAN

#include <stdexcept>
#include <array>
#include <algorithm>
#include <iostream>
#include <cstring>

namespace we::runtime::renderer {

Renderer::Renderer(const std::shared_ptr<VulkanContext>& context, SDL_Window* window)
    : m_Context(context), m_Window(window) {
    
    HE_INFO("Renderer: Querying window size...");
    int width = 0;
    int height = 0;
    if (!SDL_GetWindowSizeInPixels(window, &width, &height) || width <= 0 || height <= 0) {
        SDL_GetWindowSize(window, &width, &height);
    }
    if (width <= 0 || height <= 0) {
        width = 1280;
        height = 720;
        HE_INFO("Renderer: Window not yet visible; using fallback size 1280x720 for swapchain.");
    }
    HE_INFO("Renderer: Window pixel size: " + std::to_string(width) + "x" + std::to_string(height));

    HE_INFO("Renderer: Creating Render Passes...");
    CreateRenderPasses();

    HE_INFO("Renderer: Creating Camera Uniform Buffers...");
    VkDeviceSize cameraBufferSize = sizeof(CameraUniform);
    for (int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame) {
        m_Context->CreateBuffer(
            cameraBufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_CameraBuffers[frame],
            m_CameraBufferMemories[frame]);
    }

    // Create Camera Descriptor Layout
    VkDescriptorSetLayoutBinding cameraLayoutBinding{};
    cameraLayoutBinding.binding = 0;
    cameraLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraLayoutBinding.descriptorCount = 1;
    cameraLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo cameraLayoutInfo{};
    cameraLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    cameraLayoutInfo.bindingCount = 1;
    cameraLayoutInfo.pBindings = &cameraLayoutBinding;

    if (vkCreateDescriptorSetLayout(m_Context->GetDevice(), &cameraLayoutInfo, nullptr, &m_CameraDescLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create camera descriptor set layout!");
    }

    // Allocate one camera descriptor set per frame-in-flight.
    for (int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame) {
        VkDescriptorSetAllocateInfo cameraAllocInfo{};
        cameraAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        cameraAllocInfo.descriptorPool = m_Context->GetDescriptorPool();
        cameraAllocInfo.descriptorSetCount = 1;
        cameraAllocInfo.pSetLayouts = &m_CameraDescLayout;

        if (vkAllocateDescriptorSets(m_Context->GetDevice(), &cameraAllocInfo, &m_CameraDescSets[frame]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate camera descriptor set!");
        }

        VkDescriptorBufferInfo cameraBufferInfo{};
        cameraBufferInfo.buffer = m_CameraBuffers[frame];
        cameraBufferInfo.offset = 0;
        cameraBufferInfo.range = sizeof(CameraUniform);

        VkWriteDescriptorSet cameraWrite{};
        cameraWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        cameraWrite.dstSet = m_CameraDescSets[frame];
        cameraWrite.dstBinding = 0;
        cameraWrite.dstArrayElement = 0;
        cameraWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        cameraWrite.descriptorCount = 1;
        cameraWrite.pBufferInfo = &cameraBufferInfo;

        vkUpdateDescriptorSets(m_Context->GetDevice(), 1, &cameraWrite, 0, nullptr);
    }

    HE_INFO("Renderer: Creating swapchain...");
    CreateSwapchain(static_cast<uint32_t>(width), static_cast<uint32_t>(height));

    HE_INFO("Renderer: Creating swapchain framebuffers...");
    CreateSwapchainFramebuffers();



    HE_INFO("Renderer: Creating offscreen viewport framebuffer...");
    m_OffscreenFramebuffer = std::make_unique<Framebuffer>(
        *m_Context,
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        m_OffscreenRenderPass
    );

    HE_INFO("Renderer: Allocating command buffers...");
    CreateCommandBuffers();

    HE_INFO("Renderer: Creating sync objects...");
    CreateSyncObjects();
}

Renderer::~Renderer() {
    vkDeviceWaitIdle(m_Context->GetDevice());

    m_OffscreenFramebuffer.reset();



    CleanupSwapchain();

    VkDevice device = m_Context->GetDevice();

    for (int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame) {
        if (m_CameraBuffers[frame] != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, m_CameraBuffers[frame], nullptr);
        }
        if (m_CameraBufferMemories[frame] != VK_NULL_HANDLE) {
            vkFreeMemory(device, m_CameraBufferMemories[frame], nullptr);
        }
    }
    if (m_CameraDescLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_CameraDescLayout, nullptr);
    }

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, m_ImageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(device, m_RenderFinishedSemaphores[i], nullptr);
        vkDestroyFence(device, m_InFlightFences[i], nullptr);
    }

    if (m_OffscreenRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, m_OffscreenRenderPass, nullptr);
    }
    if (m_SwapchainRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, m_SwapchainRenderPass, nullptr);
    }
}

void Renderer::CreateRenderPasses() {
    VkDevice device = m_Context->GetDevice();

    // -------------------------------------------------------------------------
    // 1. Offscreen Render Pass
    // -------------------------------------------------------------------------
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = kOffscreenColorFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    // Subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_OffscreenRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create offscreen render pass!");
    }

    // -------------------------------------------------------------------------
    // 2. Swapchain Render Pass (for ui)
    // -------------------------------------------------------------------------
    VkAttachmentDescription swapchainAttachment{};
    swapchainAttachment.format = VK_FORMAT_B8G8R8A8_UNORM; // Will update once swapchain format is selected
    swapchainAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    swapchainAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    swapchainAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    swapchainAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    swapchainAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    swapchainAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    swapchainAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference swapchainAttachmentRef{};
    swapchainAttachmentRef.attachment = 0;
    swapchainAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription swapchainSubpass{};
    swapchainSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    swapchainSubpass.colorAttachmentCount = 1;
    swapchainSubpass.pColorAttachments = &swapchainAttachmentRef;

    VkSubpassDependency swapchainDependency{};
    swapchainDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    swapchainDependency.dstSubpass = 0;
    swapchainDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    swapchainDependency.srcAccessMask = 0;
    swapchainDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    swapchainDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo swapchainRenderPassInfo{};
    swapchainRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    swapchainRenderPassInfo.attachmentCount = 1;
    swapchainRenderPassInfo.pAttachments = &swapchainAttachment;
    swapchainRenderPassInfo.subpassCount = 1;
    swapchainRenderPassInfo.pSubpasses = &swapchainSubpass;
    swapchainRenderPassInfo.dependencyCount = 1;
    swapchainRenderPassInfo.pDependencies = &swapchainDependency;

    // Note: We use temporary format B8G8R8A8_UNORM for layout, will create with selected format during CreateSwapchain
    m_SwapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;
    if (vkCreateRenderPass(device, &swapchainRenderPassInfo, nullptr, &m_SwapchainRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swapchain render pass!");
    }
}

void Renderer::CreateSwapchain(uint32_t width, uint32_t height) {
    VkPhysicalDevice physicalDevice = m_Context->GetPhysicalDevice();
    VkDevice device = m_Context->GetDevice();
    VkSurfaceKHR surface = m_Context->GetSurface();

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

    // 1. Choose Format
    VkSurfaceFormatKHR surfaceFormat = formats[0];
    for (const auto& availableFormat : formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = availableFormat;
            break;
        }
    }
    m_SwapchainFormat = surfaceFormat.format;

    // 2. Choose Present Mode (Prefer Mailbox for low latency, fallback to FIFO)
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& availablePresentMode : presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = availablePresentMode;
            break;
        }
    }

    // 3. Choose Extent
    VkExtent2D extent = { width, height };
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        extent = capabilities.currentExtent;
    } else {
        extent.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
    m_SwapchainExtent = extent;

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = m_SwapchainFormat;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = m_SwapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = { m_Context->GetGraphicsQueueFamily(), m_Context->GetPresentQueueFamily() };

    if (m_Context->GetGraphicsQueueFamily() != m_Context->GetPresentQueueFamily()) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    HE_INFO("Renderer: Creating Vulkan swapchain (size: " + std::to_string(m_SwapchainExtent.width) + "x" + std::to_string(m_SwapchainExtent.height) + ")...");
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_Swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan swapchain!");
    }

    // Retrieve images (query count first — do not reuse createInfo minImageCount here).
    uint32_t swapchainImageCount = 0;
    vkGetSwapchainImagesKHR(device, m_Swapchain, &swapchainImageCount, nullptr);
    m_SwapchainImages.resize(swapchainImageCount);
    vkGetSwapchainImagesKHR(device, m_Swapchain, &swapchainImageCount, m_SwapchainImages.data());

    // Create Image Views
    m_SwapchainImageViews.resize(m_SwapchainImages.size());
    for (size_t i = 0; i < m_SwapchainImages.size(); i++) {
        m_SwapchainImageViews[i] = m_Context->CreateImageView(
            m_SwapchainImages[i],
            m_SwapchainFormat,
            VK_IMAGE_ASPECT_COLOR_BIT
        );
    }
}

void Renderer::CleanupSwapchain() {
    VkDevice device = m_Context->GetDevice();

    for (auto framebuffer : m_SwapchainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    m_SwapchainFramebuffers.clear();

    for (auto imageView : m_SwapchainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    m_SwapchainImageViews.clear();

    if (m_Swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
        m_Swapchain = VK_NULL_HANDLE;
    }
}

void Renderer::RecreateSwapchain(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) {
        return; // Window is minimized
    }

    vkDeviceWaitIdle(m_Context->GetDevice());

    CleanupSwapchain();
    CreateSwapchain(width, height);
    CreateSwapchainFramebuffers();
}

void Renderer::CreateSwapchainFramebuffers() {
    VkDevice device = m_Context->GetDevice();
    m_SwapchainFramebuffers.resize(m_SwapchainImageViews.size());

    for (size_t i = 0; i < m_SwapchainImageViews.size(); i++) {
        VkImageView attachments[] = {
            m_SwapchainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_SwapchainRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_SwapchainExtent.width;
        framebufferInfo.height = m_SwapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_SwapchainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain Framebuffer!");
        }
    }
}

void Renderer::CreateCommandBuffers() {
    m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_Context->GetCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

    if (vkAllocateCommandBuffers(m_Context->GetDevice(), &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

void Renderer::CreateSyncObjects() {
    m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Create in signaled state so first frame doesn't wait

    VkDevice device = m_Context->GetDevice();

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan synchronization objects!");
        }
    }
}

bool Renderer::BeginFrame() {
    VkDevice device = m_Context->GetDevice();

    // Wait for previous frame to finish
    vkWaitForFences(device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

    // Acquire next swapchain image
    VkResult result = vkAcquireNextImageKHR(
        device,
        m_Swapchain,
        UINT64_MAX,
        m_ImageAvailableSemaphores[m_CurrentFrame],
        VK_NULL_HANDLE,
        &m_CurrentImageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        int w = 0;
        int h = 0;
        if (!SDL_GetWindowSizeInPixels(m_Window, &w, &h) || w <= 0 || h <= 0) {
            SDL_GetWindowSize(m_Window, &w, &h);
        }
        RecreateSwapchain(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image!");
    }

    if (m_SwapchainFramebuffers.empty() || m_CurrentImageIndex >= m_SwapchainFramebuffers.size()) {
        HE_ERROR("Renderer: swapchain framebuffer/image index mismatch (index="
            + std::to_string(m_CurrentImageIndex) + ", framebuffers="
            + std::to_string(m_SwapchainFramebuffers.size()) + "). Recreating swapchain.");
        int w = 0;
        int h = 0;
        if (!SDL_GetWindowSizeInPixels(m_Window, &w, &h) || w <= 0 || h <= 0) {
            SDL_GetWindowSize(m_Window, &w, &h);
        }
        RecreateSwapchain(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
        return false;
    }

    // Reset fence only if we are submitting work
    vkResetFences(device, 1, &m_InFlightFences[m_CurrentFrame]);

    VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];
    vkResetCommandBuffer(commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin command buffer recording!");
    }

    return true;
}

void Renderer::EndFrame() {
    VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to end command buffer recording!");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_Context->GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit Vulkan draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = { m_Swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &m_CurrentImageIndex;
    presentInfo.pResults = nullptr;

    VkResult result = vkQueuePresentKHR(m_Context->GetPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized) {
        m_FramebufferResized = false;
        int w = 0;
        int h = 0;
        if (!SDL_GetWindowSizeInPixels(m_Window, &w, &h) || w <= 0 || h <= 0) {
            SDL_GetWindowSize(m_Window, &w, &h);
        }
        RecreateSwapchain(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present Vulkan swapchain image!");
    }

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::UploadCameraUniform(const CameraUniform& ubo) {
    m_LastUploadedCameraUniform = ubo;
    void* data = nullptr;
    vkMapMemory(
        m_Context->GetDevice(),
        m_CameraBufferMemories[m_CurrentFrame],
        0,
        sizeof(CameraUniform),
        0,
        &data);
    std::memcpy(data, &ubo, sizeof(CameraUniform));
    vkUnmapMemory(m_Context->GetDevice(), m_CameraBufferMemories[m_CurrentFrame]);
}

VkCommandBuffer Renderer::GetCommandBuffer() const {
    if (m_CurrentFrame >= m_CommandBuffers.size()) {
        throw std::runtime_error("Renderer command buffer index out of range.");
    }
    return m_CommandBuffers[m_CurrentFrame];
}

VkFramebuffer Renderer::GetSwapchainFramebuffer(uint32_t index) const {
    if (index >= m_SwapchainFramebuffers.size()) {
        throw std::runtime_error("Swapchain framebuffer index out of range.");
    }
    return m_SwapchainFramebuffers[index];
}

VkBuffer Renderer::GetCameraBuffer() const {
    return m_CameraBuffers[m_CurrentFrame];
}

VkDescriptorSet Renderer::GetCameraDescSet() const {
    return m_CameraDescSets[m_CurrentFrame];
}

const Renderer::CameraUniform& Renderer::GetLastUploadedCameraUniform() const {
    return m_LastUploadedCameraUniform;
}

} // namespace we::runtime::renderer

#endif // WE_HAS_VULKAN
