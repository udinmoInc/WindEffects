// ==============================================================================
// WindEffects — Icons — IconThemeAudit
// Public API surface for the Icons module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Icons/Export.h"

#include <filesystem>
#include <string>
#include <vector>

namespace we::runtime::icons::compiling {

struct IconAuditFinding {
    enum class Severity { Warning, Error };

    Severity severity = Severity::Warning;
    std::string category;
    std::string message;
};

struct IconAuditReport {
    bool ok = true;
    std::vector<IconAuditFinding> findings;
    uint32_t iconCount = 0;
    uint32_t tierCount = 0;
};

[[nodiscard]] ICONS_API IconAuditReport AuditIconAtlas(const std::filesystem::path& inputDir);
ICONS_API void PrintIconAuditReport(const IconAuditReport& report);

} // namespace we::runtime::icons::compiling
