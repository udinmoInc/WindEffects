// ==============================================================================
// WindEffects — KindUI — FontImportService
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "Text/Import/FontImporter.h"

#include <filesystem>
#include <string>

namespace we::runtime::kindui {

class KINDUI_API FontImportService {
public:
    [[nodiscard]] static bool ImportFontFile(
        const std::filesystem::path& inputPath,
        const std::filesystem::path& outputDirectory,
        float bakeSizePx = 48.0f);
    [[nodiscard]] static bool ImportFontFile(
        const std::filesystem::path& inputPath,
        const std::filesystem::path& outputDirectory,
        float bakeSizePx,
        const std::string& charset);
};

} // namespace we::runtime::kindui
