#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "Text/Import/FontImporter.h"

#include <filesystem>
#include <string>

namespace WindEffects::Editor::UI {

class UI_API FontImportService {
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

} // namespace WindEffects::Editor::UI
