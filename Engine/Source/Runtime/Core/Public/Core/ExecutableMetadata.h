#pragma once

#include "Core/ProductMetadata.h"
#include "Core/Export.h"

namespace we::core {

namespace ExecutableMetadata {

CORE_API void SetRole(ExecutableRole role);
CORE_API void SetIdentity(const ExecutableIdentity& identity);

CORE_API void ConfigureAsEngine();
CORE_API void ConfigureAsEditor();
CORE_API void ConfigureAsLauncher();
CORE_API void ConfigureAsCrashReporter();
CORE_API void ConfigureAsTool();
CORE_API void ConfigureAsAssetCompiler();
CORE_API void ConfigureAsShaderCompiler();
CORE_API void ConfigureAsCooker();
CORE_API void ConfigureAsPackager();
CORE_API void ConfigureAsReflectionHardening();
CORE_API void ConfigureAsSerializationHardening();

CORE_API void ConfigureAsCustom(
    const std::string& fileDescription,
    const std::string& internalName,
    const std::string& originalFilename,
    const std::string& windowTitleBase,
    ExecutableRole role = ExecutableRole::Custom);

} // namespace ExecutableMetadata

} // namespace we::core
