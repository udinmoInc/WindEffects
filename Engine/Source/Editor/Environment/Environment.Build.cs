using IgniteBT.BuildSystem;

public class Environment : ModuleRules
{
    public Environment(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("UIFramework");
        PublicDependencies.Add("World");
        PrivateDependencies.Add("PropertyEditor");
        PrivateDependencies.Add("ContentBrowser");
        PrivateDependencies.Add("Menus");

        Definitions.Add("ENVIRONMENT_EXPORTS");
    }
}
