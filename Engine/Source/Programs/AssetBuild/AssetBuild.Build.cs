using IgniteBT.BuildSystem;

public class AssetBuild : ModuleRules
{
    public AssetBuild(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("we-asset-build.exe");
        PublishAtConfigurationRoot();

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("AssetImporter");
        PublicDependencies.Add("AssetProcessors");
        PublicDependencies.Add("AssetPipeline");
        PublicDependencies.Add("AssetCooker");
        PublicDependencies.Add("AssetRuntime");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Console";
    }
}
