using IgniteBT.BuildSystem;

public class Details : ModuleRules
{
    public Details(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("UIFramework");
        PublicDependencies.Add("PropertyEditor");
        PublicDependencies.Add("ContentBrowser");

        Definitions.Add("DETAILS_EXPORTS");
    }
}
