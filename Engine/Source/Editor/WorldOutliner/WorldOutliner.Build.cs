using IgniteBT.BuildSystem;

public class WorldOutliner : ModuleRules
{
    public WorldOutliner(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("Application");
        PublicDependencies.Add("ContentBrowser");

        Definitions.Add("WORLDOUTLINER_EXPORTS");
    }
}
