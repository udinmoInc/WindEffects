using IgniteBT.BuildSystem;

public class Menus : ModuleRules
{
    public Menus(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("UI");
        PublicDependencies.Add("UIFramework");

        Definitions.Add("MENUS_EXPORTS");
    }
}
