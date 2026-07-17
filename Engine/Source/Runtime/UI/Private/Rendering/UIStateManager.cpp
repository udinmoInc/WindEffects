#include "WindEffects/Runtime/UI/Rendering/UIStateManager.h"

namespace WindEffects::Editor::UI {

void UIStateManager::Initialize(we::rhi::IRHIDevice* device) { m_Device = device; }
void UIStateManager::Shutdown() { m_Device = nullptr; }
void UIStateManager::SaveState(we::rhi::IRHICommandList*, SavedGpuState& outState) { outState.valid = true; }
void UIStateManager::RestoreState(we::rhi::IRHICommandList*, const SavedGpuState&) {}
bool UIStateManager::ValidateState(const SavedGpuState& state) const { return state.valid; }

} // namespace WindEffects::Editor::UI
