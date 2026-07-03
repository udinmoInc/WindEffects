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

        Definitions.Add("DETAILS_EXPORTS");
    }
}
