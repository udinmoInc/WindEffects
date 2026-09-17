// ==============================================================================
// WindEffects — Renderer — QualityTypes
// Internal implementation for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Renderer/Scalability/QualityTypes.h"

namespace we::runtime::renderer {

const char* ToString(RenderingProfileId id) noexcept {
    switch (id) {
    case RenderingProfileId::HighEnd: return "High-End";
    case RenderingProfileId::Balanced: return "Balanced";
    case RenderingProfileId::Low: return "Low";
    case RenderingProfileId::Custom: return "Custom";
    }
    return "Unknown";
}

const char* ToString(QualityLevel level) noexcept {
    switch (level) {
    case QualityLevel::Disabled: return "Disabled";
    case QualityLevel::Low: return "Low";
    case QualityLevel::Medium: return "Medium";
    case QualityLevel::High: return "High";
    case QualityLevel::Ultra: return "Ultra";
    case QualityLevel::Epic: return "Epic";
    }
    return "Unknown";
}

std::string_view ProfileFileStem(RenderingProfileId id) noexcept {
    switch (id) {
    case RenderingProfileId::HighEnd: return "HighEnd";
    case RenderingProfileId::Balanced: return "Balanced";
    case RenderingProfileId::Low: return "Low";
    case RenderingProfileId::Custom: return "Custom";
    }
    return "HighEnd";
}

RenderingProfileId ParseRenderingProfileId(std::string_view name) noexcept {
    if (name == "HighEnd" || name == "High-End" || name == "High End"
        || name == "AAA" || name == "AAAOpenWorld" || name == "AAA Open World") {
        return RenderingProfileId::HighEnd;
    }
    if (name == "Balanced" || name == "MobileHigh" || name == "Mobile High") {
        return RenderingProfileId::Balanced;
    }
    if (name == "Low" || name == "MobileLow" || name == "Mobile Low") {
        return RenderingProfileId::Low;
    }
    if (name == "Custom") {
        return RenderingProfileId::Custom;
    }
    return RenderingProfileId::HighEnd;
}

QualityLevel ClampQuality(QualityLevel level, QualityLevel maxLevel) noexcept {
    return static_cast<QualityLevel>(
        static_cast<uint8_t>(level) < static_cast<uint8_t>(maxLevel)
            ? static_cast<uint8_t>(level)
            : static_cast<uint8_t>(maxLevel));
}

} // namespace we::runtime::renderer
