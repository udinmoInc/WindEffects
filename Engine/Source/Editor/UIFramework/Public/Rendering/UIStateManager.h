#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "RHI/IRHI.h"

namespace WindEffects::Editor::UI {

struct UIFRAMEWORK_API SavedGpuState {
    bool valid = false;
};

class UIFRAMEWORK_API UIStateManager {
public:
    UIStateManager() = default;
    ~UIStateManager() = default;

    void Initialize(we::rhi::IRHIDevice* device);
    void Shutdown();

    void SaveState(we::rhi::IRHICommandList* cmd, SavedGpuState& outState);
    void RestoreState(we::rhi::IRHICommandList* cmd, const SavedGpuState& state);
    bool ValidateState(const SavedGpuState& state) const;

private:
    we::rhi::IRHIDevice* m_Device = nullptr;
};

using SavedVulkanState = SavedGpuState; // temporary alias during cutover

} // namespace WindEffects::Editor::UI
