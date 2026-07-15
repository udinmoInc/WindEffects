#pragma once

#include "EditorGridRenderer/Export.h"
#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <memory>

namespace we::runtime::engine {
class EditorCamera;
}

namespace we::editor::grid {

class EDITORGRIDRENDERER_API EditorGridRenderer {
public:
    static EditorGridRenderer& Get();

    void Initialize(we::rhi::IRHIDevice* device);
    void Shutdown();

    void Render(we::rhi::IRHICommandList* commandList, const we::runtime::engine::EditorCamera& camera) const;

    bool IsInitialized() const { return m_Initialized; }

private:
    EditorGridRenderer() = default;

    we::rhi::IRHIDevice* m_Device = nullptr;
    bool m_Initialized = false;
};

} // namespace we::editor::grid
