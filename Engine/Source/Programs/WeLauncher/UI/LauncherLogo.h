#pragma once

#include "RHI/Types.h"

#include <filesystem>
#include <string>

namespace WindEffects::Editor::UI {
class OverlayRenderer;
}

namespace we::programs::welauncher {

// Loads Assets/Editor/Logo/Logo_UI.png into a UI descriptor set.
// Returns Invalid if the file is missing or upload fails.
[[nodiscard]] we::rhi::RHIDescriptorSetHandle LoadLauncherLogoTexture(
    WindEffects::Editor::UI::OverlayRenderer* renderer,
    const std::filesystem::path& engineRoot,
    uint32_t displaySizePx);

[[nodiscard]] std::filesystem::path ResolveLauncherLogoPath(const std::filesystem::path& engineRoot);

} // namespace we::programs::welauncher
