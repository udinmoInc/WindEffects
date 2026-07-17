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
        PrivateIncludePaths.Add("Private");

        Definitions.Add("CORE_EXPORTS");
        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.LinkerFlags.Add("delayimp.lib");

        // nlohmann/json is optional for Core - only needed for crash reporting
        AddOptionalThirdParty("nlohmann_json");
        DefineIf(HasThirdParty("nlohmann_json"), "WE_HAS_NLOHMANN_JSON=1");
        DefineIf(!HasThirdParty("nlohmann_json"), "WE_HAS_NLOHMANN_JSON=0");
    }
}
