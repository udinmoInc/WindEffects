#pragma once

#include "EditorGridRenderer/Export.hpp"

#include <memory>
#include <volk.h>

namespace we::runtime::engine {
class EditorCamera;
}

namespace we::runtime::renderer {
class DeviceContext;
}

namespace we::editor::grid {

class EDITORGRIDRENDERER_API EditorGridRenderer {
public:
    static EditorGridRenderer& Get();

    void Initialize(we::runtime::renderer::DeviceContext* context,
                    VkRenderPass renderPass,
                    VkDescriptorSetLayout cameraDescLayout);
    void Shutdown();

    void Render(VkCommandBuffer commandBuffer,
                VkDescriptorSet cameraDescriptorSet,
                const we::runtime::engine::EditorCamera& camera) const;

    bool IsInitialized() const { return m_Initialized; }

private:
    EditorGridRenderer() = default;

    we::runtime::renderer::DeviceContext* m_Context = nullptr;
    bool m_Initialized = false;
};

} // namespace we::editor::grid
