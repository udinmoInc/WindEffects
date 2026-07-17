#include "WindEffects/BuildSDK.h"
#include "WindEffects/Editor/UI/Extensions/ExtensionBootstrap.h"
#include "KindUI/Commands/LambdaCommand.h"

REGISTER_COMMAND("build.compile", "Compile",
    [](const we::runtime::kindui::CommandContext&) {
        const auto result = we::build::CompileDebug();
        if (!result.launched) {
            WE_LOG_ERROR(we::LogCategory::Build.data(), std::string(result.errorMessage));
        }
    })
