using IgniteBT.BuildSystem;

public class Core : ModuleRules
{
    public Core(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        BootstrapBinary();
        SetBinaryName("WindeffectsCore.dll");

        PublicIncludePaths.Add("Public");
        PublicIncludePaths.Add("Public/WindEffects");
        PublicIncludePaths.Add(".");
        PrivateIncludePaths.Add("Private");

        Definitions.Add("CORE_EXPORTS");
        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.LinkerFlags.Add("delayimp.lib");
        // Vulkan is optional for Core - only needed for rendering features
        OptionalSDK("VulkanSDK");
        
        // nlohmann/json is optional for Core - only needed for crash reporting
        AddOptionalThirdParty("nlohmann_json");
        
        // Add feature flags for SDK availability
        DefineIf(HasSDK("VulkanSDK"), "WE_HAS_VULKAN=1");
        DefineIf(!HasSDK("VulkanSDK"), "WE_HAS_VULKAN=0");
        DefineIf(HasThirdParty("nlohmann_json"), "WE_HAS_NLOHMANN_JSON=1");
        DefineIf(!HasThirdParty("nlohmann_json"), "WE_HAS_NLOHMANN_JSON=0");
    }
}
