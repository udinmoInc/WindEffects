using IgniteBT.BuildSystem;

public class Toolbar : ModuleRules
{
    public Toolbar(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("Menus");
        PublicDependencies.Add("UIFramework");

        Definitions.Add("TOOLBAR_EXPORTS");
    }
}
