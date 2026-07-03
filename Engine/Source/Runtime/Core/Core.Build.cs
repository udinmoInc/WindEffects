using IgniteBT.BuildSystem;

public class Core : ModuleRules
{
    public Core(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        Definitions.Add("CORE_EXPORTS");
        
        // Vulkan is optional for Core - only needed for rendering features
        OptionalSDK("Vulkan");
        
        // SDL3 is optional for Core - only needed for message boxes and platform features
        OptionalSDK("SDL3");
        
        // nlohmann/json is optional for Core - only needed for crash reporting
        OptionalThirdParty("nlohmann_json");
        
        // Add feature flags for SDK availability
        DefineIf(HasSDK("Vulkan"), "WE_HAS_VULKAN=1");
        DefineIf(!HasSDK("Vulkan"), "WE_HAS_VULKAN=0");
        DefineIf(HasSDK("SDL3"), "WE_HAS_SDL3=1");
        DefineIf(!HasSDK("SDL3"), "WE_HAS_SDL3=0");
        DefineIf(HasThirdParty("nlohmann_json"), "WE_HAS_NLOHMANN_JSON=1");
        DefineIf(!HasThirdParty("nlohmann_json"), "WE_HAS_NLOHMANN_JSON=0");
    }
}
