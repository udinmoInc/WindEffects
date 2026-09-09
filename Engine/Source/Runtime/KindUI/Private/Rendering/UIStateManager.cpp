// ==============================================================================
// WindEffects — KindUI — UIStateManager
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Rendering/UIStateManager.h"

namespace we::runtime::kindui {

void UIStateManager::Initialize(we::rhi::IRHIDevice* device) { m_Device = device; }
void UIStateManager::Shutdown() { m_Device = nullptr; }
void UIStateManager::SaveState(we::rhi::IRHICommandList*, SavedGpuState& outState) { outState.valid = true; }
void UIStateManager::RestoreState(we::rhi::IRHICommandList*, const SavedGpuState&) {}
bool UIStateManager::ValidateState(const SavedGpuState& state) const { return state.valid; }

} // namespace we::runtime::kindui
 
