#include "Rendering/UiGpuUpload.h"
#include "RHI/IRHI.h"

namespace WindEffects::Editor::UI {

void UiGpuUpload::Init(we::rhi::IRHIDevice* device) {
    m_Device = device;
}

void UiGpuUpload::Shutdown() {
    m_Device = nullptr;
}

void UiGpuUpload::SubmitOneTime(const std::function<void(we::rhi::IRHICommandList&)>& record) {
    if (!m_Device || !record) {
        return;
    }
    // Immediate upload path will record through a dedicated transfer list once backends expose it.
    (void)record;
}

} // namespace WindEffects::Editor::UI
