#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "Renderer/Export.h"
#include <volk.h>
#include <cstdint>
#if WE_HAS_GLM
#include <glm/glm.hpp>
#endif

namespace we::runtime::renderer {

class DeviceContext;
class ResourceManager;

// Per-frame cloud parameters (descriptor set 2, binding 0).
// Must match CloudFrameBuffer.hlsli.
struct CloudFrameUniformData {
#if WE_HAS_GLM
    glm::mat4 prevViewProj{1.0f};
    glm::vec3 prevCameraPos{0.0f};
    float historyValid = 0.0f;
    glm::vec2 invFullResolution{0.0f};
    glm::vec2 invCloudResolution{0.0f};
    glm::vec2 temporalJitter{0.0f};
    float resolutionScale = 0.5f;
    float maxMarchDistance = 28000.0f;
    float temporalBlend = 0.88f;
    float emptySkipThreshold = 0.035f;
    std::uint32_t frameCounter = 0;
    float domainRadius = 14000.0f;
#else
    float prevViewProj[16]{};
    float prevCameraPos[3]{};
    float historyValid = 0.0f;
#endif
};

#if WE_HAS_GLM
static_assert(sizeof(CloudFrameUniformData) == 128, "CloudFrameUniformData packing drift.");
#endif

struct CloudRenderTargetsConfig {
    DeviceContext* deviceContext = nullptr;
    ResourceManager* resourceManager = nullptr;
    uint32_t fullWidth = 1;
    uint32_t fullHeight = 1;
    float resolutionScale = 0.5f;
};

// Half-resolution cloud color/history targets + shared linear sampler.
class RENDERER_API CloudRenderTargets {
public:
    CloudRenderTargets() = default;
    ~CloudRenderTargets();

    CloudRenderTargets(const CloudRenderTargets&) = delete;
    CloudRenderTargets& operator=(const CloudRenderTargets&) = delete;

    void Init(const CloudRenderTargetsConfig& config);
    void Shutdown();
    void Resize(uint32_t fullWidth, uint32_t fullHeight, float resolutionScale);

    uint32_t CloudWidth() const { return m_CloudWidth; }
    uint32_t CloudHeight() const { return m_CloudHeight; }
    float ResolutionScale() const { return m_ResolutionScale; }

    VkImage ScratchImage() const { return m_Scratch.image; }
    VkImageView ScratchView() const { return m_Scratch.view; }
    VkImage HistoryImage(uint32_t index) const { return m_History[index & 1u].image; }
    VkImageView HistoryView(uint32_t index) const { return m_History[index & 1u].view; }
    VkImageView FallbackDepthView() const { return m_FallbackDepth.view; }
    VkImage FallbackDepthImage() const { return m_FallbackDepth.image; }
    VkImageLayout& FallbackDepthLayout() { return m_FallbackDepth.layout; }
    VkSampler Sampler() const { return m_Sampler; }
    VkFormat Format() const { return m_Format; }

    VkImageLayout& ScratchLayout() { return m_Scratch.layout; }
    VkImageLayout& HistoryLayout(uint32_t index) { return m_History[index & 1u].layout; }

private:
    struct Target {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    void DestroyTarget(Target& target);
    void CreateTarget(Target& target, uint32_t width, uint32_t height);
    void RecreateAll();

    DeviceContext* m_DeviceContext = nullptr;
    ResourceManager* m_ResourceManager = nullptr;
    uint32_t m_FullWidth = 0;
    uint32_t m_FullHeight = 0;
    uint32_t m_CloudWidth = 0;
    uint32_t m_CloudHeight = 0;
    float m_ResolutionScale = 0.5f;
    VkFormat m_Format = VK_FORMAT_R16G16B16A16_SFLOAT;
    VkSampler m_Sampler = VK_NULL_HANDLE;
    Target m_Scratch{};
    Target m_History[2]{};
    Target m_FallbackDepth{};
    bool m_Initialized = false;
};

} // namespace we::runtime::renderer

#pragma warning(pop)
