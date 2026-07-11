#pragma once

// WindEffects Platform SDK — include this in every program and module.
// Logging, configuration, validation, paths, and module lifecycle.

#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "Core/EditorConfigPaths.h"
#include "Core/BuildPaths.h"
#include "Core/StartupValidator.h"
#include "Core/FrameCounter.h"
#include "Modules/IModuleInterface.h"
#include "WindEffects/ModuleInitializer.h"

#include <filesystem>
#include <string_view>

namespace we::platform {

// Log categories: use we::LogCategory::Build, we::LogCategory::Editor, etc.

inline void InitializeLogging() {
    we::runtime::core::Logger::Init();
}

inline void ShutdownLogging() {
    we::runtime::core::Logger::Shutdown();
}

[[nodiscard]] inline std::filesystem::path EditorConfig(std::string_view fileName) {
    return we::core::ResolveEditorConfigPath(std::string(fileName).c_str());
}

} // namespace we::platform
