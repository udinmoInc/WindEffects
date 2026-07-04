using IgniteBT.BuildSystem;

public class Details : ModuleRules
{
    public Details(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Application");
        PublicDependencies.Add("PropertyEditor");
        PublicDependencies.Add("ContentBrowser");

        Definitions.Add("DETAILS_EXPORTS");
    }
}
