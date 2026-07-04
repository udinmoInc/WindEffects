#pragma once

#if WE_HAS_VULKAN
#include "Renderer/Export.hpp"
#include "Renderer/VulkanContext.hpp"
#include "Renderer/AtmosphereLUTGenerator.hpp"
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

struct SceneEnvironmentUniform {
    glm::vec3 sunDirection{ 0.38f, 0.92f, 0.18f };
    float sunIntensity = 10.0f;
    glm::vec3 sunColor{ 1.0f, 0.96f, 0.86f };
    float skyLightIntensity = 1.0f;
    glm::vec3 skyAmbientColor{ 0.05f, 0.08f, 0.12f };
    float fogDensity = 0.02f;
    glm::vec3 skyLightLowerColor{ 0.05f, 0.05f, 0.06f };
    float fogHeightFalloff = 0.2f;
    glm::vec3 fogColor{ 0.72f, 0.78f, 0.85f };
    float fogStartDistance = 0.0f;
    glm::vec3 atmosphereRayleigh{ 0.005802f, 0.013558f, 0.033100f };
    float mieScattering = 0.003996f;
    glm::vec3 ozoneAbsorption{ 0.00065f, 0.0018f, 0.00008f };
    float mieAnisotropy = 0.76f;
    glm::vec3 worldOrigin{ 0.0f, 0.0f, 0.0f };
    float exposureEV = 2.35f;
    float planetRadius = 6360.0f;
    float atmosphereHeight = 60.0f;
    float multiScatterStrength = 1.0f;
    float eyeAltitude = 0.001f;
    float cloudCoverage = 0.45f;
    float cloudAltitude = 5000.0f;
    float cloudExtinction = 0.35f;
    float enableClouds = 1.0f;
    glm::vec3 cloudColor{ 0.95f, 0.96f, 0.98f };
    float enableVolumetricFog = 1.0f;
    float exposureCompensation = 0.0f;
    float sunAngularRadius = 0.004675f;
    float hdrSkyLuminance = 1.0f;
    int sunCastShadows = 1;
    int sunTemperature = 6500;
    glm::ivec2 envPadding{};
};

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
    RENDERER_API SceneRenderer(const std::shared_ptr<VulkanContext>& context, VkRenderPass renderPass, VkDescriptorSetLayout cameraDescLayout);
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

    RENDERER_API void PrepareAtmosphereLUTs(VkCommandBuffer cmd);

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
    RENDERER_API bool IsSkyPassReady() const { return m_SkyAtmospherePipeline != VK_NULL_HANDLE && m_LUTGenerator != nullptr; }
    RENDERER_API bool IsPostPassReady() const { return m_PostExposurePipeline != VK_NULL_HANDLE; }

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
    VkDescriptorSetLayout m_EnvironmentDescLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_EnvironmentDescSet = VK_NULL_HANDLE;

    VkPipelineLayout m_FogPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_FogLutDescLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_FogLutDescSet = VK_NULL_HANDLE;
    mutable VkImageView m_BoundFogDepthView = VK_NULL_HANDLE;

    VkPipelineLayout m_PostPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_PostStorageDescLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_PostStorageDescSet = VK_NULL_HANDLE;
    mutable VkImageView m_BoundPostColorView = VK_NULL_HANDLE;
    VkPipeline m_PostExposurePipeline = VK_NULL_HANDLE;

    std::unique_ptr<AtmosphereLUTGenerator> m_LUTGenerator;

    SceneEnvironmentUniform m_SceneEnvironment{};
    mutable bool m_SceneEnvironmentDirty = true;
    VkBuffer m_EnvironmentBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_EnvironmentBufferMemory = VK_NULL_HANDLE;

    VkPipeline m_SkyAtmospherePipeline = VK_NULL_HANDLE;
    VkPipeline m_VolumetricCloudsPipeline = VK_NULL_HANDLE;
    VkPipeline m_FogCompositePipeline = VK_NULL_HANDLE;
    VkPipeline m_LitPipeline = VK_NULL_HANDLE;
    VkPipeline m_UnlitPipeline = VK_NULL_HANDLE;
    VkPipeline m_WireframePipeline = VK_NULL_HANDLE;

    // Meshes
    std::unordered_map<std::string, MeshBuffer> m_Meshes;
};

#else

class SceneRenderer;

#endif // WE_HAS_VULKAN

} // namespace we::runtime::renderer
