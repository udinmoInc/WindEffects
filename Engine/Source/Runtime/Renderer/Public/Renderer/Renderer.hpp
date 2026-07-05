#pragma once

#if WE_HAS_VULKAN
#include "Renderer/Export.hpp"
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
#include <array>

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

class Renderer {
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    RENDERER_API Renderer(const std::shared_ptr<VulkanContext>& context, SDL_Window* window);
    RENDERER_API ~Renderer();

    // Prevent copying
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    RENDERER_API void RecreateSwapchain(uint32_t width, uint32_t height);

    RENDERER_API bool BeginFrame();
    RENDERER_API void EndFrame();

    VkRenderPass GetOffscreenRenderPass() const { return m_OffscreenRenderPass; }
    VkRenderPass GetSwapchainRenderPass() const { return m_SwapchainRenderPass; }
    RENDERER_API VkCommandBuffer GetCommandBuffer() const;
    Framebuffer& GetOffscreenFramebuffer() const { return *m_OffscreenFramebuffer; }
    RENDERER_API VkFramebuffer GetSwapchainFramebuffer(uint32_t index) const;
    RENDERER_API VkBuffer GetCameraBuffer() const;
    VkDescriptorSetLayout GetCameraDescLayout() const { return m_CameraDescLayout; }
    RENDERER_API VkDescriptorSet GetCameraDescSet() const;

    struct CameraUniform {
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec3 pos;
        float padding;
    };

    // Upload after BeginFrame() returns true (GPU fence for this frame slot has been waited on).
    RENDERER_API void UploadCameraUniform(const CameraUniform& ubo);
    RENDERER_API const CameraUniform& GetLastUploadedCameraUniform() const;
    
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

    // Per-frame camera UBOs (one buffer per frame-in-flight to avoid GPU/CPU races).
    std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> m_CameraBuffers{};
    std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> m_CameraBufferMemories{};
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_CameraDescSets{};
    VkDescriptorSetLayout m_CameraDescLayout = VK_NULL_HANDLE;
    CameraUniform m_LastUploadedCameraUniform{};

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
