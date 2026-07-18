using IgniteBT.BuildSystem;

public class ContentBrowser : ModuleRules
{
    public ContentBrowser(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("UIFramework");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("Text");
        PublicDependencies.Add("AssetTools");
        PublicDependencies.Add("AssetImporter");
        PublicDependencies.Add("AssetPipeline");

        Definitions.Add("CONTENTBROWSER_EXPORTS");
    }
}
