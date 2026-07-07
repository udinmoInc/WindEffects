#include "EditorGridRenderer.hpp"
#include "EditorCamera.hpp"
#include "Core/DeviceContext.h"

namespace we::editor::grid {

EditorGridRenderer& EditorGridRenderer::Get() {
    static EditorGridRenderer instance;
    return instance;
}

void EditorGridRenderer::Initialize(we::runtime::renderer::DeviceContext* context,
                                    VkRenderPass /*renderPass*/,
                                    VkDescriptorSetLayout /*cameraDescLayout*/) {
    m_Context = context;
    m_Initialized = context != nullptr;
}

void EditorGridRenderer::Shutdown() {
    m_Context = nullptr;
    m_Initialized = false;
}

void EditorGridRenderer::Render(VkCommandBuffer /*commandBuffer*/,
                                VkDescriptorSet /*cameraDescriptorSet*/,
                                const we::runtime::engine::EditorCamera& /*camera*/) const {
}

} // namespace we::editor::grid
