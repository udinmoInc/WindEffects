// ==============================================================================
// WindEffects — KindUI — UiGpuUpload
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <functional>

namespace we::runtime::kindui {

// Backend-agnostic one-shot GPU upload helper. Graphics API details stay in RHI backends.
class UiGpuUpload {
public:
    void Init(we::rhi::IRHIDevice* device);
    void Shutdown();

    void SubmitOneTime(const std::function<void(we::rhi::IRHICommandList&)>& record);

    [[nodiscard]] we::rhi::IRHIDevice* GetDevice() const { return m_Device; }

private:
    we::rhi::IRHIDevice* m_Device = nullptr;
};

} // namespace we::runtime::kindui
