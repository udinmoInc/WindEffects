using IgniteBT.BuildSystem;

public class Details : ModuleRules
{
    public Details(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        Dependencies.Add("Core");
        Dependencies.Add("CoreUObject");
        Dependencies.Add("Engine");

        Definitions.Add("DETAILS_EXPORTS");
    }
}
