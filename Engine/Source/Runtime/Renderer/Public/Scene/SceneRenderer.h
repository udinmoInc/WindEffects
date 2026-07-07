#pragma once

#include "Camera/CameraSystem.h"
#include "Lighting/DirectionalLight.h"
#include "Resource/DepthTarget.h"
#include "Scene/MeshPrimitives.h"
#include <volk.h>
#if WE_HAS_GLM
#include <glm/glm.hpp>
#endif

namespace we::runtime::renderer {

class Renderer;
class DeviceContext;
class ResourceManager;
class GraphicsPipelineFactory;

struct SceneRendererConfig {
    Renderer* renderer = nullptr;
    DeviceContext* deviceContext = nullptr;
    ResourceManager* resourceManager = nullptr;
    GraphicsPipelineFactory* pipelineFactory = nullptr;
    uint32_t maxFramesInFlight = 2;
    VkFormat colorFormat = VK_FORMAT_UNDEFINED;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
};

class SceneRenderer {
public:
    SceneRenderer() = default;
    ~SceneRenderer();

    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;

    void Init(const SceneRendererConfig& config);
    void Shutdown();
    void Resize(uint32_t width, uint32_t height);
    bool ResizeIfNeeded(uint32_t width, uint32_t height);

    void DrawMeshes(VkCommandBuffer cmd) const;
    VkDescriptorSetLayout GetObjectDescriptorSetLayout() const { return m_ObjectDescriptorSetLayout; }
    VkDescriptorSet GetObjectDescriptorSet(uint32_t frameIndex) const;

    DepthTarget* GetDepthTarget() { return &m_DepthTarget; }
    const DepthTarget* GetDepthTarget() const { return &m_DepthTarget; }
    DirectionalLight* GetDirectionalLight() { return &m_DirectionalLight; }
    const DirectionalLight* GetDirectionalLight() const { return &m_DirectionalLight; }

    bool IsReady() const { return m_Initialized; }

private:
    struct ObjectUniform {
#if WE_HAS_GLM
        glm::mat4 model{1.0f};
        glm::vec4 color{0.72f, 0.74f, 0.78f, 1.0f};
#else
        float model[16]{};
        float color[4]{0.72f, 0.74f, 0.78f, 1.0f};
#endif
    };

    void CreateObjectResources(uint32_t frameCount);
    void UploadObjectUniforms();

    Renderer* m_Renderer = nullptr;
    DeviceContext* m_DeviceContext = nullptr;
    ResourceManager* m_ResourceManager = nullptr;

    MeshPrimitives m_MeshPrimitives;
    DepthTarget m_DepthTarget;
    DirectionalLight m_DirectionalLight;

    VkDescriptorSetLayout m_ObjectDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_ObjectDescriptorPool = VK_NULL_HANDLE;
    std::vector<VkBuffer> m_ObjectBuffers;
    std::vector<VkDeviceMemory> m_ObjectMemories;
    std::vector<VkDescriptorSet> m_ObjectDescriptorSets;
    ObjectUniform m_ObjectUniform{};

    bool m_Initialized = false;
};

} // namespace we::runtime::renderer
