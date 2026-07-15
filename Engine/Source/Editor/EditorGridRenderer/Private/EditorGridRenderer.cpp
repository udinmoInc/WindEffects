#include "EditorGridRenderer.h"

namespace we::editor::grid {

EditorGridRenderer& EditorGridRenderer::Get() {
    static EditorGridRenderer instance;
    return instance;
}

void EditorGridRenderer::Initialize(we::rhi::IRHIDevice* device) {
    m_Device = device;
    m_Initialized = m_Device != nullptr;
}

void EditorGridRenderer::Shutdown() {
    m_Device = nullptr;
    m_Initialized = false;
}

void EditorGridRenderer::Render(we::rhi::IRHICommandList*, const we::runtime::engine::EditorCamera&) const {}

} // namespace we::editor::grid
