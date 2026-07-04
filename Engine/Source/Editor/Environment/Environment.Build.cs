using IgniteBT.BuildSystem;

public class Environment : ModuleRules
{
    public Environment(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Application");
        PublicDependencies.Add("World");
        PublicDependencies.Add("PropertyEditor");
        PublicDependencies.Add("ContentBrowser");
        PublicDependencies.Add("Menus");

        Definitions.Add("ENVIRONMENT_EXPORTS");
    }
}
