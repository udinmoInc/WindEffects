// ==============================================================================
// WindEffects — Renderer — ScalabilityDiagnostics
// Internal implementation for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Renderer/Scalability/ScalabilityDiagnostics.h"

#include "RHI/Types.h"

#include <sstream>

namespace we::runtime::renderer {
namespace {

const char* YesNo(bool value) {
    return value ? "Yes" : "No";
}

std::string FeatureLine(const char* name, bool enabled, QualityLevel quality) {
    if (!enabled || quality == QualityLevel::Disabled) {
        return std::string(name) + "        Disabled";
    }
    return std::string(name) + "        " + ToString(quality);
}

} // namespace

std::vector<std::string> FormatScalabilityDiagnostics(
    const ScalabilityDiagnosticsSnapshot& snap) {
    std::vector<std::string> lines;
    lines.emplace_back(std::string("Rendering Profile: ") + snap.profileName);
    lines.emplace_back(std::string("RHI: ") + we::rhi::ToString(snap.backend));
    lines.emplace_back("Capabilities:");
    lines.emplace_back(std::string("  Compute        ") + YesNo(snap.capabilities.maxComputeWorkGroupInvocations > 0));
    lines.emplace_back(std::string("  Indirect Draw  ") + YesNo(snap.capabilities.multiDrawIndirect));
    lines.emplace_back(std::string("  Mesh Shaders   ") + YesNo(snap.capabilities.meshShaders));
    lines.emplace_back(std::string("  Ray Tracing    ") + YesNo(snap.capabilities.rayTracing));
    lines.emplace_back(std::string("  Async Compute  ") + YesNo(snap.capabilities.asyncCompute));
    lines.emplace_back("Resolved Quality:");
    lines.push_back("  " + FeatureLine("Shadows", snap.settings.shadows.enabled, snap.settings.shadows.quality));
    lines.push_back("  " + FeatureLine("Geometry", true, snap.settings.geometry.quality));
    lines.push_back("  " + FeatureLine("Volumetrics", snap.settings.volumetrics.enabled, snap.settings.volumetrics.quality));
    lines.push_back("  " + FeatureLine("GI", snap.settings.globalIllumination.enabled, snap.settings.globalIllumination.quality));
    lines.push_back("  " + FeatureLine("Water", snap.settings.water.enabled, snap.settings.water.quality));
    lines.push_back("  " + FeatureLine("Terrain", snap.settings.terrain.enabled, snap.settings.terrain.quality));
    lines.push_back("  " + FeatureLine("Foliage", snap.settings.foliage.enabled, snap.settings.foliage.quality));
    lines.push_back("  " + FeatureLine("PostProcess", snap.settings.postProcess.enabled, snap.settings.postProcess.quality));

    if (!snap.settings.fallbackNotes.empty()) {
        lines.emplace_back("Fallbacks:");
        for (const auto& note : snap.settings.fallbackNotes) {
            lines.emplace_back("  " + note);
        }
    }
    return lines;
}

std::string FormatScalabilityDiagnosticsText(const ScalabilityDiagnosticsSnapshot& snap) {
    const auto lines = FormatScalabilityDiagnostics(snap);
    std::ostringstream ss;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) {
            ss << '\n';
        }
        ss << lines[i];
    }
    return ss.str();
}

} // namespace we::runtime::renderer
