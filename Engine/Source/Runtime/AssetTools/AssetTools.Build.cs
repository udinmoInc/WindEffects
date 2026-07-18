using IgniteBT.BuildSystem;

public class AssetTools : ModuleRules
{
    public AssetTools(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("AssetImporter");
        PublicDependencies.Add("AssetProcessors");
        PublicDependencies.Add("AssetPipeline");
        PublicDependencies.Add("AssetCooker");
        PublicDependencies.Add("Icons");

        Definitions.Add("ASSETTOOLS_EXPORTS");
    }
}
