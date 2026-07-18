#include "Modules/IModuleInterface.h"
#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"

class ProjectsModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "ProjectsModule started");
    }

    void ShutdownModule() override {
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "ProjectsModule shutdown");
    }
};

IMPLEMENT_MODULE(ProjectsModule, WindEffects_Projects)
