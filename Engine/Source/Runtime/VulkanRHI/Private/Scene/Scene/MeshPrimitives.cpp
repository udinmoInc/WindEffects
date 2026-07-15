#include "Scene/MeshPrimitives.h"
#include "Core/DeviceContext.h"
#include "Core/Validation.h"
#include "Resource/ResourceManager.h"

#include <cstring>

namespace we::runtime::renderer {

MeshPrimitives::~MeshPrimitives() {
    Shutdown();
}

void MeshPrimitives::Init(const MeshPrimitivesConfig& config) {
    WE_VALIDATE_INIT(!m_Initialized, "MeshPrimitives", "Already initialized.");
    WE_VALIDATE_INIT(config.deviceContext != nullptr, "MeshPrimitives", "DeviceContext is null.");
    WE_VALIDATE_INIT(config.resourceManager != nullptr, "MeshPrimitives", "ResourceManager is null.");

    m_DeviceContext = config.deviceContext;
    m_ResourceManager = config.resourceManager;

    const std::vector<MeshVertex> cubeVertices = {
        {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    };

    const std::vector<uint32_t> cubeIndices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20
    };

    m_UnitCube = CreateMesh(cubeVertices, cubeIndices);
    m_Initialized = true;
}

void MeshPrimitives::Shutdown() {
    if (!m_Initialized) return;

    VkDevice device = m_DeviceContext->GetDevice();
    auto destroyMesh = [&](MeshPrimitive& mesh) {
        if (mesh.indexBuffer) vkDestroyBuffer(device, mesh.indexBuffer, nullptr);
        if (mesh.indexMemory) vkFreeMemory(device, mesh.indexMemory, nullptr);
        if (mesh.vertexBuffer) vkDestroyBuffer(device, mesh.vertexBuffer, nullptr);
        if (mesh.vertexMemory) vkFreeMemory(device, mesh.vertexMemory, nullptr);
        mesh = {};
    };

    destroyMesh(m_UnitCube);
    m_Initialized = false;
}

MeshPrimitive MeshPrimitives::CreateMesh(const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices) {
    MeshPrimitive mesh{};
    mesh.indexCount = static_cast<uint32_t>(indices.size());

    const VkDeviceSize vertexSize = sizeof(MeshVertex) * vertices.size();
    const VkDeviceSize indexSize = sizeof(uint32_t) * indices.size();

    m_ResourceManager->CreateBuffer(
        vertexSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        mesh.vertexBuffer,
        mesh.vertexMemory);

    void* vertexData = nullptr;
    vkMapMemory(m_DeviceContext->GetDevice(), mesh.vertexMemory, 0, vertexSize, 0, &vertexData);
    std::memcpy(vertexData, vertices.data(), static_cast<size_t>(vertexSize));
    vkUnmapMemory(m_DeviceContext->GetDevice(), mesh.vertexMemory);

    m_ResourceManager->CreateBuffer(
        indexSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        mesh.indexBuffer,
        mesh.indexMemory);

    void* indexData = nullptr;
    vkMapMemory(m_DeviceContext->GetDevice(), mesh.indexMemory, 0, indexSize, 0, &indexData);
    std::memcpy(indexData, indices.data(), static_cast<size_t>(indexSize));
    vkUnmapMemory(m_DeviceContext->GetDevice(), mesh.indexMemory);

    return mesh;
}

} // namespace we::runtime::renderer
