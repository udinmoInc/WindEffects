using IgniteBT.BuildSystem;

public class ContentBrowser : ModuleRules
{
    public ContentBrowser(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PublicIncludePaths.Add("Public/WindEffects");
        PublicIncludePaths.Add("Public/WindEffects/Editor");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("UIFramework");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("Text");

        Definitions.Add("CONTENTBROWSER_EXPORTS");
    }
}
