#include "Modules/IModuleInterface.h"
#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"

class ProjectManagerModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "ProjectManagerModule started");
    }

    void ShutdownModule() override {
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "ProjectManagerModule shutdown");
    }
};

IMPLEMENT_MODULE(ProjectManagerModule, WindEffects_ProjectManager)
