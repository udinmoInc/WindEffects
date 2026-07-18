using IgniteBT.BuildSystem;

public class We : ModuleRules
{
    public We(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("we.exe");
        PublishAtConfigurationRoot();

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("AssetTools");
        // WeMain formats results / parses CLI flags using public types from these modules.
        PublicDependencies.Add("AssetImporter");
        PublicDependencies.Add("AssetProcessors");
        PublicDependencies.Add("AssetPipeline");
        PublicDependencies.Add("AssetCooker");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Console";
    }
}
