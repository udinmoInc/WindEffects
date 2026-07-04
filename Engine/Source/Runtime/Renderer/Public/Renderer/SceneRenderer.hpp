#pragma once

#if WE_HAS_VULKAN
#include "Renderer/Export.hpp"
#include "Renderer/VulkanContext.hpp"
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
    glm::vec3 skyAmbientColor{ 0.66f, 0.82f, 1.0f };
    float fogDensity = 0.02f;
    glm::vec3 skyLightLowerColor{ 0.05f, 0.05f, 0.06f };
    float fogHeightFalloff = 0.2f;
    glm::vec3 fogColor{ 0.72f, 0.78f, 0.85f };
    float fogStartDistance = 0.0f;
    glm::vec3 atmosphereRayleigh{ 0.005802f, 0.013558f, 0.033100f };
    float mieScattering = 0.003996f;
    glm::vec3 aerialTint{ 0.55f, 0.65f, 0.85f };
    float mieAnisotropy = 0.76f;
    glm::vec3 worldOrigin{ 0.0f, 0.0f, 0.0f };
    float exposureEV = 1.85f;
    float planetRadius = 6360.0f;
    float atmosphereHeight = 60.0f;
    float enableVolumetricFog = 1.0f;
    float enableClouds = 0.0f;
    int sunCastShadows = 1;
    int sunTemperature = 6500;
    glm::ivec2 padding{};
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
    // Editor-only empty world backdrop (no scene lighting contribution).
    struct EditorBackgroundSettings {
        glm::vec3 zenithColor{ 9.0f / 255.0f, 9.0f / 255.0f, 9.0f / 255.0f };
        float backgroundBrightness = 1.0f;
        glm::vec3 upperSkyColor{ 10.0f / 255.0f, 10.0f / 255.0f, 10.0f / 255.0f };
        float gradientStrength = 0.55f;
        glm::vec3 midSkyColor{ 11.0f / 255.0f, 11.0f / 255.0f, 11.0f / 255.0f };
        float horizonFade = 0.0f;
        glm::vec3 horizonColor{ 12.0f / 255.0f, 12.0f / 255.0f, 12.0f / 255.0f };
        float padding0 = 0.0f;
        glm::vec3 bottomColor{ 14.0f / 255.0f, 14.0f / 255.0f, 14.0f / 255.0f };
        float backgroundContrast = 1.0f;
    };

    RENDERER_API SceneRenderer(const std::shared_ptr<VulkanContext>& context, VkRenderPass renderPass, VkDescriptorSetLayout cameraDescLayout);
    RENDERER_API ~SceneRenderer();

    // Prevent copying
    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;

    RENDERER_API void DrawEditorBackground(VkCommandBuffer cmd, VkDescriptorSet cameraDescSet) const;
    RENDERER_API void DrawAtmospherePass(
        VkCommandBuffer cmd,
        VkDescriptorSet cameraDescSet) const;
    RENDERER_API void SetEditorBackgroundSettings(const EditorBackgroundSettings& settings);
    const EditorBackgroundSettings& GetEditorBackgroundSettings() const { return m_EditorBackgroundSettings; }
    void SetEditorBackgroundEnabled(bool enabled) { m_EnableEditorBackground = enabled; }
    bool IsEditorBackgroundEnabled() const { return m_EnableEditorBackground; }
    void SetAtmospherePassEnabled(bool enabled) { m_EnableAtmospherePass = enabled; }
    bool IsAtmospherePassEnabled() const { return m_EnableAtmospherePass; }
    
    RENDERER_API void DrawMesh(VkCommandBuffer cmd, const std::string& meshName, VkDescriptorSet descriptorSet, int mode) const;

    RENDERER_API void SetSceneEnvironment(const SceneEnvironmentUniform& environment);
    const SceneEnvironmentUniform& GetSceneEnvironment() const { return m_SceneEnvironment; }
    VkBuffer GetEnvironmentBuffer() const { return m_EnvironmentBuffer; }

    VkDescriptorSetLayout GetObjectDescLayout() const { return m_ObjectDescLayout; }

    // Helpers to create descriptor sets for rendering
    RENDERER_API void UpdateObjectDescriptorSet(VkDescriptorSet descriptorSet, VkBuffer cameraBuffer, VkBuffer objectBuffer) const;
    RENDERER_API void RefreshEnvironmentDescriptorBindings() const;

private:
    void CreatePipelines(VkRenderPass renderPass);
    void CreateMeshes();
    void UpdateEditorBackgroundBufferIfDirty();
    
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
    VkPipelineLayout m_SkyPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_SkyDescLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_SkyDescSet = VK_NULL_HANDLE;
    VkBuffer m_SkyBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_SkyBufferMemory = VK_NULL_HANDLE;
    EditorBackgroundSettings m_EditorBackgroundSettings{};
    bool m_EditorBackgroundDirty = true;
    bool m_EnableEditorBackground = false;
    bool m_EnableAtmospherePass = true;

    VkPipeline m_AtmospherePipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_AtmospherePipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_AtmosphereEnvDescLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_AtmosphereEnvDescSet = VK_NULL_HANDLE;

    SceneEnvironmentUniform m_SceneEnvironment{};
    mutable bool m_SceneEnvironmentDirty = true;
    VkBuffer m_EnvironmentBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_EnvironmentBufferMemory = VK_NULL_HANDLE;

    // Pipelines
    VkPipeline m_SkyboxPipeline = VK_NULL_HANDLE;
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
