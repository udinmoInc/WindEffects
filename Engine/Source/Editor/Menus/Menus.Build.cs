using IgniteBT.BuildSystem;

public class Menus : ModuleRules
{
    public Menus(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        Dependencies.Add("Core");
        Dependencies.Add("Application");

        Definitions.Add("MENUS_EXPORTS");
    }
}
