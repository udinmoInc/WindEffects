using IgniteBT.BuildSystem;

public class AssetRuntimeTests : ModuleRules
{
    public AssetRuntimeTests(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("AssetRuntimeTests.exe");
        PublishAtConfigurationRoot();

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("AssetImporter");
        PublicDependencies.Add("AssetCooker");
        PublicDependencies.Add("AssetRuntime");

        AddOptionalThirdParty("nlohmann_json");
        DefineIf(HasThirdParty("nlohmann_json"), "WE_HAS_NLOHMANN_JSON=1");
        DefineIf(!HasThirdParty("nlohmann_json"), "WE_HAS_NLOHMANN_JSON=0");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Console";
    }
}
