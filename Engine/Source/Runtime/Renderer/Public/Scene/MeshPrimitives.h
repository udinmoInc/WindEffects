#pragma once

#include <volk.h>
#include <vector>
#if WE_HAS_GLM
#include <glm/glm.hpp>
#endif

namespace we::runtime::renderer {

class DeviceContext;
class ResourceManager;

struct MeshVertex {
#if WE_HAS_GLM
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec2 uv{};
#else
    float position[3]{};
    float normal[3]{};
    float uv[2]{};
#endif
};

struct MeshPrimitive {
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexMemory = VK_NULL_HANDLE;
    uint32_t indexCount = 0;
};

struct MeshPrimitivesConfig {
    DeviceContext* deviceContext = nullptr;
    ResourceManager* resourceManager = nullptr;
};

class MeshPrimitives {
public:
    MeshPrimitives() = default;
    ~MeshPrimitives();

    MeshPrimitives(const MeshPrimitives&) = delete;
    MeshPrimitives& operator=(const MeshPrimitives&) = delete;

    void Init(const MeshPrimitivesConfig& config);
    void Shutdown();

    const MeshPrimitive& GetUnitCube() const { return m_UnitCube; }

private:
    MeshPrimitive CreateMesh(const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices);

    DeviceContext* m_DeviceContext = nullptr;
    ResourceManager* m_ResourceManager = nullptr;
    MeshPrimitive m_UnitCube{};
    bool m_Initialized = false;
};

} // namespace we::runtime::renderer
