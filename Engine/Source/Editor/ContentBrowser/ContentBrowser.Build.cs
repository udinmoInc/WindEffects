using IgniteBT.BuildSystem;

public class ContentBrowser : ModuleRules
{
    public ContentBrowser(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        // Presentation/management layer — never owns asset lifetime or undo history.
        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("UIFramework");
        PublicDependencies.Add("Text");
        PublicDependencies.Add("AssetTools");
        PublicDependencies.Add("AssetImporter");
        PublicDependencies.Add("AssetPipeline");
        PublicDependencies.Add("AssetRuntime");
        PublicDependencies.Add("Reflection");
        PublicDependencies.Add("Serialization");
        // Undo is NOT a module dependency — Editor injects transaction callbacks to avoid
        // ContentBrowser ↔ PropertyEditor ↔ Undo cycles (PE uses CB SearchBox).

        PrivateDependencies.Add("RHI");
        PrivateDependencies.Add("Renderer");

        Definitions.Add("CONTENTBROWSER_EXPORTS");
    }
}
