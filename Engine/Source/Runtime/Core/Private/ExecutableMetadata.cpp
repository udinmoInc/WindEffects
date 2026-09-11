#include "Core/ExecutableMetadata.h"

namespace we::core {
namespace ExecutableMetadata {

void SetRole(ExecutableRole role) {
    ProductMetadataService::Get().SetExecutableRole(role);
}

void SetIdentity(const ExecutableIdentity& identity) {
    ProductMetadataService::Get().SetExecutableIdentity(identity);
}

void ConfigureAsEngine() {
    SetRole(ExecutableRole::Engine);
    SetIdentity({
        .fileDescription = "WindEffects Engine",
        .internalName = "WindEffects",
        .originalFilename = "WindEffects.exe",
        .windowTitleBase = "WindEffects Engine"
    });
}

void ConfigureAsEditor() {
    SetRole(ExecutableRole::Editor);
    SetIdentity({
        .fileDescription = "WindEffects Editor",
        .internalName = "WindeffectsEditor",
        .originalFilename = "WindeffectsEditor.exe",
        .windowTitleBase = "WindEffects Editor"
    });
}

void ConfigureAsLauncher() {
    SetRole(ExecutableRole::Launcher);
    SetIdentity({
        .fileDescription = "WindEffects Launcher",
        .internalName = "WeLauncher",
        .originalFilename = "WeLauncher.exe",
        .windowTitleBase = "WindEffects Launcher"
    });
}

void ConfigureAsCrashReporter() {
    SetRole(ExecutableRole::CrashReporter);
    SetIdentity({
        .fileDescription = "WindEffects Crash Reporter",
        .internalName = "WECrashReporter",
        .originalFilename = "WECrashReporter.exe",
        .windowTitleBase = "WindEffects Crash Reporter"
    });
}

void ConfigureAsTool() {
    SetRole(ExecutableRole::Tool);
    SetIdentity({
        .fileDescription = "WindEffects Command-Line Tool",
        .internalName = "we",
        .originalFilename = "we.exe",
        .windowTitleBase = "WindEffects Tool"
    });
}

void ConfigureAsReflectionHardening() {
    SetRole(ExecutableRole::ReflectionHardening);
    SetIdentity({
        .fileDescription = "WindEffects Reflection Hardening Tool",
        .internalName = "ReflectionHardening",
        .originalFilename = "ReflectionHardening.exe",
        .windowTitleBase = "Reflection Hardening"
    });
}

void ConfigureAsSerializationHardening() {
    SetRole(ExecutableRole::SerializationHardening);
    SetIdentity({
        .fileDescription = "WindEffects Serialization Hardening Tool",
        .internalName = "SerializationHardening",
        .originalFilename = "SerializationHardening.exe",
        .windowTitleBase = "Serialization Hardening"
    });
}

void ConfigureAsAssetCompiler() {
    SetRole(ExecutableRole::AssetCompiler);
    SetIdentity({
        .fileDescription = "WindEffects Asset Compiler",
        .internalName = "WeAssetCompiler",
        .originalFilename = "WeAssetCompiler.exe",
        .windowTitleBase = "WindEffects Asset Compiler"
    });
}

void ConfigureAsShaderCompiler() {
    SetRole(ExecutableRole::ShaderCompiler);
    SetIdentity({
        .fileDescription = "WindEffects Shader Compiler",
        .internalName = "WeShaderCompiler",
        .originalFilename = "WeShaderCompiler.exe",
        .windowTitleBase = "WindEffects Shader Compiler"
    });
}

void ConfigureAsCooker() {
    SetRole(ExecutableRole::Cooker);
    SetIdentity({
        .fileDescription = "WindEffects Asset Cooker",
        .internalName = "WeCooker",
        .originalFilename = "WeCooker.exe",
        .windowTitleBase = "WindEffects Asset Cooker"
    });
}

void ConfigureAsPackager() {
    SetRole(ExecutableRole::Packager);
    SetIdentity({
        .fileDescription = "WindEffects Packager",
        .internalName = "WePackager",
        .originalFilename = "WePackager.exe",
        .windowTitleBase = "WindEffects Packager"
    });
}

void ConfigureAsCustom(
    const std::string& fileDescription,
    const std::string& internalName,
    const std::string& originalFilename,
    const std::string& windowTitleBase,
    ExecutableRole role)
{
    SetRole(role);
    SetIdentity({
        .fileDescription = fileDescription,
        .internalName = internalName,
        .originalFilename = originalFilename,
        .windowTitleBase = windowTitleBase
    });
}

} // namespace ExecutableMetadata
} // namespace we::core
