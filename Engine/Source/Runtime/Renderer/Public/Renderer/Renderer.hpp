#pragma once

#if WE_HAS_VULKAN
#include "Renderer/VulkanContext.hpp"
#include "Renderer/Framebuffer.hpp"
#include <volk.h>
#endif
#if WE_HAS_GLM
#include <glm/glm.hpp>
#else
// Fallback simple math types when GLM is not available
struct glm_vec3 { float x, y, z; };
struct glm_vec4 { float x, y, z, w; };
struct glm_mat4 { float data[16]; };
namespace glm {
    using vec3 = glm_vec3;
    using vec4 = glm_vec4;
    using mat4 = glm_mat4;
}
#endif
#include <vector>
#include <memory>

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

class Renderer {
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    Renderer(const std::shared_ptr<VulkanContext>& context, SDL_Window* window);
    ~Renderer();

    // Prevent copying
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void RecreateSwapchain(uint32_t width, uint32_t height);

    bool BeginFrame();
    void EndFrame();

    VkRenderPass GetOffscreenRenderPass() const { return m_OffscreenRenderPass; }
    VkRenderPass GetSwapchainRenderPass() const { return m_SwapchainRenderPass; }
    VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffers[m_CurrentFrame]; }
    Framebuffer& GetOffscreenFramebuffer() const { return *m_OffscreenFramebuffer; }
    VkFramebuffer GetSwapchainFramebuffer(uint32_t index) const { return m_SwapchainFramebuffers[index]; }
    VkBuffer GetCameraBuffer() const { return m_CameraBuffer; }
    VkDescriptorSetLayout GetCameraDescLayout() const { return m_CameraDescLayout; }
    VkDescriptorSet GetCameraDescSet() const { return m_CameraDescSet; }

    struct CameraUniform {
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec3 pos;
        float padding;
    };

    void UpdateCameraBuffer(const CameraUniform& ubo);
    
    uint32_t GetCurrentFrameIndex() const { return m_CurrentFrame; }
    uint32_t GetCurrentImageIndex() const { return m_CurrentImageIndex; }
    
    uint32_t GetSwapchainWidth() const { return m_SwapchainExtent.width; }
    uint32_t GetSwapchainHeight() const { return m_SwapchainExtent.height; }

private:
    void CreateSwapchain(uint32_t width, uint32_t height);
    void CleanupSwapchain();
    void CreateRenderPasses();
    void CreateSwapchainFramebuffers();
    void CreateCommandBuffers();
    void CreateSyncObjects();

    std::shared_ptr<VulkanContext> m_Context;
    SDL_Window* m_Window;

    // Swapchain resources
    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    VkFormat m_SwapchainFormat;
    VkExtent2D m_SwapchainExtent;
    std::vector<VkImage> m_SwapchainImages;
    std::vector<VkImageView> m_SwapchainImageViews;
    std::vector<VkFramebuffer> m_SwapchainFramebuffers;

    // Render passes
    VkRenderPass m_OffscreenRenderPass = VK_NULL_HANDLE;
    VkRenderPass m_SwapchainRenderPass = VK_NULL_HANDLE;

    // Offscreen viewport framebuffer
    std::unique_ptr<Framebuffer> m_OffscreenFramebuffer;

    // Camera UBO
    VkBuffer m_CameraBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_CameraBufferMemory = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_CameraDescLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_CameraDescSet = VK_NULL_HANDLE;

    // Frame synchronization
    std::vector<VkCommandBuffer> m_CommandBuffers;
    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<VkFence> m_InFlightFences;

    uint32_t m_CurrentFrame = 0;
    uint32_t m_CurrentImageIndex = 0;
    bool m_FramebufferResized = false;
};

#endif // WE_HAS_VULKAN

} // namespace we::runtime::renderer
