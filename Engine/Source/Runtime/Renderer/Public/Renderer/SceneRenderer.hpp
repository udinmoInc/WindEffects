#pragma once

#if WE_HAS_VULKAN
#include "Renderer/Export.hpp"
#include "Renderer/VulkanContext.hpp"
#include "Renderer/SceneEnvironmentUniform.hpp"
#include "Renderer/AtmosphereLUTGenerator.hpp"
#include "Renderer/PostProcessStack.hpp"
#include <volk.h>
#endif
#if WE_HAS_GLM
#include <glm/glm.hpp>
#else
// Fallback simple math types when GLM is not available
#ifndef GLM_FALLBACK_TYPES_DEFINED
#define GLM_FALLBACK_TYPES_DEFINED
struct glm_vec3 { float x, y, z; };
struct glm_vec4 { float x, y, z, w; };
struct glm_mat4 { float data[16]; };
struct glm_ivec2 { int x, y; };
namespace glm {
    using vec3 = glm_vec3;
    using vec4 = glm_vec4;
    using mat4 = glm_mat4;
    using ivec2 = glm_ivec2;
}
#endif
#endif
#include <memory>
#include <vector>
#include <array>
#include <unordered_map>

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
        
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
        
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, normal);
        
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
        
        return attributeDescriptions;
    }
};

struct SceneObjectUniform {
    glm::mat4 model;
    glm::vec4 color;
    int mode; // 0 = Lit, 1 = Unlit, 2 = Wireframe
    int padding[3];
};

class SceneRenderer {
public:
    RENDERER_API SceneRenderer(
        const std::shared_ptr<VulkanContext>& context,
        VkRenderPass renderPass,
        VkRenderPass renderPassLoad,
        VkDescriptorSetLayout cameraDescLayout);
    RENDERER_API ~SceneRenderer();

    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;

    RENDERER_API void DrawSkyAtmospherePass(VkCommandBuffer cmd, VkDescriptorSet cameraDescSet) const;
    RENDERER_API void DrawVolumetricCloudsPass(VkCommandBuffer cmd, VkDescriptorSet cameraDescSet) const;
    RENDERER_API void ApplyPostExposure(
        VkCommandBuffer cmd,
        VkImage colorImage,
        VkImageView colorImageView,
        uint32_t width,
        uint32_t height) const;

    RENDERER_API void ResizePostProcess(uint32_t width, uint32_t height);

    RENDERER_API void PrepareAtmosphereLUTs(VkCommandBuffer cmd);
    RENDERER_API bool AreAtmosphereLUTsReady() const;
    RENDERER_API bool IsSkyPipelineCreated() const;
    RENDERER_API void LogAtmospherePipelineDiagnostics() const;
    RENDERER_API void LogAtmospherePipelineDiagnosticsOnChange() const;
    RENDERER_API void InvalidateCloudTemporalHistory();
    RENDERER_API void UpdateCloudTemporalState(
        const glm::vec3& cameraPos,
        const glm::vec3& cameraForward,
        const glm::mat4& view,
        const glm::mat4& proj);

    RENDERER_API void DrawFogCompositePass(
        VkCommandBuffer cmd,
        VkDescriptorSet cameraDescSet,
        VkImageView depthImageView,
        VkSampler sampler) const;

    RENDERER_API void DrawMesh(VkCommandBuffer cmd, const std::string& meshName, VkDescriptorSet descriptorSet, int mode) const;

    RENDERER_API void SetSceneEnvironment(const SceneEnvironmentUniform& environment);
    const SceneEnvironmentUniform& GetSceneEnvironment() const { return m_SceneEnvironment; }
    VkBuffer GetEnvironmentBuffer() const { return m_EnvironmentBuffer; }

    VkDescriptorSetLayout GetObjectDescLayout() const { return m_ObjectDescLayout; }

    // Helpers to create descriptor sets for rendering
    RENDERER_API void UpdateObjectDescriptorSet(VkDescriptorSet descriptorSet, VkBuffer cameraBuffer, VkBuffer objectBuffer) const;
    RENDERER_API void RefreshEnvironmentDescriptorBindings() const;
    RENDERER_API bool ValidateRenderFrame(VkFramebuffer framebuffer, uint32_t width, uint32_t height) const;
    RENDERER_API bool IsSkyPassReady() const;
    RENDERER_API bool IsPostPassReady() const;

private:
    void ValidateCreatedPipelines() const;
    void CreatePipelines(VkRenderPass renderPass);
    void CreateMeshes();
    VkPipeline CreateFullscreenPipeline(
        VkRenderPass renderPass,
        VkPipelineLayout layout,
        const char* shaderName,
        bool depthTest,
        VkCompareOp depthCompare,
        bool depthWrite,
        bool alphaBlend) const;
    
    void CreateMeshBuffers(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    void DestroyMeshes();

    struct MeshBuffer {
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
        uint32_t indexCount = 0;
    };

    std::shared_ptr<VulkanContext> m_Context;
    VkDescriptorSetLayout m_CameraDescLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_ObjectDescLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

    VkPipelineLayout m_EnvironmentPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_CloudPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_CloudCompositePipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_EnvironmentDescLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_EnvironmentDescSet = VK_NULL_HANDLE;

    VkPipelineLayout m_FogPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_FogLutDescLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_FogLutDescSet = VK_NULL_HANDLE;
    mutable VkImageView m_BoundFogDepthView = VK_NULL_HANDLE;

    std::unique_ptr<PostProcessStack> m_PostProcess;

    std::unique_ptr<AtmosphereLUTGenerator> m_LUTGenerator;

    SceneEnvironmentUniform m_SceneEnvironment{};
    mutable bool m_SceneEnvironmentDirty = true;
    VkBuffer m_EnvironmentBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_EnvironmentBufferMemory = VK_NULL_HANDLE;

    VkPipeline m_SkyAtmospherePipeline = VK_NULL_HANDLE;
    VkPipeline m_VolumetricCloudsPipeline = VK_NULL_HANDLE;
    VkPipeline m_CloudTemporalPipeline = VK_NULL_HANDLE;
    VkPipeline m_CloudCompositePipeline = VK_NULL_HANDLE;
    VkPipeline m_FogCompositePipeline = VK_NULL_HANDLE;
    VkPipeline m_LitPipeline = VK_NULL_HANDLE;
    VkPipeline m_UnlitPipeline = VK_NULL_HANDLE;
    VkPipeline m_WireframePipeline = VK_NULL_HANDLE;

    VkDescriptorSetLayout m_CloudHistoryDescLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_CloudHistoryDescSet = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_CloudCompositeDescLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_CloudCompositeDescSet = VK_NULL_HANDLE;
    VkSampler m_CloudSampler = VK_NULL_HANDLE;
    VkImage m_CloudScratchImage = VK_NULL_HANDLE;
    VkDeviceMemory m_CloudScratchMemory = VK_NULL_HANDLE;
    VkImageView m_CloudScratchView = VK_NULL_HANDLE;
    std::array<VkImage, 2> m_CloudHistoryImages{};
    std::array<VkDeviceMemory, 2> m_CloudHistoryMemories{};
    std::array<VkImageView, 2> m_CloudHistoryViews{};
    std::array<VkFramebuffer, 2> m_CloudHistoryFramebuffers{};
    VkFramebuffer m_CloudScratchFramebuffer = VK_NULL_HANDLE;
    VkRenderPass m_CloudScratchRenderPass = VK_NULL_HANDLE;
    VkRenderPass m_CloudHistoryRenderPass = VK_NULL_HANDLE;
    VkRenderPass m_OffscreenRenderPass = VK_NULL_HANDLE;
    VkRenderPass m_OffscreenRenderPassLoad = VK_NULL_HANDLE;
    mutable VkFramebuffer m_LastOffscreenFramebuffer = VK_NULL_HANDLE;
    uint32_t m_CloudTemporalWidth = 0;
    uint32_t m_CloudTemporalHeight = 0;
    mutable uint32_t m_LastOffscreenWidth = 0;
    mutable uint32_t m_LastOffscreenHeight = 0;
    uint32_t m_CloudHistoryWriteIndex = 0;
    bool m_CloudHistoryGpuValid = false;
    glm::vec3 m_PrevCloudCameraPos{ 0.0f };
    glm::vec3 m_PrevCloudCameraForward{ 0.0f, 0.0f, -1.0f };
    bool m_CloudTemporalInitialized = false;

    void EnsureCloudTemporalResources(uint32_t width, uint32_t height);
    void DestroyCloudTemporalResources();
    void UploadCloudTemporalEnvironmentFlags() const;
    mutable uint64_t m_LastAtmosphereDiagFingerprint = 0;
    mutable bool m_AtmosphereDiagInitialized = false;

    // Meshes
    std::unordered_map<std::string, MeshBuffer> m_Meshes;
};

#else

class SceneRenderer;

#endif // WE_HAS_VULKAN

} // namespace we::runtime::renderer
