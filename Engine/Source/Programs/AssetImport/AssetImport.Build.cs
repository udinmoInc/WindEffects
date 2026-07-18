using IgniteBT.BuildSystem;

public class AssetImport : ModuleRules
{
    public AssetImport(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("we-asset-import.exe");
        PublishAtConfigurationRoot();

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("AssetImporter");
        PublicDependencies.Add("Text");
        PublicDependencies.Add("Icons");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Console";
    }
}
