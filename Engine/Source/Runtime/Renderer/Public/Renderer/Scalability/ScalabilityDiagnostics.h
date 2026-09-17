// ==============================================================================
// WindEffects — Renderer — ScalabilityDiagnostics
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Renderer/Export.h"
#include "Renderer/Scalability/QualityTypes.h"
#include "Renderer/Scalability/RenderingSettings.h"
#include "RHI/Capabilities.h"
#include "RHI/Types.h"

#include <string>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace we::runtime::renderer {

struct ScalabilityDiagnosticsSnapshot {
    RenderingProfileId profileId = RenderingProfileId::HighEnd;
    std::string profileName;
    we::rhi::RHIBackend backend = we::rhi::RHIBackend::Null;
    we::rhi::RHICapabilities capabilities{};
    ResolvedRenderingSettings settings{};
};

[[nodiscard]] RENDERER_API std::vector<std::string> FormatScalabilityDiagnostics(
    const ScalabilityDiagnosticsSnapshot& snap);

[[nodiscard]] RENDERER_API std::string FormatScalabilityDiagnosticsText(
    const ScalabilityDiagnosticsSnapshot& snap);

} // namespace we::runtime::renderer

#pragma warning(pop)
