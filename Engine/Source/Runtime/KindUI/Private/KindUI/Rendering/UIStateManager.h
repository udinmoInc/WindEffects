// ==============================================================================
// WindEffects — KindUI — UIStateManager
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "RHI/IRHI.h"

namespace we::runtime::kindui {

struct KINDUI_API SavedGpuState {
    bool valid = false;
};

class KINDUI_API UIStateManager {
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

} // namespace we::runtime::kindui
