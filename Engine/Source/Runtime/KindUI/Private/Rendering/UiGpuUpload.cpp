// ==============================================================================
// WindEffects — KindUI — UiGpuUpload
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Rendering/UiGpuUpload.h"

namespace we::runtime::kindui {

void UiGpuUpload::Init(we::rhi::IRHIDevice* device) {
    m_Device = device;
}

void UiGpuUpload::Shutdown() {
    m_Device = nullptr;
}

} // namespace we::runtime::kindui
