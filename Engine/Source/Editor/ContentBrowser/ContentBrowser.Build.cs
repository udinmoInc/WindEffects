using IgniteBT.BuildSystem;

public class ContentBrowser : ModuleRules
{
    public ContentBrowser(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        Dependencies.Add("Core");
        Dependencies.Add("CoreUObject");
        Dependencies.Add("Engine");
        Dependencies.Add("Application");

        Definitions.Add("CONTENTBROWSER_EXPORTS");
    }
}
