#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "RHI/IRHI.h"

namespace WindEffects::Editor::UI {

struct UI_API SavedGpuState {
    bool valid = false;
};

class UI_API UIStateManager {
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

} // namespace WindEffects::Editor::UI
