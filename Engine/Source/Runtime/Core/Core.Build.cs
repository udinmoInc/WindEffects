using IgniteBT.BuildSystem;

public class Core : ModuleRules
{
    public Core(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        BootstrapBinary();
        SetBinaryName("WindeffectsCore.dll");

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        Definitions.Add("CORE_EXPORTS");
        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.LinkerFlags.Add("delayimp.lib");

        // nlohmann/json is optional for Core - only needed for crash reporting
        AddOptionalThirdParty("nlohmann_json");
        DefineIf(HasThirdParty("nlohmann_json"), "WE_HAS_NLOHMANN_JSON=1");
        DefineIf(!HasThirdParty("nlohmann_json"), "WE_HAS_NLOHMANN_JSON=0");

        // glm is Private-only (via Math/GlmInterop.h). Public APIs use Core/Math/Types.h.
        AddOptionalThirdParty("glm");
        DefineIf(HasThirdParty("glm"), "WE_HAS_GLM=1");
        DefineIf(!HasThirdParty("glm"), "WE_HAS_GLM=0");
    }
}
