#include "Modules/IModuleInterface.h"
#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"
#include "World/WorldReflection.h"

#include "Reflection/Reflection.h"

namespace {

class WorldModule final : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        we::runtime::world::RegisterWorldReflectionTypes(
            we::runtime::reflection::GetTypeRegistry());
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "WorldModule started (World Runtime)");
    }

    void ShutdownModule() override {
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "WorldModule shutdown");
    }
};

} // namespace

IMPLEMENT_MODULE(WorldModule, WindEffects_World)
