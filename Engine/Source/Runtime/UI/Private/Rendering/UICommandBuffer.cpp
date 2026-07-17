#include "WindEffects/Runtime/UI/Rendering/UICommandBuffer.h"

namespace WindEffects::Editor::UI {

void UICommandBuffer::Initialize(we::rhi::IRHIDevice* device, uint32_t maxFramesInFlight) {
    m_Device = device;
    m_MaxFramesInFlight = maxFramesInFlight;
}

void UICommandBuffer::Shutdown() {
    m_Device = nullptr;
}

void UICommandBuffer::BeginRecording(uint32_t frameIndex) {
    m_ActiveFrame = frameIndex;
    m_DrawCallCount = 0;
}

void UICommandBuffer::RecordDraw(const UIRenderBatch&, const std::vector<UIVertex2>&, const std::vector<uint32_t>&) {
    ++m_DrawCallCount;
}

void UICommandBuffer::EndRecording() {}

void UICommandBuffer::Execute(we::rhi::IRHICommandList*) {}

} // namespace WindEffects::Editor::UI
