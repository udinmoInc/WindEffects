#pragma once

#include "Renderer/VulkanContext.hpp"
#include <volk.h>
#include <memory>
#include <vector>
#include <array>
#include <unordered_map>
#include <glm/glm.hpp>

namespace we::runtime::renderer {

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
    SceneRenderer(const std::shared_ptr<VulkanContext>& context, VkRenderPass renderPass, VkDescriptorSetLayout cameraDescLayout);
    ~SceneRenderer();

    // Prevent copying
    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;

    void DrawSkybox(VkCommandBuffer cmd, VkDescriptorSet cameraDescSet) const;
    
    void DrawMesh(VkCommandBuffer cmd, const std::string& meshName, VkDescriptorSet descriptorSet, int mode) const;

    VkDescriptorSetLayout GetObjectDescLayout() const { return m_ObjectDescLayout; }

    // Helpers to create descriptor sets for rendering
    void UpdateObjectDescriptorSet(VkDescriptorSet descriptorSet, VkBuffer cameraBuffer, VkBuffer objectBuffer) const;

private:
    void CreatePipelines(VkRenderPass renderPass);
    void CreateMeshes();
    
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

    // Pipelines
    VkPipeline m_SkyboxPipeline = VK_NULL_HANDLE;
    VkPipeline m_LitPipeline = VK_NULL_HANDLE;
    VkPipeline m_UnlitPipeline = VK_NULL_HANDLE;
    VkPipeline m_WireframePipeline = VK_NULL_HANDLE;

    // Meshes
    std::unordered_map<std::string, MeshBuffer> m_Meshes;
};

} // namespace we::runtime::renderer
